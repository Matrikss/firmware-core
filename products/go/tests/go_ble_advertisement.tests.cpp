/**
 * AirGradient Go — Offline BLE measurement payload tests
 */

#include "go_ble_advertisement.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <limits>

namespace {

MeasuresAGo measures(float temperature, float humidity, float pm25, int co2, float pressure,
                     float battery) {
  MeasuresAGo measures{};
  measures.temp_hum_a.temperature = temperature;
  measures.temp_hum_a.humidity = humidity;
  measures.pm_a.pm_25 = pm25;
  measures.co2.co2 = co2;
  measures.pressure.pressure = pressure;
  measures.power.battery_percentage = battery;
  return measures;
}

uint32_t field(const GoBleAdvertisement::Payload &payload, unsigned bit, unsigned width) {
  uint32_t value = 0;
  for (unsigned i = 0; i < width; ++i) {
    const unsigned position = bit + i;
    value |= static_cast<uint32_t>((payload[position / 8] >> (position % 8)) & 1u) << i;
  }
  return value;
}

} // namespace

TEST_CASE("BLE advertisement: exact bit-packed wire bytes", "[ble][advertisement]") {
  const auto payload =
      GoBleAdvertisement::encode(measures(20.0f, 50.6f, 10.0f, 400, 1013.25f, 74.6f));
  const GoBleAdvertisement::Payload expected{0xC1, 0xD2, 0x8C, 0x0C, 0x80, 0x0C, 0xBA, 0xF7, 0x12};
  REQUIRE(payload == expected);
  CHECK(field(payload, 0, 3) == 1);
  CHECK(field(payload, 3, 11) == 600);  // (20.0 + 40) * 10
  CHECK(field(payload, 14, 7) == 51);   // 50.6 rounds up
  CHECK(field(payload, 21, 14) == 100); // 10.0 * 10
  CHECK(field(payload, 35, 14) == 400);
  CHECK(field(payload, 49, 13) == 7133); // (1013.25 - 300) * 10, ties away from zero
  CHECK(field(payload, 62, 7) == 75);
  CHECK(field(payload, 69, 3) == 0);
}

TEST_CASE("BLE advertisement: invalid measurements use all-ones sentinels",
          "[ble][advertisement]") {
  const auto payload = GoBleAdvertisement::encode(MeasuresAGo{});
  const GoBleAdvertisement::Payload expected{0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
  CHECK(payload == expected);
}

TEST_CASE("BLE advertisement: range limits", "[ble][advertisement]") {
  SECTION("lowest representable values") {
    const auto payload = GoBleAdvertisement::encode(measures(-40.0f, 0.0f, 0.0f, 0, 300.0f, 0.0f));
    CHECK(field(payload, 3, 11) == 0);
    CHECK(field(payload, 14, 7) == 0);
    CHECK(field(payload, 21, 14) == 0);
    CHECK(field(payload, 35, 14) == 0);
    CHECK(field(payload, 49, 13) == 0);
    CHECK(field(payload, 62, 7) == 0);
  }
  SECTION("highest valid sensor values fit") {
    // MeasuresRange already rejects anything above 125 C, 10000 ppm and 1100 hPa.
    const auto payload =
        GoBleAdvertisement::encode(measures(125.0f, 100.0f, 1638.2f, 10000, 1100.0f, 100.0f));
    CHECK(field(payload, 3, 11) == 1650);
    CHECK(field(payload, 14, 7) == 100);
    CHECK(field(payload, 21, 14) == 16382);
    CHECK(field(payload, 35, 14) == 10000);
    CHECK(field(payload, 49, 13) == 8000);
    CHECK(field(payload, 62, 7) == 100);
  }
  SECTION("out of range becomes invalid") {
    const auto payload =
        GoBleAdvertisement::encode(measures(-40.1f, 127.0f, 1638.3f, 16383, 299.9f, 127.0f));
    CHECK(field(payload, 3, 11) == 2047);
    CHECK(field(payload, 14, 7) == 127);
    CHECK(field(payload, 21, 14) == 16383);
    CHECK(field(payload, 35, 14) == 16383);
    CHECK(field(payload, 49, 13) == 8191);
    CHECK(field(payload, 62, 7) == 127);
  }
}

TEST_CASE("BLE advertisement: manufacturer data starts with the company ID",
          "[ble][advertisement]") {
  const auto payload = GoBleAdvertisement::encode(MeasuresAGo{});
  const auto data = GoBleAdvertisement::manufacturer_data(payload);
  CHECK(data[0] == 0xFF);
  CHECK(data[1] == 0xFF);
  CHECK(std::equal(payload.begin(), payload.end(), data.begin() + 2));
}

TEST_CASE("BLE advertisement: nonfinite and overflowing values are invalid",
          "[ble][advertisement]") {
  const float value =
      GENERATE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
               -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::max());
  CHECK(GoBleAdvertisement::encode(measures(value, value, value, -1, value, value)) ==
        GoBleAdvertisement::encode(MeasuresAGo{}));
}

TEST_CASE("BLE advertisement: sensor validation precedes rounding", "[ble][advertisement]") {
  // These values could round into the wire range, but are invalid sensor readings.
  SECTION("below minimum") {
    CHECK(GoBleAdvertisement::encode(measures(-40.01f, -0.01f, -0.01f, -1, 299.99f, -0.01f)) ==
          GoBleAdvertisement::encode(MeasuresAGo{}));
  }
  SECTION("above maximum") {
    CHECK(
        GoBleAdvertisement::encode(measures(125.01f, 100.01f, 1638.3f, 10001, 1100.01f, 100.01f)) ==
        GoBleAdvertisement::encode(MeasuresAGo{}));
  }
}

TEST_CASE("BLE advertisement: rounding applies after the field offset", "[ble][advertisement]") {
  // Binary-exact halves avoid floating-point representation noise.
  const auto payload =
      GoBleAdvertisement::encode(measures(-0.25f, 49.5f, 1.25f, 400, 1000.25f, 49.49f));
  CHECK(field(payload, 3, 11) == 398); // (-0.25 + 40) * 10 rounds up to 398.
  CHECK(field(payload, 14, 7) == 50);
  CHECK(field(payload, 21, 14) == 13);
  CHECK(field(payload, 49, 13) == 7003);
  CHECK(field(payload, 62, 7) == 49);
}

TEST_CASE("BLE advertisement: one failed field preserves the remaining readings",
          "[ble][advertisement]") {
  MeasuresAGo m{};
  m.temp_hum_a.temperature = 0.0f; // Humidity remains invalid.
  m.pressure.altitude = 0.0f;      // Pressure remains invalid and altitude is not sent.
  m.co2.co2 = 400;
  const auto payload = GoBleAdvertisement::encode(m);
  CHECK(field(payload, 3, 11) == 400);
  CHECK(field(payload, 14, 7) == 127);
  CHECK(field(payload, 21, 14) == 16383);
  CHECK(field(payload, 35, 14) == 400);
  CHECK(field(payload, 49, 13) == 8191);
  CHECK(field(payload, 62, 7) == 127);
}
