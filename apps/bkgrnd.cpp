#include <sys/syscalls.h>
#include <graphics/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv)
{
	initialize_text_mode(0, 0);

	auto end = clock() + (60 * CLOCKS_PER_SEC);
	while(clock() < end)
	{
		printf("tick.\n");
		auto next = clock() + (10 * CLOCKS_PER_SEC);
		while(clock() < next);
	}

	printf("TOCK!\n");

	return 0;
}