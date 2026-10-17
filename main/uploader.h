/*
 * SPDX-FileCopyrightText: 2026 Dirk Wahrheit
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "espos_usb_storage/follower.h"

namespace bathy_bridge {

/* What one upload is labelled with, fixed at boot. */
struct Identity {
  std::string vessel;  // espos_sk_vessel_context()
  std::string app;
  std::string version;
  std::string board;
};

/* The request body: one run of whole records, base64, with where it sits in
 * the plotter's file so the server can place a resent run idempotently. */
std::string upload_body(const Identity& id, const char* format,
                        const espos_usb_storage::Chunk& c);

/* The delay before the next attempt after `failures` failures in a row: 10 s,
 * doubling, at most 5 min. A boat out of WiFi range fails every attempt, and
 * the server should not see a request every few seconds once it is back. */
uint32_t backoff_ms(uint32_t failures);

/* The follower's sink. Blocks on the network (the follower's own task, never
 * the stream or a URI handler), and returns true only for a 2xx, so a run
 * the server did not take is offered again and nothing is lost. */
class Uploader {
 public:
  explicit Uploader(Identity id) : id_(std::move(id)) {}
  bool operator()(const espos_usb_storage::Chunk& c);

 private:
  Identity id_;
  uint32_t failures_ = 0;
  int64_t next_try_us_ = 0;
};

}  // namespace bathy_bridge
