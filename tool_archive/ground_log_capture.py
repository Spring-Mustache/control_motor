#!/usr/bin/env python3
"""Capture an autonomous wheel-control log from STM32 USART3 / ST-LINK VCP.

Usage:
    python ground_log_capture.py COM5 analog_bench_01.csv

Workflow:
    1. Finish the test and stop the vehicle.
    2. Connect ST-LINK USB to the PC.
    3. Start this script.
    4. Press the NUCLEO USER button once to request CSV dump.
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("Missing pyserial. Install once with: python -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)

BEGIN = "#WHEEL_ANALOG_CSV_BEGIN"
END = "#WHEEL_ANALOG_CSV_END"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("port", help="Serial port, e.g. COM5")
    ap.add_argument("output", help="Output CSV filename")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--timeout", type=float, default=180.0,
                    help="Seconds to wait for BEGIN marker (default: 180)")
    args = ap.parse_args()

    print(f"Opening {args.port} @ {args.baud}...")
    with serial.Serial(args.port, args.baud, timeout=1.0) as ser:
        ser.reset_input_buffer()
        print("Ready. Press the USER button on the NUCLEO to dump the saved log.")

        deadline = time.monotonic() + args.timeout
        lines = []
        started = False

        while True:
            if not started and time.monotonic() > deadline:
                print("Timeout: no BEGIN marker received.", file=sys.stderr)
                return 1

            raw = ser.readline()
            if not raw:
                continue

            line = raw.decode("ascii", errors="replace").rstrip("\r\n")

            if not started:
                if line == BEGIN:
                    started = True
                    lines.append(line)
                    print("Receiving log...")
                continue

            lines.append(line)
            if line == END:
                break

    with open(args.output, "w", newline="", encoding="ascii", errors="replace") as f:
        for line in lines:
            f.write(line + "\n")

    data_rows = sum(1 for x in lines if x and not x.startswith("#") and not x.startswith("time_ms"))
    print(f"Saved {data_rows} samples -> {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
