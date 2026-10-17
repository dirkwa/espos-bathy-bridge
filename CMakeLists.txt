# SPDX-FileCopyrightText: 2026 Dirk Wahrheit
# SPDX-License-Identifier: Apache-2.0
#
# espos-bathy-bridge: a pure ESP-IDF 6 project. WiFi, config, the web UI,
# Signal K discovery and token, OTA, the USB mass-storage export and the file
# follower come from the espOS submodule (espos/); this project picks the
# components, the card pins and where the records go.
#
# Build:  . <IDF v6.0.3>/export.sh
#         scripts/build.sh -B build-esp32p4 -DSDKCONFIG=build-esp32p4/sdkconfig -DIDF_TARGET=esp32p4 build
cmake_minimum_required(VERSION 3.22)

# The Waveshare ESP32-P4 boards carry 16 MB or more. A device keeps the table
# it was first flashed with (OTA replaces app slots, never the table), so a
# change here reaches only boards flashed over USB afterwards.
include("${CMAKE_CURRENT_LIST_DIR}/espos/cmake/espos_project.cmake")
set(_partitions "${ESPOS_PARTITIONS_DIR}/16mb.csv")
espos_project_prologue(NAME "bathy-bridge" PARTITIONS "${_partitions}"
                       COMPONENTS espos_usb_storage espos_eth)

project(bathy_bridge)

# The committed espOS web UI (espos/ui/dist-gz) -> LittleFS "storage" image,
# flashed with `idf.py flash`. No Node step anywhere.
espos_project_ui_partition(NAME "bathy-bridge")
