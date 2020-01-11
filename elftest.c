#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>

extern uint16_t getSS();

int _start(void)
{
	//set_video_mode(90, 60, VIDEO_TEXT_MODE);

	*(volatile uint8_t*)0 = 1;
	//puts("testing\n");

	do
	{
		initialize_text_mode(80, 50);
		video_clear();
		for (int i = 0; i < 256; i++)
		{
			putchar(i);
		}
		wait_and_getkey();

		initialize_text_mode(80, 25);
		video_clear();
		for (int i = 0; i < 256; i++)
		{
			putchar(i);
		}
	}
	while(wait_and_getkey() != 'x');

	printf("SS=%X\n", getSS());

	set_video_mode(320, 200, VIDEO_8BPP);

	uint8_t* mem = map_video_memory();
	for (int i = 0; i < 100; i++)
	{
		memset(mem + i * 2 * 320, i, 320);
		memset(mem + (i * 2 + 1) * 320, 0, 320);
	}
	//spawn_process("test.elf", WAIT_FOR_PROCESS);
	
	//*(volatile uint8_t*)0 = 1;

	wait_and_getkey();
	//{
	//	//puts(buf);
	//	//puts("\n");
	//	//puts("hi\n\n");
	//}
	set_video_mode(90, 60, VIDEO_TEXT_MODE);
	exit(42);
	
	return 42;
}