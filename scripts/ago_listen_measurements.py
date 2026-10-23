#!/usr/bin/env python3
"""Listen for AirGradient Go Offline BLE measurements until Ctrl+C.

Requirements:
    python3 -m pip install bleak

Usage:
    python3 scripts/ago_listen_measurements.py
    python3 scripts/ago_listen_measurements.py --address AA:BB:CC:DD:EE:FF

Prints one JSON object per matching scanner report. Measurements are raw,
invalid values are null, and received_at is the client's reception time.
No connection, pairing, or bonding is performed.
"""

from __future__ import annotations

import argparse
import asyncio
from datetime import datetime
import json
import logging
import struct
from typing import Any

logger = logging.getLogger("ago_measurements")

# Matches products/go/main/go_ble_advertisement.h.
SERVICE_UUID = "d2086101-9a63-4acc-8a7c-c07f607cb240"
VERSION = 1
PAYLOAD = struct.Struct("<BhBHHHhB")
INVALID_U8 = 0xFF
INVALID_U16 = 0xFFFF
INVALID_I16 = -32768


def decode_measurements(data: bytes) -> dict[str, int | float | None]:
    """Decode the 13-byte Service Data body (Bleak already removes the UUID)."""
    if len(data) != PAYLOAD.size:
        raise ValueError(f"expected {PAYLOAD.size} bytes, received {len(data)}")
    version, temp, humidity, pm25, co2, pressure, altitude, battery = PAYLOAD.unpack(data)
    if version != VERSION:
        raise ValueError(f"unsupported payload version {version}")
    return {
        "version": version,
        "temperature_c": None if temp == INVALID_I16 else temp / 100,
        "humidity_percent": None if humidity == INVALID_U8 else humidity,
        "pm25_ug_m3": None if pm25 == INVALID_U16 else pm25 / 10,
        "co2_ppm": None if co2 == INVALID_U16 else co2,
        "pressure_hpa": None if pressure == INVALID_U16 else pressure / 10,
        "barometric_altitude_m": None if altitude == INVALID_I16 else altitude,
        "battery_percent": None if battery == INVALID_U8 else battery,
    }


async def listen(address: str | None = None) -> None:
    from bleak import BleakScanner

    def on_detect(device: Any, advertisement: Any) -> None:
        if address and device.address.casefold() != address.casefold():
            return
        data = advertisement.service_data.get(SERVICE_UUID)
        if data is None:
            return
        try:
            readings = decode_measurements(data)
        except ValueError as exc:
            logger.warning("Ignoring packet from %s: %s", device.address, exc)
            return
        print(json.dumps({
            "received_at": datetime.now().astimezone().isoformat(timespec="milliseconds"),
            "device": device.address,
            "rssi_dbm": advertisement.rssi,
            **readings,
        }), flush=True)

    # Match Service Data in the callback: the broadcast has no local name or
    # separate service UUID list. Preserve repeated reports from the scanner.
    async with BleakScanner(detection_callback=on_detect):
        logger.info("Listening for Go measurements. Press Ctrl+C to stop.")
        await asyncio.Event().wait()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--address", help="Only print this device's Bluetooth address or macOS UUID")
    args = parser.parse_args()
    try:
        from bleak.exc import BleakError
    except ImportError:
        parser.exit(1, "Missing dependency: install with python3 -m pip install bleak\n")

    logging.basicConfig(level=logging.INFO, format="%(message)s")
    logging.getLogger("bleak").setLevel(logging.WARNING)
    try:
        asyncio.run(listen(args.address))
    except KeyboardInterrupt:
        logger.info("Stopped.")
    except (BleakError, OSError) as exc:
        logger.error("Bluetooth error: %s", exc)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
