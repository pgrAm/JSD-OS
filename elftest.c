#include <sys/syscalls.h>
#include <stdio.h>

extern uint16_t getSS();

int _start(void)
{
	puts("testing\n");

	printf("SS=%X\n", getSS());
	file_stream* f = open("test.elf", 0);
	
	if(f == NULL)
	{
		puts("cannot open file\n");
		return 0;
	}
	
	char buf[5];
	
	read(buf, 4, f);
	
	close(f);
	
	//spawn_process("test.elf", WAIT_FOR_PROCESS);

	buf[4] = 0;
	
	//while(1)
	//{
	//	//puts(buf);
	//	//puts("\n");
	//	//puts("hi\n\n");
	//}
	
	exit(42);
	
	return 42;
}