# Roadmap: MX400 controller

## Done (v0.1)
- [x] CC maps for Stereo, Dual Stereo (incl. routing 5) and Surround, from the manual
- [x] Program change with bank select, channels A and B, direct port or host MIDI out
- [x] Safe sending: nothing on load, touched-only Send all, manual mode, rate limit
- [x] Library page: notes, favourites, SysEx capture and file check
- [x] Surround page (controls only), settings page, resizable editor
- [x] Installers, uninstallers and PDF manual (CI)

## Needs the real unit
- [ ] Test on hardware: every CC, channels, program change timing
- [ ] Learn effect type, routing and bypass values (capture or test)
- [ ] Capture MX-Edit SysEx (see docs/CAPTURE-GUIDE.md), then store, backup and restore
- [ ] Calibrate the graphs

## Not built yet
- [ ] Surround channel trims, depth, return ducker, direct I/O, latency ping (need an audio engine)
- [ ] Manual tour with numbered badges per control
- [ ] Test installers on real Windows, macOS and Flatpak setups

## Every cat plugin
- Draw through `catui::RenderGate` so meters and animations stop when the window is not in front (done in the editor)
- Settings page with MIDI filters, UI size reset to 100% (done)
