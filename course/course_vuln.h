#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void course_preparse_trigger(const uint8_t* raw, size_t raw_size);


void course_process_image(const uint8_t* rgba,
                          size_t rgba_size,
                          uint32_t width,
                          uint32_t height,
                          uint8_t channels,
                          const uint8_t* raw,
                          size_t raw_size);

#ifdef __cplusplus
}
#endif
