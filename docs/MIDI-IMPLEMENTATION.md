# Lexicon MX400: MIDI implementation

Source: MX400 User Guide (Lexicon part 18-0446-A, 2006), pp. 33-34 (MIDI system menus), p. 45 (MIDI Implementation Chart) and pp. 46-49 (MIDI CC maps). The CC maps are images in the PDF; they were read by eye from rendered pages. Marked **(check)** where the print is ambiguous.

## Basics (chart, p. 45)

| Item | Value |
|---|---|
| Receive only | Nothing is transmitted (DIN MIDI IN and THRU, plus USB) |
| Basic channel | Default 1; set 1 to 16, Omni or Off. Two receive channels: A and B |
| Channel B | Only used in Dual Stereo (controls the bottom program, Effects 3 and 4) |
| Mode | Mode 2 (Omni on, mono) |
| Control change | 1 to 127, see maps below |
| Program change | 0 to 98 = programs 1 to 99 |
| Bank select | CC 32: 0 = Factory, 1 = User (chart lists CC 32 only) |
| System exclusive | Yes, Lexicon editor only. SysEx follows the MIDI channel; if the channel is Off, it is received on all channels |
| MIDI clock | Received, sets delay tempo (needs System > MIDI Clock Sync On; front Tempo button is then disabled) |
| Notes, velocity, aftertouch, pitch bend, song position/select, start/stop | Not recognized |

System menu switches that gate MIDI: MIDI Channel A, MIDI Channel B, MIDI Clock Sync, MIDI Program Receive, MIDI CC Receive. Program change still works with CC Receive off, and CC still works with Program Receive off. (The manual's CC Receive paragraph mistakenly says "MIDI Program Receive"; the filter is for CCs.)

Programs: 99 Factory + 99 User per configuration (Surround, Stereo, Dual Stereo). Files from MX-Edit: `.mx400dsp` (dual stereo), `.mx400stp` (stereo), `.mx400srp` (surround); backup `.m2a` on Windows, `.syx` on Mac. The editor/librarian uses USB or MIDI.

## CC map: how to read it

Each printed CC cell is "A/B": the first CC drives Effect 1, the second Effect 2 (in Dual Stereo: Effects 1 and 2 are Program A on channel A; Effects 3 and 4 are Program B on channel B). The first four parameter columns are the same row for every effect type; their meaning changes with the effect loaded. Every value is 0 to 127 (range not printed).

### Common per effect slot

| Function | Dual Stereo map (p. 46) | Stereo / Routing 5 map (p. 47) |
|---|---|---|
| Mix | 26 / 27 | 27 / 54 |
| Bypass | 13 / 20 | 13 / 40 |
| Tempo (delay effects) | 18 / 25 | 26 / 53 |
| Effect Type | 12 / 19 | 12 / 39 |
| Routing | 28 | 28 |
| Assign A, B, C (knob assigns) | 26/29, 27/30, 28/31 | 29/55, 30/56, 31/57 |

Note: in the Dual Stereo map the printed CCs 26, 27, 28 appear both as Mix/Routing and as Assign A/B/C; Korg-style overlap is not explained in the manual **(check on hardware)**.

### Dual Stereo effects (p. 46): parameter CCs 14/21, 15/22, 16/23, 17/24

| Effect | 14/21 | 15/22 | 16/23 | 17/24 |
|---|---|---|---|---|
| Small/Large Hall, Small/Large Plate, Chamber, Room, Studio, Ambience, Vocal Hall, Vocal Plate, Drum Hall, Drum Plate, Arena | Pre Delay | Decay / Mid RT | Liveliness | none |
| Gated, Reverse | Pre Delay | Decay | Diffusion | none |
| Spring | Pre Delay | Decay | Boing | none |
| Studio Delay, Mono Delay | Tempo | Delay Time | Feedback | Ducker Threshold |
| Tape Delay | Tempo | Delay Time | Feedback | Smear |
| Pong Delay | Tempo | Delay Time | Feedback | Tap Ratio **(check, as printed)** |
| Modulated Delay | Tempo | Delay Time | Feedback | Modulation Depth |
| Reverse Delay | Tempo | Delay Time | Feedback | Smear |
| Chorus | Speed | Depth | Voices | none |
| Flanger, Phaser | Speed | Depth | Regen | none |
| Tremolo/Pan, Vibrato | Speed | Depth | Phase | none |
| Rotary | Speed | Doppler | Stereo Spread | none |
| Pitch Shift | Shift Amount | Down/Up Shift | High Pass Filter | none |
| Detune | Shift Amount | Shift Amount | Stereo Spread | none |
| De Esser | Ratio | Frequency | Bandwidth | none |

### Stereo and Dual Stereo Routing 5 (pp. 47 and 48)

Page 47: parameter CCs 14/41, 15/42, 16/43, 17/44.

| Effect | 14/41 | 15/42 | 16/43 | 17/44 |
|---|---|---|---|---|
| Hall, Plate, Chamber Reverb | Pre Delay | Mid RT | Size | Shape |
| Room Reverb | Pre Delay | Mid RT | Diffusion | Shape |
| Ambience Reverb | Pre Delay | Size | Diffusion | Shape |
| Gated / Reverse Reverb | Pre Delay | Decay | Diffusion | none |
| Spring Reverb | Pre Delay | Decay | Boing | none |
| Stereo Studio, Mono, Pong, 2-Tap, Modulated, Tape/Reverse Delay | Tempo | Delay Time | Feedback | High Pass Filter (Stereo Studio, Mono, Pong, Modulated); Delay Type (2-Tap); Pan (Tape/Reverse) |
| Chorus | Speed | Depth | Voices | Pre Delay |
| Flanger, Phaser | Speed | Depth | Regen | Waveform |
| Tremolo/Panner | Speed | Depth | Phase | Waveform |
| Rotary | Speed | Stereo Spread | Drive | Rotor Minimum |
| Vibrato | Speed | Depth | Phase | Waveform |
| Pitch Shift / Detune | Shift 1 | Delay 1 | Feedback 1 | Shift 2 |
| DeEsser | Ratio | Frequency | Bandwidth | none |
| Compressor | Ratio | Threshold | Gain | Attack |

Page 48: extra parameters, CCs 18/45 to 25/52.

| Effect | 18/45 | 19/46 | 20/47 | 21/48 | 22/49 | 23/50 | 24/51 | 25/52 |
|---|---|---|---|---|---|---|---|---|
| Hall Reverbs | Spread | RT High Cut | High Cut | Bass Boost Freq | Bass Boost | Diffusion | ER Time | ER Level |
| Plate Reverbs | Spread | RT High Cut | High Cut | Bass Boost Freq | Bass Boost | Diffusion | Feedback Delay | Feedback Level |
| Chamber Reverb | Spread | RT High Cut | High Cut | Bass Boost Freq | Bass Boost | Diffusion | ER Time | ER Level |
| Room Reverb | Spread | RT High Cut | High Cut | ER Time | ER Level | | | |
| Ambience Reverb | Spread | RT High Cut | High Cut | | | | | |
| Gated/Reverse, Spring | none | | | | | | | |
| Stereo Studio Delay | Low Pass Filter | Ducker Level | Ducker Thresh | | | | | |
| Mono Delay | Low Pass Filter | Ducker Level | Ducker Thresh | Pan | | | | |
| Pong Delay | Low Pass Filter | Ducker Level | Ducker Thresh | Tap Ratio | Tap Swap | | | |
| 2-Tap Delay | Tap 1 | Level 1 | Pan 1 | Tap 2 | Level 2 | Pan 2 | | |
| Modulated Delay | Low Pass Filter | Mod Depth | | | | | | |
| Tape / Reverse Delay | Smear | | | | | | | |
| Chorus | Waveform | Diffusion | High Pass Filter | | | | | |
| Flanger | PreDelay | Phase | | | | | | |
| Phaser | Phase Stages | | | | | | | |
| Rotary | Rotor Maximum | Horn Minimum | Horn Maximum | Doppler | | | | |
| Pitch Shift / Detune | Delay 2 | Feedback 2 | Pan 1 | Pan 2 | | | | |
| Compressor | Release | Knee | | | | | | |

Tremolo/Panner, Vibrato, DeEsser: none on this page.

### Surround Hall (p. 49)

| Function | CC |
|---|---|
| Bypass | 13 |
| Mix Front | 26 |
| Mix Rear | 27 |
| Routing | 28 |
| Pre Delay | 14 |
| Mid RT | 15 |
| Size | 16 |
| Spread | 18 |
| RT High Cut | 19 |
| High Cut | 20 |
| Bass Boost Freq | 21 |
| Bass Boost | 22 |
| Diffusion | 23 |
| ER Time | 24 |
| ER Level | 25 |
| Assign A, B, C | 29, 30, 31 |

## Open items

1. CC value ranges and enum encodings (Effect Type list, Routing 1 to 5, Delay Type, Waveform, Bypass on/off thresholds) are not printed.
2. The overlap of CCs 26 to 28 in the Dual Stereo map, and the Pong Delay / Tap Ratio row.
3. SysEx format for MX-Edit (USB two-way untested; capture planned).
