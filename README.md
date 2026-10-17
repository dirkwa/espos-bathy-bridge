# espos-bathy-bridge

Gets the depth soundings your chartplotter records into Signal K, from a boat
that has no Signal K server of its own, such as a dinghy.

The bridge is a small ESP32-P4 board with a microSD card. Plugged into the
chartplotter's USB port, the card shows up on the plotter as a second storage
card, and the plotter records to it. Whenever the plotter has stopped writing
for a while, the bridge sends every new record over WiFi to
[crowd-depth](https://github.com/openwatersio/crowd-depth) on your main
boat's Signal K server. It labels them as a vessel of their own, so the
dinghy's soundings are never mixed up with the main boat's.

Nothing is lost while the dinghy is out of range: the records stay on the
card, and the bridge uploads them the next time it reaches the server. The
bridge never writes to the card while the plotter is using it.

**Status:** it builds; it has not been tested on hardware yet. The upload
endpoint in crowd-depth is still being agreed, so for now the records wait
on the card.

## What it works with

| Chartplotter | What it records | Status |
|---|---|---|
| Raymarine Axiom on LightHouse 4 | RealBathy (`realbathy/<id>.trl`) | builds only |

On the plotter, choose the USB card as the save location for RealBathy.

## Hardware

| Board | Flash | Status |
|---|---|---|
| Waveshare ESP32-P4-ETH (with or without PoE) | 16 MB or 32 MB | builds only |

You also need a microSD card formatted FAT32.

**Never power the board from PoE and USB at the same time.** It damages the
board.

## Wiring

The board's USB-C socket is only the serial console; it cannot act as a
storage card. The plotter connects to the 4-pin header **J3** ("USB
Interface"), the P4's high-speed USB port. Wire a USB-A plug to it:

| J3 pin | Signal | USB-A pin |
|---|---|---|
| 1 | 5V | 1 (VBUS) |
| 2 | D− | 2 |
| 3 | D+ | 3 |
| 4 | GND | 4 |

The board takes its power from the plotter over this cable; J3's 5V feeds the
board before its regulator. Whether every plotter supplies enough current for
it is still to be tested.

## Install

Flash it from the
[hosted flasher](https://signalk-espos.github.io/signalk-espos-manager/flash/)
in Chrome or Edge over the USB-C socket, or from the Flash page of the
signalk-espos-manager plugin. For manual flashing, write the `-merged.bin` at
offset `0x0`.

## First setup

1. Join the `espOS-xxxx` WiFi network the board opens and enter your main
   boat's WiFi.
2. In Signal K, approve the bridge's access request under Security → Access
   Requests.
3. In the bridge's web UI, the **Chartplotter bridge** settings choose whether
   to upload and the upload path on the server.

The bridge's vessel identity, `vessels.urn:mrn:signalk:uuid:<uuid>`, is
created once and kept across updates. It is shown as `vessel_context` in the
Signal K status of the device's web UI. Erasing the board's settings makes a
new vessel.

## Updates

Over the air through signalk-espos-manager. A board that ran another project's
firmware needs one USB flash first.

## When it misbehaves

The device log is at `http://<device>/api/v1/logs`. Lines tagged
`bathy_upload` say what the server answered to each upload. Report issues at
https://github.com/dirkwa/espos-bathy-bridge/issues.

## Building

```bash
git clone --recurse-submodules https://github.com/dirkwa/espos-bathy-bridge
. <ESP-IDF v6.0.3>/export.sh       # the version in .idf-version
scripts/build.sh -B build-esp32p4 -DSDKCONFIG=build-esp32p4/sdkconfig -DIDF_TARGET=esp32p4 build
```

The upload is a JSON `POST` to the Signal K server (default path
`/plugins/crowd-depth/realbathy`), with the device's Signal K token. The body
carries one run of whole records: `vessel`, `device`, `format`
(`raymarine-realbathy-trl`), `file`, `offset`, `length`, `restarted`,
`crc32` (zlib, hex) and `data` (base64). A file's runs arrive in order;
`restarted` means the plotter replaced the file and earlier runs of it are
stale. Any 2xx reply means the server has the run.

Apache-2.0.
