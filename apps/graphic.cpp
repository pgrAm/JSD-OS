#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t from_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	return	((uint32_t)r << 16)|
			((uint32_t)g << 8) |
			((uint32_t)b);
}

int main(int argc, char** argv)
{
	display_mode requested = {
		800, 600,
		0,
		0,
		32,
		FORMAT_ARGB32,
		0,
		0
	};
	display_mode actual;
	if(set_display_mode(&requested, &actual) != 0)
	{
		initialize_text_mode(80, 25);
		printf("Could not set graphics mode\n");
		return 0;
	}

	auto mem = (uint32_t*)map_display_memory();

	size_t x_spacing = actual.width / 10;
	for(size_t x = 0; x < actual.width; x += x_spacing)
	{
		for(size_t y = 0; y < actual.height; y++)
		{
			mem[y * (actual.pitch / 4) + x] = 0xFFFF00;
		}
	}

	size_t y_spacing = actual.height / 10;
	for(size_t y = 0; y < actual.height; y += y_spacing)
	{
		for(size_t x = 0; x < actual.width; x++)
		{
			mem[y * (actual.pitch / 4) + x] = 0x00FFFF00;
		}
	}

	//while(VK_ESCAPE != wait_and_getkey());

	const size_t virtual_height = actual.buffer_size / actual.pitch;

	for(size_t y = 0; y < virtual_height; y++)
	{
		auto cy = y * 255 / virtual_height;

		for(size_t x = 0; x < actual.width; x++)
		{
			auto c = x * 255 / actual.width;

			mem[y * (actual.pitch / 4) + x] = from_rgb(c, cy, 255 - c);
		}
	}

	size_t offset = 0;
	while(offset < actual.buffer_size - actual.height * actual.pitch)
	{
		set_display_offset(offset, true);
		offset += actual.pitch;
	}

	while(VK_ESCAPE != wait_and_getkey());

	initialize_text_mode(80, 25);
	video_clear();

	return 0;
}