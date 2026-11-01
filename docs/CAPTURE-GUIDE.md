# Capturing MX400 SysEx (for the missing pieces)

Why: the manual says the MX400 transmits and recognizes SysEx for the Lexicon editor, but the format is not published. Until it is known, this plugin never builds or sends MX400 SysEx. Captures from your own unit (and from MX-Edit talking to it) are the only safe source.

Status of everything here: **unverified** until you test it on your unit.

## What to capture (one step at a time)
1. Back up first: in MX-Edit, save all your programs to a file. Keep it.
2. Connect the MX400 by USB or MIDI. Find its port: `amidi -l` (Linux).
3. Start recording: `sh tools/capture_sysex.sh hw:1,0,0 baseline` (replace the port with yours). Stop with Ctrl+C. Result: `baseline.syx`.
4. On the unit, change exactly one thing (for example one program's Pre Delay) and capture again under a new name.
5. Compare: the bytes that differ show where that value lives. In the plugin, Library > SysEx capture can also record incoming dumps from the MX400 input.
6. Repeat for: effect type, routing, bypass, program name, one knob at min, middle and max.

## Rules
- Do not send anything back to the unit until a dump of the same kind has been understood.
- Never send firmware, bootloader or factory-reset data.
- Keep captures for yourself; do not publish Lexicon files.

## What we still need to learn
- The effect type list and routing list values (CC 12, 19, 39 and CC 28).
- The bypass on and off values for CC 13, 20, 40.
- The SysEx dump request and dump layout, any checksum, and any packing of 8-bit data.
