/**
 * AirGradient Go — Offline BLE measurement payload
 *
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#include "go_ble_advertisement.h"

#include <algorithm>
#include <cmath>

namespace GoBleAdvertisement {
namespace {

constexpr double PM25_SCALE = 10.0;
constexpr double PRESSURE_SCALE = 10.0;
constexpr double WHOLE_UNIT_SCALE = 1.0;
constexpr unsigned BITS_PER_BYTE = 8;

struct Field {
  unsigned bit;
  unsigned width;
};

constexpr Field VERSION_FIELD{0, 3};
constexpr Field TEMPERATURE{3, 11};
constexpr Field HUMIDITY{14, 7};
constexpr Field PM25{21, 14};
constexpr Field CO2{35, 14};
constexpr Field PRESSURE{49, 13};
constexpr Field BATTERY{62, 7};
constexpr double TEMPERATURE_OFFSET_C = 40.0;
constexpr double TEMPERATURE_SCALE = 10.0;
constexpr double PRESSURE_OFFSET_HPA = -300.0;

constexpr uint32_t invalid_value(Field field) { return (1u << field.width) - 1; }

// Scales value + offset; out-of-range, invalid or nonfinite values use the
// field's all-ones sentinel. Round to nearest, ties away from zero.
uint32_t encode_field(Field field, double value, bool valid, double offset, double scale) {
  const uint32_t invalid = invalid_value(field);
  if (!valid || !std::isfinite(value)) {
    return invalid;
  }
  const double rounded = std::round((value + offset) * scale);
  if (rounded < 0 || rounded >= invalid) {
    return invalid;
  }
  return static_cast<uint32_t>(rounded);
}

void put_bits(Payload &payload, Field field, uint32_t value) {
  for (unsigned i = 0; i < field.width; ++i) {
    if ((value >> i) & 1u) {
      const unsigned bit = field.bit + i;
      payload[bit / BITS_PER_BYTE] |= static_cast<uint8_t>(1u << (bit % BITS_PER_BYTE));
    }
  }
}

} // namespace

Payload encode(const MeasuresAGo &m) {
  Payload payload{};
  put_bits(payload, VERSION_FIELD, VERSION);
  put_bits(payload, TEMPERATURE,
           encode_field(TEMPERATURE, m.temp_hum_a.temperature, m.temp_hum_a.is_temp_valid(),
                        TEMPERATURE_OFFSET_C, TEMPERATURE_SCALE));
  put_bits(payload, HUMIDITY,
           encode_field(HUMIDITY, m.temp_hum_a.humidity, m.temp_hum_a.is_hum_valid(), 0,
                        WHOLE_UNIT_SCALE));
  put_bits(payload, PM25, encode_field(PM25, m.pm_a.pm_25, m.pm_a.is_pm_25_valid(), 0, PM25_SCALE));
  put_bits(payload, CO2, encode_field(CO2, m.co2.co2, m.co2.is_valid(), 0, WHOLE_UNIT_SCALE));
  put_bits(payload, PRESSURE,
           encode_field(PRESSURE, m.pressure.pressure, m.pressure.is_pressure_valid(),
                        PRESSURE_OFFSET_HPA, PRESSURE_SCALE));
  put_bits(payload, BATTERY,
           encode_field(BATTERY, m.power.battery_percentage, m.power.is_battery_percentage_valid(),
                        0, WHOLE_UNIT_SCALE));
  return payload;
}

ManufacturerData manufacturer_data(const Payload &payload) {
  ManufacturerData data{};
  data[0] = static_cast<uint8_t>(COMPANY_ID);
  data[1] = static_cast<uint8_t>(COMPANY_ID >> BITS_PER_BYTE);
  std::copy(payload.begin(), payload.end(), data.begin() + COMPANY_ID_SIZE);
  return data;
}

} // namespace GoBleAdvertisement
