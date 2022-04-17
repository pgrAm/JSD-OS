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

void draw_gradient(size_t begin, size_t end, display_mode& actual, uint32_t* mem)
{
	size_t ddpl = (actual.pitch / 4);
	size_t offset = 0;

	for(size_t y = begin; y < end; y++)
	{
		auto cy = y * 255 / (end - begin);

		for(size_t x = 0; x < actual.width; x++)
		{
			auto c = x * 255 / actual.width;

			mem[offset++] = from_rgb(c, cy, 255 - c);
		}
		offset += ddpl - actual.width;
	}
}

void draw_fill(size_t x0, size_t y0, size_t w, size_t h, display_mode& actual, uint32_t* mem)
{
	size_t ddpl = (actual.pitch / 4);
	size_t offset = y0 * ddpl + x0;
	uint32_t value = from_rgb(255, 255, 255);

	for(size_t y = 0; y < h; y++)
	{
		for(size_t x = 0; x < w; x++)
		{
			mem[offset++] = value;
		}
		offset += ddpl - w;
	}
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

	int cursor_x = 0, cursor_y = 0;

	size_t page_begin = 0;
	set_display_offset(page_begin, true);

	input_event e{};
	while(!(e.type == KEY_DOWN && e.data == VK_ESCAPE))
	{
		while(get_input_event(&e, false) == 0)
		{
			if(e.device_index == 0)
			{
				if(e.type == AXIS_MOTION)
				{
					if(e.control_index == 0) // x axis
					{
						cursor_x = (cursor_x + e.data);

						if(cursor_x < 0)
							cursor_x = 0;
						else if(cursor_x >= (actual.width - 32))
							cursor_x = actual.width - 32;
					}
					else if(e.control_index == 1) // y axis
					{
						cursor_y = (cursor_y + e.data);

						if(cursor_y < 0)
							cursor_y = 0;
						else if(cursor_y >= (actual.height - 32))
							cursor_y = actual.height - 32;
					}
				}
			}
		}

		if(page_begin == 0)
		{
			page_begin = (actual.height * actual.pitch);
		}
		else
		{
			page_begin = 0;
		}

		draw_gradient(0, actual.height, actual, mem + page_begin / 4);

		draw_fill(cursor_x, cursor_y, 32, 32, actual, mem + page_begin / 4);

		set_display_offset(page_begin, true);
	}

	initialize_text_mode(80, 25);
	video_clear();

	return 0;
}