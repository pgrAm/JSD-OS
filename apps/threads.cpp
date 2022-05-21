#include <stdio.h>
#include <terminal/terminal.h>
#include <sys/syscalls.h>

terminal s_term{"terminal_1"};

int main(int argc, char** argv)
{
	set_stdout([](const char* buf, size_t size, void* impl)
			   { s_term.print(buf, size); });

	auto tid = spawn_thread(
		[]()
		{
			printf("this is a test from another thread!\n");
			exit_thread(0);
		});

	printf("Spawned new thread tid = %d\n", tid);

	return 0;
}