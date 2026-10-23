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

inline constexpr const char *SERVICE_UUID = "d2086101-9a63-4acc-8a7c-c07f607cb240";
inline constexpr uint8_t VERSION = 1;
inline constexpr size_t PAYLOAD_SIZE = 13;

inline constexpr size_t VERSION_OFFSET = 0;
inline constexpr size_t TEMPERATURE_OFFSET = 1;
inline constexpr size_t HUMIDITY_OFFSET = 3;
inline constexpr size_t PM25_OFFSET = 4;
inline constexpr size_t CO2_OFFSET = 6;
inline constexpr size_t PRESSURE_OFFSET = 8;
inline constexpr size_t ALTITUDE_OFFSET = 10;
inline constexpr size_t BATTERY_OFFSET = 12;

inline constexpr uint8_t INVALID_U8 = 0xFF;
inline constexpr uint16_t INVALID_U16 = 0xFFFF;
inline constexpr uint16_t INVALID_I16 = 0x8000;

using Payload = std::array<uint8_t, PAYLOAD_SIZE>;

// Service Data body only; excludes the AD header and UUID. Multibyte fields
// are little-endian. Units: 0.01 C, 1 %RH, 0.1 ug/m3, 1 ppm, 0.1 hPa, 1 m,
// and 1 % battery. Altitude comes from pressure.altitude, not GPS.
// Round to nearest, ties away from zero; invalid or unencodable fields use
// their wire sentinel. The caller supplies battery in power.battery_percentage.
Payload encode(const MeasuresAGo &measures);

} // namespace GoBleAdvertisement
