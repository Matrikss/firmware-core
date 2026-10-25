"""Wire-format and scanner tests; no Bluetooth hardware or Bleak required."""

import asyncio
import importlib.util
import json
from pathlib import Path
from types import SimpleNamespace
import unittest
from unittest.mock import AsyncMock, Mock, patch


spec = importlib.util.spec_from_file_location(
    "ago_listen_measurements", Path(__file__).resolve().parents[1] / "ago_listen_measurements.py"
)
listener = importlib.util.module_from_spec(spec)
spec.loader.exec_module(listener)

# Same golden wire bytes as the firmware encoder tests, without the company ID.
VALID = bytes.fromhex("c1d28c0c800cbaf712")
INVALID = bytes.fromhex("f9ffffffffffffff1f")


class DecodeTests(unittest.TestCase):
    def test_firmware_golden_packet(self):
        self.assertEqual(listener.decode_measurements(VALID), {
            "version": 1,
            "temperature_c": 20.0,
            "humidity_percent": 51,
            "pm25_ug_m3": 10.0,
            "co2_ppm": 400,
            "pressure_hpa": 1013.3,
            "barometric_altitude_m": 0,
            "battery_percent": 75,
        })

    def test_invalid_fields_are_null(self):
        decoded = listener.decode_measurements(INVALID)
        self.assertEqual(decoded.pop("version"), 1)
        self.assertTrue(all(value is None for value in decoded.values()))

    def test_minimum_values_preserve_zero(self):
        decoded = listener.decode_measurements(bytes.fromhex("010000000000000000"))
        self.assertEqual(decoded["temperature_c"], -40.0)
        self.assertEqual(decoded["pressure_hpa"], 300.0)
        for key in ("humidity_percent", "pm25_ug_m3", "co2_ppm", "battery_percent"):
            self.assertEqual(decoded[key], 0)

    def test_bad_length_and_old_format_are_rejected(self):
        for data in (b"", VALID[:-1], VALID + b"\x00",
                     bytes.fromhex("010080ffffffffffffff0080ff")):
            with self.subTest(data=data), self.assertRaises(ValueError):
                listener.decode_measurements(data)

    def test_other_versions_are_rejected(self):
        for version in (0, 2, 7):
            data = bytes([(VALID[0] & ~7) | version]) + VALID[1:]
            with self.subTest(version=version), self.assertRaises(ValueError):
                listener.decode_measurements(data)


class ScannerTests(unittest.IsolatedAsyncioTestCase):
    async def test_service_company_and_address_filters(self):
        ready = asyncio.Event()
        scanner = AsyncMock()
        scanner.__aenter__.side_effect = lambda: ready.set()
        scanner_factory = Mock(return_value=scanner)
        with patch.dict("sys.modules", {"bleak": SimpleNamespace(BleakScanner=scanner_factory)}), \
                patch("builtins.print") as output:
            task = asyncio.create_task(listener.listen("aa:bb"))
            try:
                await asyncio.wait_for(ready.wait(), timeout=1)
                self.assertEqual(scanner_factory.call_args.kwargs["service_uuids"],
                                 [listener.SERVICE_UUID])
                callback = scanner_factory.call_args.kwargs["detection_callback"]
                device = SimpleNamespace(address="AA:BB")

                def report(uuids, company=listener.COMPANY_ID, data=VALID):
                    return SimpleNamespace(service_uuids=uuids,
                                           manufacturer_data={company: data}, rssi=-55)

                callback(device, report([]))
                callback(device, report(["unrelated-service"]))
                callback(device, report([listener.SERVICE_UUID], company=0x1234))
                callback(SimpleNamespace(address="CC:DD"), report([listener.SERVICE_UUID]))
                with self.assertLogs(listener.logger, level="WARNING"):
                    callback(device, report([listener.SERVICE_UUID], data=b"bad"))
                output.assert_not_called()

                advertisement = report([listener.SERVICE_UUID.upper()])
                callback(device, advertisement)
                callback(device, advertisement)  # Preserve repeated scanner reports.
                self.assertEqual(output.call_count, 2)
                decoded = json.loads(output.call_args.args[0])
                self.assertEqual(decoded["device"], "AA:BB")
                self.assertEqual(decoded["temperature_c"], 20.0)
                self.assertEqual(decoded["rssi_dbm"], -55)
            finally:
                task.cancel()
                with self.assertRaises(asyncio.CancelledError):
                    await task
            scanner.__aexit__.assert_awaited_once()


if __name__ == "__main__":
    unittest.main()
