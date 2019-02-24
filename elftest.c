#include <sys/syscalls.h>

#include <stdio.h>

int _start(void)
{
	//while(1);
	
	puts("testing\n");
	
	file_stream* f = open("test.elf", 0);
	
	if(f == NULL)
	{
		puts("cannot open file\n");
		return 0;
	}
	
	char buf[5];
	
	read(buf, 4, f);
	
	//print_string("done reading\n");
	
	close(f);
	
	buf[4] = 0;
	
	while(1)
	{
		puts(buf);
		puts("\n");
		puts("hi\n\n");
	}
	
	exit(42);
	
	return 42;
}