#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	display_mode requested = {
		1024, 768,
		0,
		0,
		32,
		FORMAT_RGBA,
		0
	};
	display_mode actual;
	if(set_display_mode(&requested, &actual) != 0)
	{
		initialize_text_mode(80, 25);
		printf("Could not set graphics mode\n");
		return 42;
	}

	auto mem = (uint32_t*)map_display_memory();

	size_t spacing = actual.width / 10;
	for(size_t x = 0; x < actual.width; x += spacing)
	{
		for(size_t y = 0; y < actual.height; y++)
		{
			mem[y * (actual.pitch / 4) + x] = 0xFFFF00;
		}
	}

	while(VK_ESCAPE != wait_and_getkey());

	initialize_text_mode(80, 25);
	video_clear();

	return 0;
}