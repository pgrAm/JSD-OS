#include <sys/syscalls.h>
#include <terminal/terminal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

terminal s_term{"terminal_1"};

int main(int argc, char** argv)
{
	set_stdout(
		[](const char* buf, size_t size, void* impl)
		{
			s_term.print(buf, size);
			return size;
		});

	auto end = clock() + (60 * CLOCKS_PER_SEC);
	while(clock() < end)
	{
		for(size_t i = 0; i < 10; i++)
		{
			printf("%d", i);
			auto next = clock() + (CLOCKS_PER_SEC);
			while(clock() < next);
		}
		printf("\n");
	}

	printf("BANG!\n");

	return 0;
}