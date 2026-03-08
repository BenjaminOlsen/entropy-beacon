#!/usr/bin/env python3
"""
Capture conditioned entropy output from the REB-01 over serial.

Usage:
    python3 tools/capture.py [--server URL] [--port PORT] [--output FILE]

Defaults:
    port:        /dev/tty.usbmodem14303
    output_file: entropy.bin
    server:      (none — set to e.g. http://localhost:8080 to POST to server)
"""

import argparse
import json
import re
import sys
import urllib.request
import urllib.error

import serial


def post_to_server(server_url, entropy_hex, sig_hex):
    url = server_url.rstrip("/") + "/api/ingest"
    data = json.dumps({"entropy": entropy_hex, "signature": sig_hex}).encode()
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
    try:
        resp = urllib.request.urlopen(req)
        return resp.status
    except urllib.error.HTTPError as e:
        print(f"  [server] HTTP {e.code}: {e.read().decode().strip()}", file=sys.stderr)
        return e.code


def main():
    parser = argparse.ArgumentParser(description="Capture entropy from REB-01")
    parser.add_argument("--port", default="/dev/tty.usbmodem14303", help="serial port")
    parser.add_argument("--output", default="entropy.bin", help="output binary file")
    parser.add_argument("--server", default=None, help="server URL (e.g. http://localhost:8080)")
    args = parser.parse_args()

    ser = serial.Serial(args.port, 115200, timeout=1)
    entropy_pat = re.compile(r"^(?:  >> \[#\d+\]: )?([0-9a-f]{64})\s*$")
    sig_pat = re.compile(r"^\s*sig:\s*([0-9a-f]{128})\s*$")

    print(f"Listening on {args.port}, writing to {args.output}")
    if args.server:
        print(f"POSTing to {args.server}")
    print("Press Ctrl-C to stop.\n")

    pending_entropy = None

    with open(args.output, "ab") as f:
        try:
            while True:
                line = ser.readline().decode("utf-8", errors="ignore")
                if not line:
                    continue

                sys.stdout.write(line)
                sys.stdout.flush()

                m = entropy_pat.search(line)
                if m:
                    pending_entropy = m.group(1)
                    raw = bytes.fromhex(pending_entropy)
                    f.write(raw)
                    f.flush()
                    continue

                m = sig_pat.search(line)
                if m and pending_entropy:
                    sig_hex = m.group(1)
                    if args.server:
                        status = post_to_server(args.server, pending_entropy, sig_hex)
                        if status == 200:
                            sys.stdout.write("  [server] ok\r\n")
                    pending_entropy = None

        except KeyboardInterrupt:
            size = f.tell()
            print(f"\n\nDone. {size} bytes ({size // 32} samples) written to {args.output}")


if __name__ == "__main__":
    main()
