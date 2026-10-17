/*
 * SPDX-FileCopyrightText: 2026 Dirk Wahrheit
 * SPDX-License-Identifier: Apache-2.0
 *
 * espos-bathy-bridge: this board's microSD card appears to a chartplotter as
 * a second storage card on its USB port. A Raymarine Axiom (LightHouse 4)
 * records RealBathy soundings there, realbathy/<id>.trl, and once the plotter
 * has gone quiet every new record is uploaded to the boat's Signal K server
 * under this device's own vessel context, so a dinghy's soundings are never
 * filed as the mothership's.
 *
 * Recorded, not live: the records reach the server minutes or days after they
 * were measured, so they go as an upload and never as deltas, which the
 * server would take as the vessel's current state (espos/docs/signalk.md).
 *
 * espOS never writes to the card while the plotter has it, and progress lives
 * in NVS, advanced only after the server answered 2xx: a boat out of WiFi
 * range, a power cut or a server without the endpoint loses nothing.
 */
#include <cstdint>
#include <utility>

#include "esp_app_desc.h"
#include "esp_log.h"
#include "espos.h"
#include "espos_sk.h"
#include "espos_usb_storage/usb_storage.h"
#include "uploader.h"

namespace {

const char* TAG = "bathy_bridge";

/* The label espOS reports and the registry matches an update against. */
constexpr const char* kBoard = "Waveshare ESP32-P4-ETH";

/* RealBathy writes fixed 24-byte records (position, depth, time), appended
 * while the plotter records; only whole records are handed on. */
constexpr uint32_t kTrlRecord = 24;

}  // namespace

extern "C" void app_main(void) {
  espos_start_opts_t opts = ESPOS_START_OPTS_DEFAULT;
  opts.board = kBoard;
  ESP_ERROR_CHECK(espos_start(&opts));

  /* The Waveshare ESP32-P4 boards' TF slot (the P4-ETH and the Touch-LCD
   * panels share it). The plotter connects to the P4's high-speed OTG port,
   * on the P4-ETH the 4-pin header J3, which is espos_usb_storage's default. */
  espos_usb_storage::Config cfg = {};
  cfg.sd = {.clk = 43,
            .cmd = 44,
            .d0 = 39,
            .d1 = 40,
            .d2 = 41,
            .d3 = 42,
            .slot = 0,
            .freq_khz = 40000,
            .ldo_chan = 4};
  if (espos_usb_storage::start(cfg) != ESP_OK) {
    ESP_LOGE(TAG, "no card exported: check that a FAT32 microSD is inserted");
    return;
  }

  /* Without its own vessel the records could only be filed under the
   * mothership, which is the one thing this bridge must never do. */
  const char* vessel = espos_sk_vessel_context();
  if (vessel == nullptr) {
    ESP_LOGE(TAG, "no vessel UUID in NVS: the card is exported, nothing is uploaded");
    return;
  }

  bathy_bridge::Identity id;
  id.vessel = vessel;
  id.app = espos_app_name();
  id.version = esp_app_get_description()->version;
  id.board = kBoard;
  ESP_LOGI(TAG, "uploading RealBathy records as %s", id.vessel.c_str());

  espos_usb_storage::FollowConfig follow;
  follow.follower.watches = {{"realbathy", ".trl", kTrlRecord}};
  follow.sink = bathy_bridge::Uploader(std::move(id));
  ESP_ERROR_CHECK(espos_usb_storage::follow(std::move(follow)));
}
