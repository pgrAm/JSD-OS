#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef __KERNEL
#include <graphics/graphics.h>
#include <sys/syscalls.h>
#else
#include <kernel/sys/syscalls.h>
#endif

#define		PR_LJ	0x01	// left justify */
#define		PR_CA	0x02	// use A-F instead of a-f for hex */
#define		PR_SG	0x04	// signed numeric conversion (%d vs. %u) */
#define		PR_32	0x08	// long (32-bit) numeric conversion */
#define		PR_16	0x10	// short (16-bit) numeric conversion */
#define		PR_WS	0x20	// PR_SG set and num was < 0 */
#define		PR_LZ	0x40	// pad left with '0' instead of ' ' */
#define		PR_FP	0x80	// pointers are far */

/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define		PR_BUFLEN	16

//char blank_get_func(FILE* stream){return 0;};
//void blank_print_func(FILE* stream, const char* c){};
//void blank_print_len_func(FILE* stream, const char* c, size_t n){};
//void blank_print_c_func(FILE* stream, const char c){};
//
//int blank_close_func(FILE* stream){ return 0;};
//
//char _stdin_get_func(FILE* stream){ return keybuf_pop(); };
//
//void _stdout_print_func(FILE* stream, const char* c){ print_string(c); };
//void _stdout_print_len_func(FILE* stream, const char* c, size_t n){ print_string_len(c, n); };
//void _stdout_print_c_func(FILE* stream, const char c){ print_char(c); };
//
/*struct _internal_FILE
{
	volatile char* buf;
	volatile size_t bufsize;
	volatile size_t readptr;
	volatile size_t writeptr;
};*/
//
//void copy_stream(FILE* dest, FILE* src)
//{
//	dest->eof_indicator		= src->eof_indicator;		
//	dest->error_indicator 	= src->error_indicator; 	
//	dest->data				= src->data;	
//	
//	dest->close_func        = src->close_func;
//	dest->print_func		= src->print_func;		
//	dest->print_len_func	= src->print_len_func;	
//	dest->print_c_func		= src->print_c_func;	
//	dest->get_c_func		= src->get_c_func;		
//}
//
//int fflush(FILE* stream )
//{
//	return 0;
//}
//
//char _rs232_get_func(FILE* stream)
//{ 
//	return rs232_read_byte(stream->data); 
//}
//void _rs232_print_func(FILE* stream, const char* c)
//{ 
//	rs232_send_string(stream->data, c); 
//}
//
//void _rs232_print_len_func(FILE* stream, const char* c, size_t n)
//{ 
//	rs232_send_string_len(stream->data, c, n); 
//}
//
//void _rs232_print_c_func(FILE* stream, const char c)
//{ 
//	rs232_send_byte(stream->data, c); 
//}

//struct _internal_FILE _internal_stdout = 	{
//							.eof_indicator = 0, 	
//							.error_indicator = 0,
//							.print_func = &_stdout_print_func,
//							.print_len_func = &_stdout_print_len_func,
//							.print_c_func = &_stdout_print_c_func,
//							.get_c_func = &blank_get_func,
//							.close_func = &blank_close_func
//						};
//
//struct _internal_FILE _internal_stdin = 	
//{ 	
//	.buf = NULL,
//	.bufsize = 0,
//	.readptr = 0,
//	.writeptr = 0
//};		


//struct _internal_FILE _internal_stderr = 	{ 
//							.eof_indicator = 0, 	
//							.error_indicator = 0,
//							.print_func = &_stdout_print_func,
//							.print_len_func = &_stdout_print_len_func,
//							.print_c_func = &_stdout_print_c_func,
//							.get_c_func = &blank_get_func,
//							.close_func = &blank_close_func
//						};									
//
////template, do not use without setting data
//struct _internal_FILE _internal_serial = 	{ 	
//							.eof_indicator = 0, 
//							.error_indicator = 0,
//							.data = (size_t)NULL,
//							.print_func = &_rs232_print_func,
//							.print_len_func = &_rs232_print_len_func,
//							.print_c_func = &_rs232_print_c_func,
//							.get_c_func = &_rs232_get_func,
//							.close_func = &blank_close_func
//						};
						
//struct _internal_FILE* stdout = 	&_internal_stdout;
//struct _internal_FILE* stdin = &_internal_stdin;
//struct _internal_FILE* stderr = 	&_internal_stderr;

FILE* fopen (const char * filename, const char * mode)
{
	//char* keyword = strtok(filename, ":,");
	//
	////check list of predefined files
	////TODO: make this use a hashtable
	//if(strcmp(keyword, "COM1") == 0)
	//{
	//	FILE* com1 = (FILE*)malloc(sizeof(FILE)); 
	//	
	//	copy_stream(com1, &_internal_serial);
	//	
	//	com1->data = COM1;
	//	
	//	rs232_init(COM1, 38400);
	//	
	//	return com1;
	//}
	//else if(strcmp(keyword, "COM2") == 0)
	//{
	//	FILE* com2 = (FILE*)malloc(sizeof(FILE)); 
	//	
	//	copy_stream(com2, &_internal_serial);
	//	
	//	com2->data = COM2;
	//	
	//	rs232_init(COM2, 38400);
	//	
	//	return com2;
	//}
	//else //look in the current directory
	//{
	//	
	//}
	
	return NULL;
}

void clearerr(FILE* stream)
{
	//stream->error_indicator = 0;
	//stream->eof_indicator = 0;
}

int feof(FILE* stream)
{
	return 0;//(int)stream->eof_indicator;
}

int ferror(FILE* stream)
{
	return 0;//(int)stream->error_indicator;
}
				
char *gets_s(char *str, size_t length) //!safe version of gets, if you provide a valid length it will not overflow
{
	char *c = str;
	
	char* end = c + length - 1;
	
	for(; c < end; c++)
	{
		//*c = stdin->get_c_func(stdin);
		if(*c == '\n') 
		{
			*c = '\0';
			return str;
		}
	}
	
	*(end) = '\0';
	return str;
}

char *gets(char *str) //blatantly unsafe function please do not use: BUFFER OVERFLOW!!!
{
	//char *c = str;
	
	//*c = stdin->get_c_func(stdin);
	//while(*(c++) != '\n')
	//{
	//	*c = stdin->get_c_func(stdin);
	//}
	//*c = '\0';
	
	return str;
}

/*int fputc(int character, FILE * stream)
{
	if(stream == stdin)
	{
		//if(stdin->writeptr < stdin->bufsize)
		//{
		//	stdin->buf[stdin->writeptr++] = character;
		//}
		//else
		//{
		//	stdin->writeptr = 0;
		//	stdin->readptr = 0;
		//}
	}
	
	//stream->print_c_func(stream, (unsigned char)character);
	
	return character;
}*/

int putchar(int character)
{
	char c = (char)character;
	
	print_string(&c, 1);
	
	return character;
}

/*int getc(FILE* stream)
{
	if(stream == stdin)
	{
		return wait_and_getkey();
	}
	
	return 0;//stream->get_c_func(stdin);
}

int getchar()
{
	return getc(stdin);//stdin->get_c_func(stdin);
}*/

#define WRITECHAR(c, m) if(count < (m - 1)) { *ptr++ = c; } count++;

int vsnprintf(char *buffer, size_t n, const char *fmt, va_list args)
{
	unsigned state = 0, flags = 0, radix = 0, actual_wd = 0, count = 0, given_wd = 0;
	unsigned char* where = NULL;
	unsigned char buf[PR_BUFLEN];

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
				given_wd = 10 * given_wd + (*fmt - '0');
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
						num = va_arg(args, unsigned long);
					}
					else if(flags & PR_16) /* h=short=16 bits (signed or unsigned) */
					{
						if(flags & PR_SG)
						{
							num = va_arg(args, int);
						}
						else
						{
							num = va_arg(args, unsigned int);
						}
					}
					else /* no h nor l: sizeof(int) bits (signed or unsigned) */
					{
						if(flags & PR_SG)
						{
							num = va_arg(args, int);
						}
						else
						{
							num = va_arg(args, unsigned int);
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

					do /* convert binary to octal/decimal/hex ASCII. The math here is _always_ unsigned */
					{
						unsigned long temp;

						temp = (unsigned long)num % radix;
						where--;
						if(temp < 10)
						{
							*where = temp + '0';
						}
						else if(flags & PR_CA)
						{
							*where = temp - 10 + 'A';
						}
						else
						{
							*where = temp - 10 + 'a';
						}
						num = (unsigned long)num / radix;
					} while(num != 0);
				}
				goto EMIT;
			case 'c':
				flags &= ~PR_LZ; /* disallow pad-left-with-zeroes for %c */
				where--;
				*where = (unsigned char)va_arg(args, unsigned int);
				actual_wd = 1;
				goto EMIT2;
			case 's':
				flags &= ~PR_LZ; /* disallow pad-left-with-zeroes for %s */
				where = va_arg(args, unsigned char *);
EMIT:
				actual_wd = strlen((char*)where);
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
	return count;
}

int puts(const char* str)
{
	//stdout->print_func(stdout, str);
	print_string(str, strlen(str));
	
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
	//stream->print_len_func(stream, buffer, len);
	
	va_end(args);
	
	return len;
}

int printf(const char* format, ...)
{
	char buffer[128];
	va_list args;
	va_start(args, format);
	
	int len = vsnprintf(buffer, 128, format, args);
	//stdout->print_len_func(stdout, buffer, len);
	print_string(buffer, len);
	
	va_end(args);
	
	return len;
}

int fclose(FILE* stream)
{
	return 0;//stream->close_func(stream);
}