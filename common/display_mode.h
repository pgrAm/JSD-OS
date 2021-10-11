#ifndef DISPLAY_MODE_H
#define DISPLAY_MODE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum display_format {
	FORMAT_DONT_CARE = 0,
	FORMAT_TEXT_W_ATTRIBUTE,
	FORMAT_INDEXED_BIT_PLANE,
	FORMAT_INDEXED_BYTE_PLANE,
	FORMAT_INDEXED_LINEAR,
	FORMAT_RGB,
	FORMAT_ARGB,
	FORMAT_RGBA,
	FORMAT_BGR,
	FORMAT_BGRA,
	FORMAT_ABGR,
	FORMAT_GRAYSCALE
};

typedef enum display_format display_format;

enum display_flags {
	DISPLAY_TEXT_MODE = 0x01,
	DISPLAY_INDEXED = 0x02,
	DISPLAY_RGB = 0x04,
	DISPLAY_MONO = 0x08
};

struct display_mode {
	size_t width;
	size_t height;
	size_t pitch;
	size_t refresh;
	size_t bpp;
	display_format format;
	uint32_t flags;
};

typedef struct display_mode display_mode;

#ifdef __cplusplus
}
#endif
#endif