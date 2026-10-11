
# Synthesizer

This is a simple polyphonic audio synthesizer
written in YASCL. It reads a MIDI command stream
from stdin and produces as steady stream of 
16-bit PCM samples at 44.1kHz which it writes
to stdout.
The `adapt/` directory contains
adaptors to hook input and output of the synth
up to various devices.

## Details
It operates on one thread, polling for input,
and locking to the sample rate by computing
frame times and `clock_nanosleep`ing.
Active notes are stored as a linked list
of nodes which as synthesized and mixed
in one pass by `Synth::Sample`.
The supports sine and square wave
oscillation with frequency modulation.

