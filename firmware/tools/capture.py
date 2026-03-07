#!/usr/bin/env python3
"""
Capture conditioned entropy output from the REB-01 over serial.

Usage:
    python3 tools/capture.py [port] [output_file]

Defaults:
    port:        /dev/tty.usbmodem14303
    output_file: entropy.bin
"""

import serial
import sys
import re

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/tty.usbmodem14303"
outfile = sys.argv[2] if len(sys.argv) > 2 else "entropy.bin"

ser = serial.Serial(port, 115200, timeout=1)
pat = re.compile(r"([0-9a-f]{64})\s*$")

print(f"Listening on {port}, writing to {outfile}")
print("Press Ctrl-C to stop.\n")

with open(outfile, "ab") as f:
    try:
        while True:
            line = ser.readline().decode("utf-8", errors="ignore")
            if not line:
                continue
            m = pat.search(line)
            if m:
                raw = bytes.fromhex(m.group(1))
                f.write(raw)
                f.flush()
            sys.stdout.write(line)
            sys.stdout.flush()
    except KeyboardInterrupt:
        size = f.tell()
        print(f"\n\nDone. {size} bytes ({size // 32} samples) written to {outfile}")
