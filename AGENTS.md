# espos-bathy-bridge

Chartplotter-to-Signal K bridge firmware on espOS. The board's microSD card
appears to a chartplotter as a second storage card over USB; the plotter
records bathymetry there (today a Raymarine Axiom's RealBathy files,
`realbathy/<id>.trl`), and the bridge uploads every new record to crowd-depth
on the boat's Signal K server under the device's own vessel context. The USB
export and the file follower are espOS's `espos_usb_storage` (the `espos/`
submodule); this repo picks the card pins, the watched files and where the
records go.

## Rules

- **Fixes to USB storage, the follower, WiFi, config, OTA or the web UI
  belong in espOS**, with a host test where the logic is testable. Merge there
  first, then bump the submodule here. A workaround here names the espOS issue
  and goes when the bump lands.
- **Never write to the card while it is exported.** The plotter owns the FAT;
  a write from the device corrupts it. espOS enforces this; nothing here may
  work around it.
- **Recorded data is uploaded, never sent as deltas.** signalk-server takes
  any delta as the vessel's current state, whatever its timestamp
  (`espos/docs/signalk.md`, "Another vessel"). Records are labelled with
  `espos_sk_vessel_context()`, never `vessels.self`: the bridge sits in a
  dinghy, and its soundings are not the mothership's.
- **Progress advances only on a 2xx.** The sink returns true only once the
  server has the records, so being out of range, a power cut or a server
  without the endpoint loses nothing.
- **Partition tables are fixed at the first USB flash.** OTA never rewrites
  them, so a change reaches only newly flashed boards.
- **Build through `scripts/build.sh`** (espOS's locked, nice'd wrapper), one
  build directory per target. Zero warnings.
- **Never commit boat configuration** (WiFi credentials, server addresses) or
  the signing key. Site config lives in NVS.

## Conventions

- Angular conventional commits and PR titles, `type(scope): subject`, subject
  lower case, at most 50 characters. PR titles become the release notes.
- Branch names `feat/…`, `fix/…`, `docs/…`; never commit to `main`.
- No AI attribution anywhere: no Co-Authored-By, no "Generated with".
- No version bumps or tags by hand; release-please cuts releases.
- Squash-merge every PR, the release PR included.
- Comments say why, not what.
