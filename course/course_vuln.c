#include "course_vuln.h"
#include <stdlib.h>
#include <string.h>

// INTENTIONALLY BUGGY CODE FOR COURSE USE ONLY.
//
// Required bugs:
//   BUG-1: use-after-free
//   BUG-2: double-free
//
// Optional stretch bugs:
//   BUG-3: integer overflow -> heap buffer overflow
//   BUG-4: out-of-bounds read

static void bug1_use_after_free(const uint8_t* rgba, size_t rgba_size) {
  if (rgba_size < 16) return;
  uint8_t* tmp = (uint8_t*)malloc(16);
  if (!tmp) return;
  memcpy(tmp, rgba, 16);
  free(tmp);

  // INTENTIONAL BUG: use-after-free
  volatile uint8_t x = tmp[0];
  (void)x;
}

static void bug2_double_free(const uint8_t* rgba, size_t rgba_size) {
  if (rgba_size < 8) return;
  uint8_t* tmp = (uint8_t*)malloc(8);
  if (!tmp) return;
  memcpy(tmp, rgba, 8);
  free(tmp);

  // INTENTIONAL BUG: double free
  free(tmp);
}

static void bug3_overflow_heap_write(const uint8_t* rgba, size_t rgba_size,
                                     uint32_t width, uint32_t height, uint8_t channels) {
  // INTENTIONAL BUG: 32-bit overflow in allocation size computation
  uint32_t pixels = width * height;
  uint32_t need = pixels * channels;
  uint8_t* out = (uint8_t*)malloc((size_t)need);
  if (!out) return;

  // Copy based on "real" size: overflow write if need < real_need
  size_t real_need = (size_t)width * (size_t)height * (size_t)channels;
  if (real_need > rgba_size) real_need = rgba_size;
  memcpy(out, rgba, real_need);
  free(out);
}

static uint8_t bug4_oob_read(const uint8_t* rgba, size_t rgba_size,
                             uint32_t width, uint32_t height) {
  // INTENTIONAL BUG: index can exceed rgba_size
  uint32_t idx = (width << 16) ^ height;
  (void)rgba_size;
  return rgba[idx];
}

void course_preparse_trigger(const uint8_t* raw, size_t raw_size) {
  if (!raw || raw_size < 2) return;

  // Disabled in the released harness. Kept here for staff experiments only.
  if (raw[0] == 'A' && raw[1] == 'A') {
    uint8_t* tmp = (uint8_t*)malloc(16);
    if (!tmp) return;
    tmp[0] = raw[0];
    tmp[1] = raw[1];
    free(tmp);

    volatile uint8_t x = tmp[0];
    (void)x;
  }
}

void course_process_image(const uint8_t* rgba,
                          size_t rgba_size,
                          uint32_t width,
                          uint32_t height,
                          uint8_t channels,
                          const uint8_t* raw,
                          size_t raw_size) {
  (void)width;
  (void)height;
  (void)channels;
  (void)raw;
  (void)raw_size;

  if (!rgba || rgba_size == 0) return;

  // Required Bug 1: found first.
  if (rgba_size >= 16) {
    bug1_use_after_free(rgba, rgba_size);
  }

  // Required Bug 2: exposed after Bug 1 is fixed.
  if (rgba_size >= 8) {
    bug2_double_free(rgba, rgba_size);
  }

  // Optional stretch bugs for future offerings / extra credit.
  // if (width == 32) {
  //   bug3_overflow_heap_write(rgba, rgba_size, width, height, channels);
  // }
  //
  // if (height == 32) {
  //   (void)bug4_oob_read(rgba, rgba_size, width, height);
  // }
}