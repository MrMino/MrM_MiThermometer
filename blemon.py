#!/usr/bin/env python
"""
BLE advertisements monitor.

Requires: bleak
"""
import asyncio
import datetime as dt
import sys

from bleak import BleakScanner

enable_color = sys.stdout.isatty()

if enable_color:
    FG = [
        (15, 197, 1),
        (49, 188, 188),
        (85, 255, 27),
        (91, 229, 182),
        (116, 171, 255),
        (119, 217, 254),
        (131, 184, 111),
        (190, 254, 179),
        (196, 197, 8),
        (202, 163, 253),
        (228, 148, 175),
        (250, 246, 27),
        (253, 211, 140),
        (254, 121, 251),
        (255, 144, 51),
        (255, 195, 253),
    ]

    BG = [
        (0, 0, 16),
        (3, 2, 102),
        (3, 29, 1),
        (4, 39, 62),
        (5, 62, 112),
        (8, 73, 1),
        (9, 5, 51),
        (29, 0, 1),
        (31, 1, 210),
        (50, 2, 52),
        (53, 17, 144),
        (67, 40, 19),
        (90, 47, 84),
        (91, 0, 38),
        (106, 4, 137),
        (127, 17, 6),
    ]

    def fmt_byte(byte: int) -> str:
        hi = (byte >> 4) & 0xF
        lo = byte & 0xF
        fr, fg, fb = FG[hi]
        br, bg, bb = BG[lo]
        return (
            f"\x1b[38;2;{fr};{fg};{fb}m"
            f"\x1b[48;2;{br};{bg};{bb}m"
            f"{byte:02X}\x1b[0m"
        )

else:

    def fmt_byte(byte: int) -> str:
        return f"{byte:02X}"


def hexdump(data: bytes) -> str:
    parts = []
    for byte in data:
        parts.append(fmt_byte(byte))
    return "".join(parts)


if len(sys.argv) == 1:

    def mac_filter(mac: str) -> bool:
        return mac.startswith("A4:C1:38:")

else:

    def mac_filter(mac: str) -> bool:
        return mac in {arg.strip().upper() for arg in sys.argv[1:]}


async def main():
    def adv_detected(device, adv):
        mac = device.address.strip().upper()
        if not mac_filter(mac):
            return

        # Service data only (uuid -> bytes)
        for uuid, blob in (adv.service_data or {}).items():
            ts = dt.datetime.now().strftime("%H:%M:%S")
            print(f"[{ts}] [{len(blob)}] {mac} {uuid} {hexdump(blob)}", flush=True)

    async with BleakScanner(detection_callback=adv_detected):
        await asyncio.Event().wait()


asyncio.run(main())
