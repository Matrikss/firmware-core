/**
 * AirGradient Go — Offline BLE measurement payload
 *
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#pragma once

#include "measures_types.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace GoBleAdvertisement {

inline constexpr const char *SERVICE_UUID = "992f1fe6-4519-4d4f-8ca7-546d36126ee8";
// The payload ships as Manufacturer Specific Data beside a Complete List of
// 128-bit Service UUIDs (SERVICE_UUID), so iOS can match it with a
// service-filtered scan while the phone is locked. The UUID list leaves room
// for only 9 payload bytes, so fields are bit-packed, least significant bit
// first across the bytes:
//   bits  0..2   version (1)
//   bits  3..13  temperature, (C + 40) * 10        0..2046, invalid 2047
//   bits 14..20  humidity, 1 %RH                   0..126,  invalid 127
//   bits 21..34  PM2.5, 0.1 ug/m3                  0..16382, invalid 16383
//   bits 35..48  CO2, 1 ppm                        0..16382, invalid 16383
//   bits 49..61  pressure, (hPa - 300) * 10        0..8190, invalid 8191
//   bits 62..68  battery, 1 %                      0..126,  invalid 127
//   bits 69..71  reserved, 0
// Barometric altitude is not sent; receivers derive it from pressure.
inline constexpr uint8_t VERSION = 1;
inline constexpr size_t PAYLOAD_SIZE = 9;
// Bluetooth SIG's company ID reserved for testing. Replace with an assigned ID
// before release.
inline constexpr uint16_t COMPANY_ID = 0xFFFF;
inline constexpr size_t COMPANY_ID_SIZE = 2;
inline constexpr size_t MANUFACTURER_DATA_SIZE = COMPANY_ID_SIZE + PAYLOAD_SIZE;

using Payload = std::array<uint8_t, PAYLOAD_SIZE>;
using ManufacturerData = std::array<uint8_t, MANUFACTURER_DATA_SIZE>;

// Validate each field independently; invalid, nonfinite or unencodable values
// use its all-ones sentinel. Round the offset/scaled value to nearest, ties
// away from zero. The caller supplies battery in power.battery_percentage.
Payload encode(const MeasuresAGo &measures);
// Company ID (little-endian) followed by the payload.
ManufacturerData manufacturer_data(const Payload &payload);

} // namespace GoBleAdvertisement
