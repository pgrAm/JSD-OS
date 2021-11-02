#ifndef DISPLAY_MODE_H
#define DISPLAY_MODE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum display_format 
{
	FORMAT_DONT_CARE = 0,
	FORMAT_TEXT_W_ATTRIBUTE,
	FORMAT_INDEXED_BIT_PLANE,
	FORMAT_INDEXED_BYTE_PLANE,
	FORMAT_INDEXED_LINEAR,

	FORMAT_GRAYSCALE,

	FORMAT_BGR444,
	FORMAT_RGB332,
	FORMAT_RGB444,
	FORMAT_RGB555,
	FORMAT_BGR555,
	FORMAT_ARGB4444,
	FORMAT_RGBA4444,
	FORMAT_ABGR4444,
	FORMAT_BGRA4444,
	FORMAT_ARGB1555,
	FORMAT_RGBA5551,
	FORMAT_ABGR1555,
	FORMAT_BGRA5551,
	FORMAT_RGB565,
	FORMAT_BGR565,
	FORMAT_RGB24,
	FORMAT_BGR24,
	FORMAT_RGB888,
	FORMAT_RGBX8888,
	FORMAT_BGR888,
	FORMAT_BGRX8888,
	FORMAT_ARGB8888,
	FORMAT_RGBA8888,
	FORMAT_ABGR8888,
	FORMAT_BGRA8888,
	FORMAT_ARGB2101010,

	FORMAT_RGBA32 = FORMAT_RGBA8888, //RGBA byte array of color data, for the current platform
	FORMAT_ARGB32 = FORMAT_ARGB8888, //ARGB byte array of color data, for the current platform
	FORMAT_BGRA32 = FORMAT_BGRA8888, //BGRA byte array of color data, for the current platform
	FORMAT_ABGR32 = FORMAT_ABGR8888, //ABGR byte array of color data, for the current platform

	/*
	FORMAT_YV12, //planar mode : Y + V + U(3 planes)
	FORMAT_IYUV, //planar mode : Y + U + V(3 planes)
	FORMAT_YUY2, //packed mode : Y0 + U0 + Y1 + V0(1 plane)
	FORMAT_UYVY, //packed mode : U0 + Y0 + V0 + Y1(1 plane)

	FORMAT_YVYU, //packed mode : Y0 + V0 + Y1 + U0(1 plane)
	FORMAT_NV12, //planar mode : Y + U / V interleaved(2 planes)
	FORMAT_NV21, //planar mode : Y + V / U interleaved(2 planes)
	*/
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
	size_t pitch;	//in bytes
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