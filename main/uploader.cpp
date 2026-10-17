/*
 * SPDX-FileCopyrightText: 2026 Dirk Wahrheit
 * SPDX-License-Identifier: Apache-2.0
 */
#include "uploader.h"

#include <cinttypes>
#include <cstdio>
#include <vector>

#include "esp_log.h"
#include "esp_timer.h"
#include "espos_cfg_keys.h"
#include "espos_config.h"
#include "espos_sk_http.h"
#include "mbedtls/base64.h"

namespace bathy_bridge {

namespace {

const char* TAG = "bathy_upload";

/* A whole chunk (16 KiB by default, ~22 KiB as base64) has to reach the
 * server inside one request, over a dinghy's marginal WiFi. */
constexpr uint32_t kTimeoutMs = 20000;

void append_json_string(std::string& out, const std::string& s) {
  out += '"';
  for (unsigned char ch : s) {
    if (ch == '"' || ch == '\\') {
      out += '\\';
      out += static_cast<char>(ch);
    } else if (ch < 0x20) {
      char esc[8];
      snprintf(esc, sizeof(esc), "\\u%04x", ch);
      out += esc;
    } else {
      out += static_cast<char>(ch);
    }
  }
  out += '"';
}

}  // namespace

std::string upload_body(const Identity& id, const char* format,
                        const espos_usb_storage::Chunk& c) {
  size_t b64_len = 0;
  mbedtls_base64_encode(nullptr, 0, &b64_len, c.data, c.len);
  std::vector<unsigned char> b64(b64_len);
  if (mbedtls_base64_encode(b64.data(), b64.size(), &b64_len, c.data, c.len) !=
      0) {
    return {};
  }

  std::string body;
  body.reserve(b64_len + 512);
  body += "{\"vessel\":";
  append_json_string(body, id.vessel);
  body += ",\"device\":{\"app\":";
  append_json_string(body, id.app);
  body += ",\"version\":";
  append_json_string(body, id.version);
  body += ",\"board\":";
  append_json_string(body, id.board);
  body += "},\"format\":";
  append_json_string(body, format);
  body += ",\"file\":";
  append_json_string(body, c.path);
  char nums[160];
  snprintf(nums, sizeof(nums),
           ",\"offset\":%" PRIu64 ",\"length\":%u,\"restarted\":%s,\"crc32\":\"%08" PRIx32
           "\",\"data\":\"",
           c.offset, static_cast<unsigned>(c.len), c.restarted ? "true" : "false",
           espos_usb_storage::crc32(0, c.data, c.len));
  body += nums;
  body.append(reinterpret_cast<const char*>(b64.data()), b64_len);
  body += "\"}";
  return body;
}

uint32_t backoff_ms(uint32_t failures) {
  constexpr uint32_t kFirst = 10000;
  constexpr uint32_t kMax = 300000;
  if (failures == 0) return 0;
  uint32_t ms = kFirst;
  for (uint32_t i = 1; i < failures && ms < kMax; ++i) ms *= 2;
  return ms < kMax ? ms : kMax;
}

bool Uploader::operator()(const espos_usb_storage::Chunk& c) {
  bool enabled = true;
  espos_config_get_bool(ESPOS_CFG_NS_BRIDGE, ESPOS_CFG_BRIDGE_UPLOAD, &enabled);
  if (!enabled) return false;

  /* Refusing without a request ends the follower's pass; it offers the same
   * bytes again on its next poll, so this only spaces the attempts out. */
  if (esp_timer_get_time() < next_try_us_) return false;

  char path[128];
  if (espos_config_get_str(ESPOS_CFG_NS_BRIDGE, ESPOS_CFG_BRIDGE_UPLOAD_PATH, path,
                           sizeof(path), nullptr) != ESP_OK ||
      path[0] != '/') {
    ESP_LOGE(TAG, "bridge.upload_path must start with '/'");
    return false;
  }

  const std::string body = upload_body(id_, "raymarine-realbathy-trl", c);
  bool ok = false;
  if (!body.empty()) {
    espos_sk_http_opts_t o = {};
    o.timeout_ms = kTimeoutMs;
    o.max_body = 1024;
    espos_sk_http_resp_t r = {};
    const esp_err_t err = espos_sk_http_post(path, body.c_str(), &o, &r);
    ok = err == ESP_OK && r.status >= 200 && r.status < 300;
    if (ok) {
      ESP_LOGI(TAG, "%s: %u bytes from %" PRIu64 " uploaded", c.path.c_str(),
               static_cast<unsigned>(c.len), c.offset);
    } else if (err == ESP_OK) {
      ESP_LOGW(TAG, "%s: server answered %d at %s", c.path.c_str(), r.status, path);
    } else {
      ESP_LOGW(TAG, "%s: no upload (%s)", c.path.c_str(), esp_err_to_name(err));
    }
    espos_sk_http_resp_free(&r);
  } else {
    ESP_LOGE(TAG, "%s: could not encode %u bytes from %" PRIu64, c.path.c_str(),
             static_cast<unsigned>(c.len), c.offset);
  }

  failures_ = ok ? 0 : failures_ + 1;
  next_try_us_ = esp_timer_get_time() + static_cast<int64_t>(backoff_ms(failures_)) * 1000;
  return ok;
}

}  // namespace bathy_bridge
