#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "png.h"

extern "C" void course_process_image(const uint8_t* rgba,
                                     size_t rgba_size,
                                     uint32_t width,
                                     uint32_t height,
                                     uint8_t channels,
                                     const uint8_t* raw,
                                     size_t raw_size);

static void safe_png_cleanup(png_structp png_ptr, png_infop info_ptr) {
  if (png_ptr) png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
}

extern "C" void course_preparse_trigger(const uint8_t* raw, size_t raw_size);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 2) return 0;               // allow tiny inputs for preparse trigger

  // course_preparse_trigger(data, size);  // run trigger even if PNG parsing is skipped

  if (size < 8) return 0;               // PNG header parsing needs at least 8 bytes

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_ptr) return 0;

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    safe_png_cleanup(png_ptr, nullptr);
    return 0;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    safe_png_cleanup(png_ptr, info_ptr);
    return 0;
  }

  struct Mem {
    const uint8_t* p;
    size_t n;
    size_t off;
  } mem{data, size, 0};

  auto read_fn = [](png_structp pngp, png_bytep outBytes, png_size_t byteCountToRead) {
    Mem* m = (Mem*)png_get_io_ptr(pngp);
    if (!m || m->off + byteCountToRead > m->n) {
      png_error(pngp, "read_fn: out of bounds");
      return;
    }
    memcpy(outBytes, m->p + m->off, byteCountToRead);
    m->off += byteCountToRead;
  };

  png_set_read_fn(png_ptr, &mem, read_fn);

  png_read_info(png_ptr, info_ptr);

  png_uint_32 width = 0, height = 0;
  int bit_depth = 0, color_type = 0, interlace_type = 0, compression_type = 0, filter_method = 0;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               &interlace_type, &compression_type, &filter_method);

  if (bit_depth == 16) png_set_strip_16(png_ptr);
  if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  if (rowbytes == 0 || height == 0 || height > (1u << 22)) {
    safe_png_cleanup(png_ptr, info_ptr);
    return 0;
  }

  size_t total = rowbytes * (size_t)height;
  if (total > (32u * 1024u * 1024u)) {
    safe_png_cleanup(png_ptr, info_ptr);
    return 0;
  }

  uint8_t* rgba = (uint8_t*)malloc(total);
  if (!rgba) {
    safe_png_cleanup(png_ptr, info_ptr);
    return 0;
  }

  png_bytep* rows = (png_bytep*)malloc(sizeof(png_bytep) * (size_t)height);
  if (!rows) {
    free(rgba);
    safe_png_cleanup(png_ptr, info_ptr);
    return 0;
  }

  for (png_uint_32 y = 0; y < height; y++) {
    rows[y] = rgba + (size_t)y * rowbytes;
  }

  png_read_image(png_ptr, rows);

  course_process_image(rgba, total, (uint32_t)width, (uint32_t)height, 4, data, size);

  free(rows);
  free(rgba);
  safe_png_cleanup(png_ptr, info_ptr);
  return 0;
}
