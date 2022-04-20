#include <sys/syscalls.h>
#include <terminal/terminal.h>
#include <stdio.h>
#include <stdlib.h>

terminal s_term{"terminal_1"};

int main(int argc, char** argv)
{
	set_stdout([](const char* buf, size_t size, void* impl) {
					s_term.print_string(buf, size);
			   });

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