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
from typing import Any

logger = logging.getLogger("ago_measurements")

# Matches products/go/main/go_ble_advertisement.h.
SERVICE_UUID = "992f1fe6-4519-4d4f-8ca7-546d36126ee8"
# 9-byte bit-packed manufacturer data payload, least significant bit first.
COMPANY_ID = 0xFFFF  # Bluetooth SIG test ID until an assigned ID replaces it.
PAYLOAD_SIZE = 9
VERSION = 1
FIELDS = {  # name: (first bit, width)
    "version": (0, 3),
    "temperature": (3, 11),
    "humidity": (14, 7),
    "pm25": (21, 14),
    "co2": (35, 14),
    "pressure": (49, 13),
    "battery": (62, 7),
}
SEA_LEVEL_PRESSURE_HPA = 1013.25


def _bits(data: bytes, first: int, width: int) -> int:
    return sum(((data[(first + i) // 8] >> ((first + i) % 8)) & 1) << i for i in range(width))


def decode_measurements(data: bytes) -> dict[str, int | float | None]:
    """Decode the 9-byte payload (Bleak already removes the company ID)."""
    if len(data) != PAYLOAD_SIZE:
        raise ValueError(f"expected {PAYLOAD_SIZE} bytes, received {len(data)}")
    raw = {name: _bits(data, *field) for name, field in FIELDS.items()}
    if raw["version"] != VERSION:
        raise ValueError(f"unsupported payload version {raw['version']}")

    def value(name: str) -> int | None:
        return None if raw[name] == (1 << FIELDS[name][1]) - 1 else raw[name]

    temperature, pm25, pressure = value("temperature"), value("pm25"), value("pressure")
    pressure_hpa = None if pressure is None else round(pressure / 10 + 300, 1)
    return {
        "version": VERSION,
        "temperature_c": None if temperature is None else round(temperature / 10 - 40, 1),
        "humidity_percent": value("humidity"),
        "pm25_ug_m3": None if pm25 is None else pm25 / 10,
        "co2_ppm": value("co2"),
        "pressure_hpa": pressure_hpa,
        "barometric_altitude_m": None if pressure_hpa is None else round(
            44330 * (1 - (pressure_hpa / SEA_LEVEL_PRESSURE_HPA) ** 0.1903)),
        "battery_percent": value("battery"),
    }


async def listen(address: str | None = None) -> None:
    from bleak import BleakScanner

    def on_detect(device: Any, advertisement: Any) -> None:
        if address and device.address.casefold() != address.casefold():
            return
        if SERVICE_UUID not in (uuid.lower() for uuid in advertisement.service_uuids):
            return
        data = advertisement.manufacturer_data.get(COMPANY_ID)
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

    # Discover the advertised service; verify the company ID in the callback.
    async with BleakScanner(detection_callback=on_detect, service_uuids=[SERVICE_UUID]):
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
