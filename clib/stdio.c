#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef __KERNEL
#include <sys/syscalls.h>
#else
#include <kernel/sys/syscalls.h>
#endif

#define		PR_LJ	0x01u	// left justify */
#define		PR_CA	0x02u	// use A-F instead of a-f for hex */
#define		PR_SG	0x04u	// signed numeric conversion (%d vs. %u) */
#define		PR_32	0x08u	// long (32-bit) numeric conversion */
#define		PR_16	0x10u	// short (16-bit) numeric conversion */
#define		PR_WS	0x20u	// PR_SG set and num was < 0 */
#define		PR_LZ	0x40u	// pad left with '0' instead of ' ' */
#define		PR_FP	0x80u	// pointers are far */

/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define		PR_BUFLEN	16

#ifndef __KERNEL

struct FILE
{
	void* impl_ptr;
	void (*write)(const char* buf, size_t size, void* impl);
	void (*read)(char* buf, size_t size, void* impl);
	void (*close)(void* impl);
};

static FILE m_stdout = {NULL, NULL, NULL};

static FILE* stdout = &m_stdout;

void set_stdout(void (*write)(const char* buf, size_t size, void* impl))
{
	stdout->write = write;
}

static void file_write(const char* buf, size_t size, FILE* f)
{
	if(f != NULL && f->write != NULL)
	{
		f->write(buf, size, f->impl_ptr);
	}
}
#else
static FILE* stdout = NULL;

void file_write(const char* buf, size_t size, FILE* f)
{
	print_string_len(buf, size);
}
#endif

int putchar(int character)
{
	char c = (char)character;
	
	file_write(&c, 1, stdout);

	return character;
}

#define WRITECHAR(c, m) if(count < (m - 1)) { *ptr++ = c; } count++;

int vsnprintf(char *buffer, size_t n, const char *fmt, va_list args)
{
	unsigned state = 0, flags = 0, radix = 0, actual_wd = 0, count = 0, given_wd = 0;
	char* where = NULL;
	char buf[PR_BUFLEN];

	char* ptr = buffer;

	for(; *fmt; fmt++) /* begin scanning format specifier list */
	{
		switch(state)
		{
		case 0: /* STATE 0: AWAITING % */
			if(*fmt != '%')	/* not %... */
			{
				WRITECHAR(*fmt, n);
				break;
			}
			state++; /* found %, get next char and advance state to check if next char is a flag */
			fmt++;
			/* FALL THROUGH */
		case 1: /* STATE 1: AWAITING FLAGS (%-0) */
			if(*fmt == '%')	/* %% */
			{
				WRITECHAR(*fmt, n);
				state = flags = given_wd = 0;
				break;
			}
			else if(*fmt == '-')
			{
				if(flags & PR_LJ)/* %-- is illegal */
				{
					state = flags = given_wd = 0;
				}
				else
				{
					flags |= PR_LJ;
				}
				break;
			}
			state++; /* not a flag char: advance state to check if it's field width */

			if(*fmt == '0') /* check now for '%0...' */
			{
				flags |= PR_LZ;
				fmt++;
			}
			/* FALL THROUGH */
		case 2: /* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
			if(*fmt >= '0' && *fmt <= '9')
			{
				given_wd = 10 * given_wd + (unsigned int)(*fmt - '0');
				break;
			}
			state++; /* not field width: advance state to check if it's a modifier */
			/* FALL THROUGH */
		case 3: /* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
			switch(*fmt)
			{
			case 'F':
				flags |= PR_FP;
				break;
			case 'N':
				break;
			case 'l':
				flags |= PR_32;
				break;
			case 'h':
				flags |= PR_16;
				break;
			}
			state++; /* not modifier: advance state to check if it's a conversion char */
			/* FALL THROUGH */
		case 4: /* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch(*fmt)
			{
			case 'X':
				flags |= PR_CA;
				/* FALL THROUGH */
			case 'x': /* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'p':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
				/* FALL THROUGH */
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;
DO_NUM:			/* load the value to be printed. l=long=32 bits: */
				{
					long num = 0;
					if(flags & PR_32)
					{
						num = (long)va_arg(args, unsigned long);
					}
					else if(flags & PR_16) /* h=short=16 bits (signed or unsigned) */
					{
						if(flags & PR_SG)
						{
							num = (long)va_arg(args, int);
						}
						else
						{
							num = (long)va_arg(args, unsigned int);
						}
					}
					else /* no h nor l: sizeof(int) bits (signed or unsigned) */
					{
						if(flags & PR_SG)
						{
							num = (long)va_arg(args, int);
						}
						else
						{
							num = (long)va_arg(args, unsigned int);
						}
					}

					if(flags & PR_SG) /* take care of sign */
					{
						if(num < 0)
						{
							flags |= PR_WS;
							num = -num;
						}
					}

					{
						//convert binary to octal/decimal/hex ASCII.
						//The math here is _always_ unsigned
						unsigned long u_num = (unsigned long)num;
						do 
						{
							unsigned long digit = u_num % radix;
							where--;
							if(digit < 10)
							{
								*where = (char)(digit + '0');
							}
							else if(flags & PR_CA)
							{
								*where = (char)(digit - 10 + 'A');
							}
							else
							{
								*where = (char)(digit - 10 + 'a');
							}
							u_num = u_num / radix;
						} while(u_num != 0);
					}
				}
				goto EMIT;
			case 'c':
				flags &= ~PR_LZ; /* disallow pad-left-with-zeroes for %c */
				where--;
				*where = (char)va_arg(args, unsigned int);
				actual_wd = 1;
				goto EMIT2;
			case 's':
				flags &= ~PR_LZ; /* disallow pad-left-with-zeroes for %s */
				where = va_arg(args, char*);
EMIT:
				actual_wd = strlen(where);
				if(flags & PR_WS)
				{
					actual_wd++;
				}

				if((flags & (PR_WS | PR_LZ)) == (PR_WS | PR_LZ)) /* if we pad left with ZEROES, do the sign now */
				{
					WRITECHAR('-', n);
				}

EMIT2:			/* pad on left with spaces or zeroes (for right justify) */
				if((flags & PR_LJ) == 0) 
				{
					while(given_wd > actual_wd)
					{
						WRITECHAR(flags & PR_LZ ? '0' : ' ', n);
						given_wd--;
					}
				}

				if((flags & (PR_WS | PR_LZ)) == PR_WS) /* if we pad left with SPACES, do the sign now */
				{
					WRITECHAR('-', n);
				}

				while(*where != '\0') /* emit string/char/converted number */
				{
					WRITECHAR(*where++, n);
				}

				if(given_wd < actual_wd) /* pad on right with spaces (for left justify) */
				{
					given_wd = 0;
				}
				else 
				{
					given_wd -= actual_wd;
				}
					
				for(; given_wd; given_wd--)
				{
					WRITECHAR(' ', n);
				}
				break;
			default:
				break;
			}
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	
	buffer[count] = '\0';
	return (int)count;
}

int puts(const char* str)
{
	file_write(str, strlen(str), stdout);

	putchar('\n');
	return 1;
}

int fputs(const char * str, FILE * stream)
{
	//stream->print_func(stream, str);
	putchar('\n');
	return 1;
}

void perror(const char* str)
{
	//stderr->print_func(stderr, str);
}

int vsprintf(char *s, const char *format, va_list arg)
{
	return vsnprintf(s, 256, format, arg);
}

int snprintf(char* s, size_t n, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	int len = vsnprintf(s, n, format, args);

	va_end(args);

	return len;
}

int sprintf(char* s, const char * format, ...)
{
	va_list args;
	va_start(args, format);
	
	int len = vsnprintf(s, 128, format, args);
	
	va_end(args);
	
	return len;
}

int fprintf(FILE * stream, const char * format, ...)
{
	char buffer[128];
	va_list args;
	va_start(args, format);
	
	int len = vsnprintf(buffer, 128, format, args);
	file_write(buffer, (size_t)len, stream);
	
	va_end(args);
	
	return len;
}

int printf(const char* format, ...)
{
	char buffer[128];
	va_list args;
	va_start(args, format);
	
	int len = vsnprintf(buffer, 128, format, args);
	file_write(buffer, (size_t)len, stdout);

	va_end(args);
	
	return len;
}