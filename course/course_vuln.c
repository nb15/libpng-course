#include "course_vuln.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// INTENTIONALLY BUGGY CODE FOR COURSE USE ONLY.
// Bug IDs:
//   BUG-1: integer overflow -> heap buffer overflow
//   BUG-2: out-of-bounds read
//   BUG-3: use-after-free
//   BUG-4: double-free

static void bug1_overflow_heap_write(const uint8_t* rgba, size_t rgba_size,
                                     uint32_t width, uint32_t height, uint8_t channels) {
  // INTENTIONAL BUG: 32-bit overflow in allocation size computation
  uint32_t pixels = width * height;          // overflow
  uint32_t need = pixels * channels;         // overflow
  uint8_t* out = (uint8_t*)malloc((size_t)need);
  if (!out) return;

  // Copy based on "real" size: overflow write if need < real_need
  size_t real_need = (size_t)width * (size_t)height * (size_t)channels;
  if (real_need > rgba_size) real_need = rgba_size;
  memcpy(out, rgba, real_need);
  free(out);
}

static uint8_t bug2_oob_read(const uint8_t* rgba, size_t rgba_size,
                            uint32_t width, uint32_t height) {
  // INTENTIONAL BUG: index can exceed rgba_size
  uint32_t idx = (width << 16) ^ height;
  (void)rgba_size;
  return rgba[idx]; // OOB read if idx >= rgba_size
}

static void bug3_use_after_free(const uint8_t* rgba, size_t rgba_size) {
  if (rgba_size < 16) return;
  uint8_t* tmp = (uint8_t*)malloc(16);
  if (!tmp) return;
  memcpy(tmp, rgba, 16);
  free(tmp);

  // INTENTIONAL BUG: use-after-free
  volatile uint8_t x = tmp[0];
  (void)x;
}

static void bug4_double_free(const uint8_t* rgba, size_t rgba_size) {
  if (rgba_size < 8) return;
  uint8_t* tmp = (uint8_t*)malloc(8);
  if (!tmp) return;
  memcpy(tmp, rgba, 8);
  free(tmp);
  // INTENTIONAL BUG: double free
  free(tmp);
}


void course_preparse_trigger(const uint8_t* raw, size_t raw_size) {
  (void)raw; (void)raw_size;
  const char msg[] = "PREPARSE_TRIGGER_CALLED\n";
  write(2, msg, sizeof(msg) - 1);
  __builtin_trap();
}



void course_process_image(const uint8_t* rgba,
                          size_t rgba_size,
                          uint32_t width,
                          uint32_t height,
                          uint8_t channels,
                          const uint8_t* raw,
                          size_t raw_size) {
  if (!rgba || rgba_size == 0 || !raw || raw_size == 0) return;

  // Deterministic gates so fuzzing can find these on laptops.
  if ((width & 0x3FF) == 0x155) {
    bug1_overflow_heap_write(rgba, rgba_size, width, height, channels);
  }
  if ((height & 0x3FF) == 0x2AA) {
    (void)bug2_oob_read(rgba, rgba_size, width, height);
  }
  if (raw_size >= 2 && raw[0] == 'A' && raw[1] == 'A') {
    bug3_use_after_free(rgba, rgba_size);
  }
  if ((rgba_size >= 4) && ((uint8_t)(rgba[2] + rgba[3]) == 0xFF)) {
    bug4_double_free(rgba, rgba_size);
  }
}
