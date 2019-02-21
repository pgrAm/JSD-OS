#include "api/syscalls.h"

int _start(void)
{
	//while(1);
	
	print_string("testing\n");
	
	file_stream* f = open("test.elf", 0);
	
	if(f == NULL)
	{
		print_string("cannot open file\n");
		return 0;
	}
	
	char buf[5];
	
	read(buf, 4, f);
	
	//print_string("done reading\n");
	
	close(f);
	
	buf[4] = 0;
	
	print_string(buf);
	
	print_string("\n");
	
	exit(42);
	
	return 42;
}