#!/usr/bin/python3
import mido
import sys

tracks = mido.MidiFile(sys.argv[1])

for x in tracks.play():
	packet = bytes(x.bytes())
	sys.stdout.buffer.write(packet)
	sys.stdout.buffer.flush()
	print(x, file=sys.stderr)





