#!/bin/sh
# Records raw MIDI from a port into a .syx file (Linux, needs amidi from alsa-utils).
# Usage: sh tools/capture_sysex.sh <port, see amidi -l> <name>   Stop with Ctrl+C.
[ $# -eq 2 ] || { echo "Usage: $0 <port> <name>"; exit 1; }
echo "Recording from $1 to $2.syx. Press Ctrl+C to stop."
exec amidi -p "$1" -r "$2.syx"
