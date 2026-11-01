**MX400 Controller v0.1 (test build, early)**

A free, open source MIDI controller plugin for the Lexicon MX400 hardware unit. It sends CC and program change; it does not process audio. It collects nothing and works offline.

One zip: installers for Windows (.exe), macOS (.pkg), Linux (Flatpak extension and a plain .vst3), the PDF manual, INSTALL.txt and SHA-256 sums.

**Read this first**
- Not tested on a real MX400 yet. All CC numbers come from the printed manual ("official only").
- Effect type and routing values are raw 0 to 127 (the manual does not print the lists).
- Nothing is sent when a project opens. Use SEND ALL.
- Not built yet: surround trims, ducker, direct I/O, SysEx store and restore (the MX-Edit SysEx format is undocumented). SysEx can be captured and checked only.
- Installers are unsigned and not yet tested on real machines.
