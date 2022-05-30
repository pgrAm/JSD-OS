#include <stdio.h>
#include <terminal/terminal.h>
#include <sys/syscalls.h>
#include <thread.h>

terminal s_term{"terminal_1"};

thread_local int tls_test = 0;

int main(int argc, char** argv)
{
	set_stdout([](const char* buf, size_t size, void* impl)
			   { s_term.print(buf, size); });

	auto tid = spawn_thread(
		[]()
		{
			tls_test = 1;
			for(size_t i = 0; i < 10; i++)
			{
				printf("TLS data = %d, this is a test from another thread!\n",
					   tls_test);

				yield_to(INVALID_TASK_ID);
			}
		});

	printf("Spawned new thread tid = %d\n", tid);

	for(size_t i = 0; i < 10; i++)
	{
		printf("TLS data = %d, main thread\n", tls_test);
		yield_to(tid);
	}

	return 0;
}