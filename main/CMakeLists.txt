# SPDX-FileCopyrightText: 2026 Dirk Wahrheit
# SPDX-License-Identifier: Apache-2.0
#
# espos_usb_storage exports the card and follows the plotter's files,
# espos_sk carries the upload, espos_config the "bridge" settings. espos_ota
# and espos_eth are listed for the build, not for main.cpp: espOS consumers
# build with IDF's MINIMAL_BUILD, so a component nothing requires is not
# compiled, and espos_start() starts only what is in the build. Without
# espos_ota the bridge could never be updated in the field; without espos_eth
# a P4-ETH on its cable would ignore it.
idf_component_register(
    SRCS main.cpp uploader.cpp
    PRIV_REQUIRES espos_core espos_config espos_sk espos_usb_storage espos_eth
                  espos_ota esp_app_format esp_timer mbedtls)

# The upload settings page (namespace "bridge"). After idf_component_register.
espos_config_add_descriptor(config/bridge.json)
