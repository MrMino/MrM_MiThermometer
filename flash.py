#!/usr/bin/env python
"""
Telink flasher for Mi thermoemeter firmware.

Requires: pyserial

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""

from __future__ import annotations

import argparse
import functools
import struct
import sys
import time
from pathlib import Path

import serial


def sleep_ms(ms: int) -> None:
    time.sleep(ms / 1000.0)


# Note: writing byte X followed by a byte Y into 0x0C is the same
# as writing X to 0x0C and Y to 0x0D
SPI_DATA_REG = 0x0C
SPI_CTRL_CS = 0x0D

SPI_CS_ACTIVE = 0x00
SPI_CS_INACTIVE = 0x01

SPI_OP_WRSR = 0x01
SPI_OP_PP = 0x02
SPI_OP_WREN = 0x06
SPI_OP_RDPD = 0xAB
SPI_OP_SECTOR_ERASE = 0x20

SWS_REG_SWIRE_CLOCK_DIV = 0xB2
SWS_REG_SWIRE_ID = 0xB3
SWS_MASK_FIFO_ENABLE = 0b1000_0000
SWS_MASK_FIFO_DISABLE = 0b0000_0000

SWS_CPU_STATE = 0x0602
SWS_CPU_STOP_CMD = 0x05

TLK_REG_PWDNEN = 0x6F
TLK_REG_PWDNEN_RST_ALL = 0b0010_0000


class SwsUart(serial.Serial):
    """UART that masquarades as SWS"""

    SWS_BIT_HIGH = 0x80
    SWS_BIT_LOW = 0xFE

    MASKS = (0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01)

    @classmethod
    def sws_encode_data(cls, addr: int, data: bytes) -> bytes:
        """Encode SWS data: each byte -> 10-bit SWS word -> 10 UART bytes.

        This models the UART TX waveform into something that SWS recognizes.
        """

        START_CMD = 0x5A
        STOP_CMD = 0xFF

        header = bytes(
            [
                (addr >> 16) & 0xFF,
                (addr >> 8) & 0xFF,
                addr & 0xFF,
                0x00,
            ]
        )

        to_encode = bytes([START_CMD]) + header + data + bytes([STOP_CMD])

        encoded = bytearray((len(data) + 6) * 10)
        for offset, byte in enumerate(to_encode):
            sws_byte = cls.sws_encode_byte(byte)
            encoded[offset * 10 : (offset + 1) * 10] = sws_byte

        # Flip the CMD bit of the first and last byte (START_CMD and STOP_CMD)
        encoded[0] = cls.SWS_BIT_HIGH
        encoded[-10] = cls.SWS_BIT_HIGH

        return bytes(encoded)

    @classmethod
    @functools.cache
    def sws_encode_byte(cls, byte: int) -> bytes:
        assert 0 <= byte <= 255

        return bytes(
            [cls.SWS_BIT_LOW]
            + [
                cls.SWS_BIT_HIGH if byte & mask else cls.SWS_BIT_LOW
                for mask in cls.MASKS
            ]
            + [cls.SWS_BIT_LOW]
        )

    def write_sws(self, address: int, data: bytes) -> None:
        to_write = self.sws_encode_data(address, data)
        self.write(to_write)
        self.flush()


class TelinkSws:
    def __init__(self, sws: SwsUart) -> None:
        self.sws = sws

    def flash_byte_cmd(self, cmd: int) -> None:
        assert 0 <= cmd <= 255
        self.sws.write_sws(SPI_CTRL_CS, bytes([SPI_CS_ACTIVE]))
        self.sws.write_sws(SPI_DATA_REG, bytes([cmd, SPI_CS_INACTIVE]))

    def flash_write_enable(self) -> None:
        self.flash_byte_cmd(SPI_OP_WREN)

    def flash_wake_up(self) -> None:
        self.flash_byte_cmd(SPI_OP_RDPD)

    def flash_write_status_clear(self) -> None:
        self.flash_write_enable()
        self.sws.write_sws(SPI_CTRL_CS, bytes([SPI_CS_ACTIVE]))
        self.sws.write_sws(SPI_DATA_REG, bytes([0x01]))
        self.sws.write_sws(SPI_DATA_REG, bytes([0x00, SPI_CS_INACTIVE]))
        sleep_ms(3)

    def write_fifo(self, addr: int, data: bytes) -> None:
        self.sws.write_sws(SWS_REG_SWIRE_ID, bytes([SWS_MASK_FIFO_ENABLE]))
        self.sws.write_sws(addr, data)
        self.sws.write_sws(SWS_REG_SWIRE_ID, bytes([SWS_MASK_FIFO_DISABLE]))

    def sector_erase(self, addr: int) -> None:
        POST_ERASE_DALAY_MS = 250

        self.flash_write_enable()
        self.sws.write_sws(SPI_CTRL_CS, bytes([SPI_CS_ACTIVE]))
        self.sws.write_sws(SPI_DATA_REG, bytes([SPI_OP_SECTOR_ERASE]))
        self.sws.write_sws(SPI_DATA_REG, bytes([(addr >> 16) & 0xFF]))
        self.sws.write_sws(SPI_DATA_REG, bytes([(addr >> 8) & 0xFF]))
        self.sws.write_sws(SPI_DATA_REG, bytes([addr & 0xFF, SPI_CS_INACTIVE]))
        sleep_ms(POST_ERASE_DALAY_MS)

    def bulk_write_flash(self, addr: int, data: bytes) -> None:
        POST_FLASH_DELAY_MS = 10

        self.flash_write_enable()
        self.sws.write_sws(SPI_CTRL_CS, bytes([SPI_CS_ACTIVE]))

        to_write = bytearray(4 + len(data))
        to_write[0] = SPI_OP_PP
        to_write[1] = (addr >> 16) & 0xFF
        to_write[2] = (addr >> 8) & 0xFF
        to_write[3] = addr & 0xFF
        to_write[4:] = data

        self.write_fifo(SPI_DATA_REG, bytes(to_write))
        self.sws.write_sws(SPI_CTRL_CS, bytes([SPI_CS_INACTIVE]))
        sleep_ms(POST_FLASH_DELAY_MS)

    def chip_reset(self) -> None:
        self.sws.write_sws(TLK_REG_PWDNEN, bytes([TLK_REG_PWDNEN_RST_ALL]))

    def force_cpu_state(self, tim_ms: int) -> None:
        self.chip_reset()

        end_t = time.monotonic() + (tim_ms / 1000.0)
        while time.monotonic() < end_t:
            self.sws.write_sws(SWS_CPU_STATE, bytes([SWS_CPU_STOP_CMD]))
            sleep_ms(1)

    def set_sws_clk_speed(self):
        SWS_CLK_DIV = 55
        self.sws.write_sws(SWS_REG_SWIRE_CLOCK_DIV, bytes([SWS_CLK_DIV]))
        self.sws.write_sws(SWS_CPU_STATE, bytes([SWS_CPU_STOP_CMD]))


def flash_fw(port: SwsUart, firmware: bytes, activate_ms: int) -> None:
    CHUNK_SIZE = 256

    flasher = TelinkSws(port)

    time_start = time.monotonic()
    print("Bombarding chip with CPU halt")
    flasher.force_cpu_state(activate_ms)
    print("Setting SWS clk speed")
    flasher.set_sws_clk_speed()
    print("Waking up flash")
    flasher.flash_wake_up()

    print("Clearing status register")
    if firmware:
        flasher.flash_write_status_clear()
        flasher.flash_write_status_clear()

    print("Flashing!")
    print(f"Writing {len(firmware)} bytes into flash")
    for offset in range(0, len(firmware), CHUNK_SIZE):
        if (offset % 0x1000) == 0:
            print(f"Erasing sector: 0x{offset:06X}")
            flasher.sector_erase(offset)

        chunk = firmware[offset : offset + CHUNK_SIZE]
        print(f"Writing {len(chunk)} bytes at 0x{offset:06X}")
        flasher.bulk_write_flash(offset, chunk)

    print("Resetting the chip")
    flasher.chip_reset()

    time_done = time.monotonic() - time_start
    print(f"Done in {time_done:.3f} sec.")


def check_magic(firmware: bytes):
    TLNK_MAGIC_OFS = 8
    TLNK_MAGIC = (b"K", b"N", b"L", b"T")  # TLNK in little endian

    magic = struct.unpack_from("<cccc", firmware, TLNK_MAGIC_OFS)
    if magic != TLNK_MAGIC:
        print("Selected file is not a Telink firmware .bin (missing TLNK magic)")
        sys.exit(2)


def main(argv: list[str]) -> None:
    DEFAULT_PORT = "/dev/ttyUSB0"
    DEFAULT_BAUD = 460800
    DEFAULT_ACTIVATE_TIME = 5000
    cli = argparse.ArgumentParser(description="PySerial rewrite of WebSerial flasher")
    cli.add_argument(
        "--port",
        default=DEFAULT_PORT,
        help=f"Serial port (default: {DEFAULT_PORT})",
    )
    cli.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Baud rate (default: {DEFAULT_BAUD})",
    )
    cli.add_argument(
        "--activate-ms",
        type=int,
        default=DEFAULT_ACTIVATE_TIME,
        help=f"Activate time in ms (default: {DEFAULT_ACTIVATE_TIME})",
    )
    cli.add_argument(
        "--file",
        required=True,
        type=Path,
        help="Path to the firmware file",
    )

    args = cli.parse_args(argv)

    sc = SwsUart(port=args.port, baudrate=args.baud)

    try:
        firmware = args.file.read_bytes()
        print(f"Loaded firmware from {args.file} ({len(firmware)} bytes)")
        check_magic(firmware)
        flash_fw(sc, firmware, args.activate_ms)

    finally:
        sc.close()


if __name__ == "__main__":
    main(sys.argv[1:])
