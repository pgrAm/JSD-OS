int main(void)
{
	__asm__ ("int $0x80");
	
	return 42;
}