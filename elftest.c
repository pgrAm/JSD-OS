#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>

extern uint16_t getSS();

int _start(void)
{
	set_video_mode(320, 200, VIDEO_8BPP);

	uint8_t* mem = map_video_memory();
	for (int i = 0; i < 100; i++)
	{
		memset(mem + i * 2 * 320, i, 320);
		memset(mem + (i * 2 + 1) * 320, 0, 320);
	}

	while(VK_ESCAPE != wait_and_getkey());

	initialize_text_mode(90, 60);
	video_clear();
	exit(42);
	
	return 42;
}