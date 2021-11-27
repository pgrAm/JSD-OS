#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	initialize_text_mode(0, 0);

	video_clear();

	display_mode mode;
	int i = 0;
	while(get_display_mode(i, &mode) == 0)
	{
		if(mode.flags & DISPLAY_TEXT_MODE)
			printf("text     ");
		else
			printf("graphics ");

		printf("%dx%d@%d", mode.width, mode.height, mode.bpp);

		if(i & 1)
			putchar('\n');
		else
			putchar('\t');

		i++;
	}
	return 42;
}