/**
 * AirGradient
 * https://airgradient.com
 *
 * CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
 */

#ifndef AG_DECIMAL64_H
#define AG_DECIMAL64_H

#include <cstddef>
#include <cstdint>

// 20 digits (or sign + 19 digits), followed by the terminating null.
constexpr size_t DECIMAL64_BUFFER_SIZE = 21;

// Return the start of the decimal text within the caller's buffer for use with %s.
// The array reference enforces sufficient space without allocation or truncation.
inline char *format_uint64_decimal(uint64_t value, char (&buffer)[DECIMAL64_BUFFER_SIZE]) {
  constexpr uint64_t RADIX = 10;
  char *text = buffer + DECIMAL64_BUFFER_SIZE - 1;
  *text = '\0';
  do {
    *--text = static_cast<char>('0' + value % RADIX);
    value /= RADIX;
  } while (value != 0);
  return text;
}

inline char *format_int64_decimal(int64_t value, char (&buffer)[DECIMAL64_BUFFER_SIZE]) {
  // Unsigned subtraction also handles INT64_MIN without signed overflow.
  const uint64_t magnitude =
      value < 0 ? uint64_t{0} - static_cast<uint64_t>(value) : static_cast<uint64_t>(value);
  char *text = format_uint64_decimal(magnitude, buffer);
  if (value < 0) {
    *--text = '-';
  }
  return text;
}

#endif // AG_DECIMAL64_H
