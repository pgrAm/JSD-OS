// Much of the code in this file was shamelessly copied from mlibc on June 15, 2022
// Used under MIT License
// https://github.com/managarm/mlibc

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include "internal_file.h"

int fscanf(FILE* __restrict stream, const char* __restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vfscanf(stream, format, args);
	va_end(args);
	return result;
}

namespace
{
enum
{
	SCANF_TYPE_CHAR,
	SCANF_TYPE_SHORT,
	SCANF_TYPE_INTMAX,
	SCANF_TYPE_L,
	SCANF_TYPE_LL,
	SCANF_TYPE_PTRDIFF,
	SCANF_TYPE_SIZE_T,
	SCANF_TYPE_INT
};
}

static void store_int(void* dest, unsigned int size, unsigned long long i)
{
	switch(size)
	{
	case SCANF_TYPE_CHAR:
		*(char*)dest = i;
		break;
	case SCANF_TYPE_SHORT:
		*(short*)dest = i;
		break;
	case SCANF_TYPE_INTMAX:
		*(intmax_t*)dest = i;
		break;
	case SCANF_TYPE_L:
		*(long*)dest = i;
		break;
	case SCANF_TYPE_LL:
		*(long long*)dest = i;
		break;
	case SCANF_TYPE_PTRDIFF:
		*(ptrdiff_t*)dest = i;
		break;
	case SCANF_TYPE_SIZE_T:
		*(size_t*)dest = i;
		break;
	/* fallthrough */
	case SCANF_TYPE_INT:
	default:
		*(int*)dest = i;
		break;
	}
}

struct scanf_handler
{
	int num_consumed = 0;
	virtual char consume() = 0;
	virtual char look_ahead() = 0;
};

static int do_scanf(scanf_handler& handler, const char* fmt, va_list args)
{
	int match_count = 0;
	for(; *fmt; fmt++)
	{
		if(isspace(*fmt))
		{
			while(isspace(fmt[1]))
				fmt++;
			while(isspace(handler.look_ahead()))
				handler.consume();
			continue;
		}

		if(*fmt != '%' || fmt[1] == '%')
		{
			if(*fmt == '%') fmt++;
			char c = handler.consume();
			if(c != *fmt) break;
			continue;
		}

		void* dest = NULL;
		/* %n$ format */
		if(isdigit(*fmt) && fmt[1] == '$')
		{
			/* TODO: dest = get_arg_at_pos(args, *fmt -'0'); */
			fmt += 3;
		}
		else
		{
			if(fmt[1] != '*')
			{
				dest = va_arg(args, void*);
			}
			fmt++;
		}

		int width = 0;
		if(*fmt == '*')
		{
			fmt++;
		}
		else if(*fmt == '\'')
		{
			/* TODO: numeric seperators locale stuff */
			puts("do_scanf: \' not implemented!");
			fmt++;
			continue;
		}
		else if(*fmt == 'm')
		{
			/* TODO: allocate buffer for them */
			puts("do_scanf: m not implemented!");
			fmt++;
			continue;
		}
		else if(*fmt >= '0' && *fmt <= '9')
		{
			/* read in width specifier */
			width = 0;
			while(*fmt >= '0' && *fmt <= '9')
			{
				width = width * 10 + (*fmt - '0');
				fmt++;
				continue;
			}
		}

		/* type modifiers */
		unsigned int type = SCANF_TYPE_INT;
		unsigned int base = 10;
		switch(*fmt)
		{
		case 'h':
		{
			if(fmt[1] == 'h')
			{
				type = SCANF_TYPE_CHAR;
				fmt += 2;
				break;
			}
			type = SCANF_TYPE_SHORT;
			fmt++;
			break;
		}
		case 'j':
		{
			type = SCANF_TYPE_INTMAX;
			fmt++;
			break;
		}
		case 'l':
		{
			if(fmt[1] == 'l')
			{
				type = SCANF_TYPE_LL;
				fmt += 2;
				break;
			}
			type = SCANF_TYPE_L;
			fmt++;
			break;
		}
		case 'L':
		{
			type = SCANF_TYPE_LL;
			fmt++;
			break;
		}
		case 'q':
		{
			type = SCANF_TYPE_LL;
			fmt++;
			break;
		}
		case 't':
		{
			type = SCANF_TYPE_PTRDIFF;
			fmt++;
			break;
		}
		case 'z':
		{
			type = SCANF_TYPE_SIZE_T;
			fmt++;
			break;
		}
		case '0':
		{
			if(fmt[1] == 'x' || fmt[1] == 'X')
			{
				base = 16;
				fmt += 2;
				break;
			}
			base = 8;
			fmt++;
			break;
		}
		}

		switch(*fmt)
		{
		case 'd':
		case 'u':
			base = 10;
			[[fallthrough]];
		case 'i':
		{
			unsigned long long res = 0;
			char c				   = handler.look_ahead();
			switch(base)
			{
			case 10:
				while(c >= '0' && c <= '9')
				{
					handler.consume();
					res = res * 10 + (c - '0');
					c	= handler.look_ahead();
				}
				break;
			case 16:
				if(c == '0')
				{
					handler.consume();
					c = handler.look_ahead();
					if(c == 'x')
					{
						handler.consume();
						c = handler.look_ahead();
					}
				}
				while(true)
				{
					if(c >= '0' && c <= '9')
					{
						handler.consume();
						res = res * 16 + (c - '0');
					}
					else if(c >= 'a' && c <= 'f')
					{
						handler.consume();
						res = res * 16 + (c - 'a');
					}
					else if(c >= 'A' && c <= 'F')
					{
						handler.consume();
						res = res * 16 + (c - 'A');
					}
					else
					{
						break;
					}
					c = handler.look_ahead();
				}
				break;
			case 8:
				while(c >= '0' && c <= '7')
				{
					handler.consume();
					res = res * 10 + (c - '0');
					c	= handler.look_ahead();
				}
				break;
			}
			if(dest) store_int(dest, type, res);
			break;
		}
		case 'o':
		{
			unsigned long long res = 0;
			char c				   = handler.look_ahead();
			while(c >= '0' && c <= '7')
			{
				handler.consume();
				res = res * 10 + (c - '0');
				c	= handler.look_ahead();
			}
			if(dest) store_int(dest, type, res);
			break;
		}
		case 'x':
		case 'X':
		{
			unsigned long long res = 0;
			char c				   = handler.look_ahead();
			if(c == '0')
			{
				handler.consume();
				c = handler.look_ahead();
				if(c == 'x')
				{
					handler.consume();
					c = handler.look_ahead();
				}
			}
			while(true)
			{
				if(c >= '0' && c <= '9')
				{
					handler.consume();
					res = res * 16 + (c - '0');
				}
				else if(c >= 'a' && c <= 'f')
				{
					handler.consume();
					res = res * 16 + (c - 'a');
				}
				else if(c >= 'A' && c <= 'F')
				{
					handler.consume();
					res = res * 16 + (c - 'A');
				}
				else
				{
					break;
				}
				c = handler.look_ahead();
			}
			if(dest) store_int(dest, type, res);
			break;
		}
		case 's':
		{
			char* typed_dest = (char*)dest;
			char c			 = handler.look_ahead();
			int count		 = 0;
			while(c && !isspace(c))
			{
				handler.consume();
				if(typed_dest) typed_dest[count] = c;
				c = handler.look_ahead();
				count++;
				if(width && count >= width) break;
			}
			if(typed_dest) typed_dest[count] = '\0';
			break;
		}
		case 'c':
		{
			char* typed_dest = (char*)dest;
			char c			 = handler.look_ahead();
			int count		 = 0;
			if(!width) width = 1;
			while(c && count < width)
			{
				handler.consume();
				if(typed_dest) typed_dest[count] = c;
				c = handler.look_ahead();
				count++;
			}
			break;
		}
		case '[':
		{
			fmt++;
			int invert = 0;
			if(*fmt == '^')
			{
				invert = 1;
				fmt++;
			}

			char scanset[257];
			memset(&scanset[0], invert, sizeof(char) * 257);
			scanset[0] = '\0';

			if(*fmt == '-')
			{
				fmt++;
				scanset[1 + '-'] = 1 - invert;
			}
			else if(*fmt == ']')
			{
				fmt++;
				scanset[1 + ']'] = 1 - invert;
			}

			for(; *fmt != ']'; fmt++)
			{
				if(!*fmt) return EOF;
				if(*fmt == '-' && *fmt != ']')
				{
					fmt++;
					for(char c = *(fmt - 2); c < *fmt; c++)
						scanset[1 + c] = 1 - invert;
				}
				scanset[1 + *fmt] = 1 - invert;
			}

			char* typed_dest = (char*)dest;
			int count		 = 0;
			char c			 = handler.look_ahead();
			while(c && (!width || count < width))
			{
				handler.consume();
				if(!scanset[1 + c]) break;
				if(typed_dest) typed_dest[count] = c;
				c = handler.look_ahead();
				count++;
			}
			if(typed_dest) typed_dest[count] = '\0';
			break;
		}
		case 'p':
		{
			unsigned long long res = 0;
			char c				   = handler.look_ahead();
			if(c == '0')
			{
				handler.consume();
				c = handler.look_ahead();
				if(c == 'x')
				{
					handler.consume();
					c = handler.look_ahead();
				}
			}
			while(true)
			{
				if(c >= '0' && c <= '9')
				{
					handler.consume();
					res = res * 16 + (c - '0');
				}
				else if(c >= 'a' && c <= 'f')
				{
					handler.consume();
					res = res * 16 + (c - 'a');
				}
				else if(c >= 'A' && c <= 'F')
				{
					handler.consume();
					res = res * 16 + (c - 'A');
				}
				else
				{
					break;
				}
				c = handler.look_ahead();
			}
			void** typed_dest = (void**)dest;
			*typed_dest		  = (void*)(uintptr_t)res;
			break;
		}
		case 'n':
		{
			int* typed_dest = (int*)dest;
			if(typed_dest) *typed_dest = handler.num_consumed;
			continue;
		}
		}
		if(dest) match_count++;
	}
	return match_count;
}

int scanf(const char* __restrict format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vfscanf(stdin, format, args);
	va_end(args);
	return result;
}

int sscanf(const char* __restrict buffer, const char* __restrict format, ...)
{
	return 0;

	va_list args;
	va_start(args, format);

	int result = vsscanf(buffer, format, args);

	va_end(args);
	return result;
}

int vfscanf(FILE* __restrict stream, const char* __restrict format,
			va_list args)
{
	struct file_handler : scanf_handler
	{
		char look_ahead()
		{
			char c;
			size_t actual_size = file->read(&c, 1);
			if(actual_size) file->set_pos(file->get_pos() - 1);
			return actual_size ? c : 0;
		}

		char consume()
		{
			char c;
			size_t actual_size = file->read(&c, 1);
			if(actual_size) num_consumed++;
			return actual_size ? c : 0;
		}

		FILE* file;

		file_handler(FILE* _file) : scanf_handler{}, file{_file}
		{}
	};

	file_handler h{stream};
	return do_scanf(h, format, args);
}

/*int vscanf(const char* __restrict, va_list)
{
	assert(false); //Not implemented
	__builtin_unreachable();
}*/

int vsscanf(const char* __restrict buffer, const char* __restrict format,
			va_list args)
{
	struct raw_handler : scanf_handler
	{
		virtual char look_ahead() override
		{
			return *buffer;
		}

		virtual char consume() override
		{
			num_consumed++;
			return *buffer++;
		}

		const char* buffer;

		raw_handler(const char* _buffer) : scanf_handler{}, buffer{_buffer}
		{}
	};

	raw_handler h{buffer};
	int result = do_scanf(h, format, args);

	return result;
}