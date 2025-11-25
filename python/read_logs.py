#!/usr/bin/env python3
"""
read_logs.py – Simple serial logger for the Arduino sketch.

Usage:
    python read_logs.py               # tries default COM port & baud rate
    python read_logs.py -p COM3 -b 115200
    python read_logs.py --help

The script continuously reads lines from the serial port and prints them
to stdout, prefixed with a timestamp.  It stops gracefully on Ctrl‑C.
"""

import argparse
import sys
import time

try:
    import serial  # pyserial
except ImportError:
    sys.stderr.write(
        "Error: pyserial is not installed. Install it with:\n"
        "    pip install pyserial\n"
    )
    sys.exit(1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Read Arduino serial logs.")
    parser.add_argument(
        "-p",
        "--port",
        default=None,
        help="Serial port (e.g. COM3 on Windows, /dev/ttyUSB0 on Linux). "
             "If omitted, the script will try common Arduino ports.",
    )
    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=9600,
        help="Baud rate (default: 9600).",
    )
    parser.add_argument(
        "-t",
        "--timeout",
        type=float,
        default=1.0,
        help="Read timeout in seconds (default: 1.0).",
    )
    return parser.parse_args()


def guess_port() -> str | None:
    """Return a likely Arduino port on Windows, or None if we can't guess."""
    import serial.tools.list_ports

    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "Arduino" in p.description or "USB" in p.description:
            return p.device
    return ports[0].device if ports else None


def main() -> None:
    args = parse_args()

    port = args.port or guess_port()
    if not port:
        sys.stderr.write(
            "Could not auto‑detect a serial port. Please specify one with -p.\n"
        )
        sys.exit(1)

    try:
        ser = serial.Serial(port, args.baud, timeout=args.timeout)
    except serial.SerialException as exc:
        sys.stderr.write(f"Failed to open {port}: {exc}\n")
        sys.exit(1)

    print(f"Listening on {port} @ {args.baud} baud (Ctrl‑C to quit)…")
    try:
        while True:
            line_bytes = ser.readline()
            if not line_bytes:
                continue
            try:
                line = line_bytes.decode(errors="replace").rstrip()
            except UnicodeDecodeError:
                line = repr(line_bytes)
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{timestamp}] {line}")
    except KeyboardInterrupt:
        print("\nInterrupted – closing serial port.")
    finally:
        ser.close()


if __name__ == "__main__":
    main()
