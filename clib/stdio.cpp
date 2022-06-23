#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef __KERNEL
#	include <sys/syscalls.h>
#	include <bit>
#else
#	include <kernel/sys/syscalls.h>
#endif

#define PR_LJ 0x01u // left justify */
#define PR_CA 0x02u // use A-F instead of a-f for hex */
#define PR_SG 0x04u // signed numeric conversion (%d vs. %u) */
#define PR_32 0x08u // long (32-bit) numeric conversion */
#define PR_16 0x10u // short (16-bit) numeric conversion */
#define PR_WS 0x20u // PR_SG set and num was < 0 */
#define PR_LZ 0x40u // pad left with '0' instead of ' ' */
#define PR_FP 0x80u // pointers are far */

/* largest number handled is 2^32-1, lowest radix handled is 8.
2^32-1 in base 8 has 11 digits (add 5 for trailing NUL and for slop) */
#define PR_BUFLEN 16

#ifndef __KERNEL

#	include <string_view>
#	include "internal_file.h"

static size_t write_stub(const char*, size_t, FILE*)
{
	return 0;
};
static size_t read_stub(char*, size_t, FILE*)
{
	return 0;
};
static file_size_t tell_sub(FILE*)
{
	return 0;
};
static int seek_sub(file_size_t, FILE*)
{
	return 0;
};
static int close_stub(FILE*)
{
	return 0;
};

static FILE m_stdout = {write_stub, read_stub, tell_sub,
						seek_sub,	tell_sub,  close_stub};
static FILE m_stdin	 = {write_stub, read_stub, tell_sub,
						seek_sub,	tell_sub,  close_stub};

constinit FILE* stdin  = &m_stdin;
constinit FILE* stdout = &m_stdout;
constinit FILE* stderr = &m_stdout;

void set_stdout(size_t (*write)(const char* buf, size_t size, void* impl))
{
	m_stdout.m_write = std::bit_cast<decltype(m_stdin.m_write)>(write);
}
void set_stdin(size_t (*read)(char* buf, size_t size, void* impl))
{
	m_stdin.m_read = std::bit_cast<decltype(m_stdin.m_read)>(read);
}

static std::string_view s_cwd;

void set_cwd(const char* cwd, size_t cwd_len)
{
	s_cwd = std::string_view{cwd, cwd_len};
}

struct fs_file : FILE
{
	file_size_t offset;
	file_info info;
	file_stream* file;
};

long int ftell(FILE* stream)
{
	return static_cast<long int>(stream->get_pos());
}

int fseek(FILE* stream, long int offset, int origin)
{
	switch(origin)
	{
	case SEEK_SET:
		return stream->set_pos(offset);
	case SEEK_CUR:
		return stream->set_pos(stream->get_pos() + offset);
	case SEEK_END:
		return stream->set_pos(stream->end() + offset);
	}

	return -1;
}

int fflush(FILE* stream)
{
	return 0;
}

FILE* fopen(const char* file, const char* mode)
{
	using namespace std::literals;

	if(!mode)
	{
		return nullptr;
	}

	std::string_view mode_str{mode};

	if(mode_str.ends_with('b'))
		mode_str = mode_str.substr(0, mode_str.size() - 1);

	bool append	   = false;
	int mode_flags = 0;

	if(mode_str == "w"sv)
		mode_flags = FILE_CREATE | FILE_WRITE;
	else if(mode_str == "a"sv)
	{
		append	   = true;
		mode_flags = FILE_CREATE | FILE_WRITE;
	}
	else if(mode_str == "r+"sv)
		mode_flags = FILE_READ | FILE_WRITE;
	else if(mode_str == "w+"sv)
		mode_flags = FILE_CREATE | FILE_READ | FILE_WRITE;
	else if(mode_str == "a+"sv)
	{
		append	   = true;
		mode_flags = FILE_READ;
	}
	else if(mode_str == "r"sv)
	{
		mode_flags = FILE_READ;
	}

	auto cwd_dir = s_cwd.empty()
					   ? nullptr
					   : open_dir(nullptr, s_cwd.data(), s_cwd.size(), 0);

	auto handle = find_path(cwd_dir, file, strlen(file), mode_flags, 0);

	if(cwd_dir)
	{
		close_dir(cwd_dir);
	}

	if(!handle) return nullptr;

	auto file_stream = open_file_handle(handle, mode_flags);
	if(!file_stream)
	{
		dispose_file_handle(handle);
		return nullptr;
	}

	fs_file* state = (fs_file*)malloc(sizeof(fs_file));

	get_file_info(&state->info, handle);
	state->file = file_stream;

	dispose_file_handle(handle);

	state->offset  = append ? state->info.size : 0;
	state->m_write = [](const char* buf, size_t size, FILE* impl)
	{
		auto f	 = static_cast<fs_file*>(impl);
		auto amt = write(f->offset, buf, size, f->file);
		f->offset += amt;
		return (size_t)amt;
	};
	state->m_read = [](char* buf, size_t size, FILE* impl)
	{
		auto f	 = static_cast<fs_file*>(impl);
		auto amt = read(f->offset, buf, size, f->file);
		f->offset += amt;
		return (size_t)amt;
	};
	state->m_set_pos = [](file_size_t position, FILE* impl)
	{
		auto f	  = static_cast<fs_file*>(impl);
		f->offset = position;
		return 0;
	};
	state->m_get_pos = [](FILE* impl)
	{
		auto f = static_cast<fs_file*>(impl);
		return f->offset;
	};
	state->m_close = [](FILE* impl)
	{
		auto f	 = static_cast<fs_file*>(impl);
		auto ret = close(f->file);
		free(f);
		return ret;
	};
	state->m_end = [](FILE* impl)
	{ return static_cast<fs_file*>(impl)->info.size; };

	return state;
}

static size_t file_write(const char* buf, size_t size, FILE* stream)
{
	return stream->write(buf, size);
}

size_t fread(void* ptr, size_t size, size_t count, FILE* stream)
{
	return stream->read((char*)ptr, size * count);
}

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream)
{
	return stream->write((const char*)ptr, size * count);
}

int fclose(FILE* stream)
{
	return stream->close();
}
#else
static FILE* stdout = nullptr;

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

#define WRITECHAR(c, m) \
	if(count < (m - 1)) \
	{                   \
		*ptr++ = c;     \
	}                   \
	count++;

int vsnprintf(char* buffer, size_t n, const char* fmt, va_list args)
{
	unsigned state = 0, flags = 0, radix = 0, actual_wd = 0, count = 0,
			 given_wd = 0;
	char* where		  = nullptr;
	char buf[PR_BUFLEN];

	char* ptr = buffer;

	for(; *fmt; fmt++) /* begin scanning format specifier list */
	{
		switch(state)
		{
		case 0:				/* STATE 0: AWAITING % */
			if(*fmt != '%') /* not %... */
			{
				WRITECHAR(*fmt, n);
				break;
			}
			state++; /* found %, get next char and advance state to check if next char is a flag */
			fmt++;
			/* FALL THROUGH */
		case 1:				/* STATE 1: AWAITING FLAGS (%-0) */
			if(*fmt == '%') /* %% */
			{
				WRITECHAR(*fmt, n);
				state = flags = given_wd = 0;
				break;
			}
			else if(*fmt == '-')
			{
				if(flags & PR_LJ) /* %-- is illegal */
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

			if(*fmt == '0' || *fmt == '.') /* check now for '%0...' */
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
		case 3:		 /* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
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
			where  = buf + PR_BUFLEN - 1;
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
			DO_NUM: /* load the value to be printed. l=long=32 bits: */
			{
				long num = 0;
				if(flags & PR_32)
				{
					num = (long)va_arg(args, unsigned long);
				}
				else if(flags &
						PR_16) /* h=short=16 bits (signed or unsigned) */
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
				*where	  = (char)va_arg(args, unsigned int);
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

				if((flags & (PR_WS | PR_LZ)) ==
				   (PR_WS |
					PR_LZ)) /* if we pad left with ZEROES, do the sign now */
				{
					WRITECHAR('-', n);
				}

			EMIT2: /* pad on left with spaces or zeroes (for right justify) */
				if((flags & PR_LJ) == 0)
				{
					while(given_wd > actual_wd)
					{
						WRITECHAR(flags & PR_LZ ? '0' : ' ', n);
						given_wd--;
					}
				}

				if((flags & (PR_WS | PR_LZ)) ==
				   PR_WS) /* if we pad left with SPACES, do the sign now */
				{
					WRITECHAR('-', n);
				}

				while(*where != '\0') /* emit string/char/converted number */
				{
					WRITECHAR(*where++, n);
				}

				if(given_wd <
				   actual_wd) /* pad on right with spaces (for left justify) */
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

int fputs(const char* str, FILE* stream)
{
	//stream->print_func(stream, str);
	putchar('\n');
	return 1;
}

void perror(const char* str)
{
	//stderr->print_func(stderr, str);
}

int vfprintf(FILE* stream, const char* format, va_list args)
{
	char buffer[128];

	int len = vsnprintf(buffer, 128, format, args);
	file_write(buffer, (size_t)len, stream);

	return len;
}

int vsprintf(char* s, const char* format, va_list arg)
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

int sprintf(char* s, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	int len = vsnprintf(s, 128, format, args);

	va_end(args);

	return len;
}

int fprintf(FILE* stream, const char* format, ...)
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

#ifndef __KERNEL

int remove(const char* filename)
{
	const file_handle* f = find_path(nullptr, filename, strlen(filename), 0, 0);
	if(f)
	{
		delete_file(f);
		dispose_file_handle(f);
		return 0;
	}

	return -1;
}

int rename(const char* old_path, const char* new_path)
{
	const file_handle* old_handle =
		find_path(nullptr, old_path, strlen(old_path), 0, 0);
	if(old_handle)
	{
		file_info f;
		if(get_file_info(&f, old_handle) == 0 && !(f.flags & IS_DIR))
		{
			file_stream* old_file = open_file_handle(old_handle, 0);
			file_stream* new_file = open(nullptr, new_path, strlen(new_path),
										 FILE_WRITE | FILE_CREATE);
			if(new_file)
			{
				char* dataBuf = (char*)malloc(f.size);

				read(0, dataBuf, f.size, old_file);
				write(0, dataBuf, f.size, new_file);

				free(dataBuf);

				delete_file(old_handle);
			}
		}

		dispose_file_handle(old_handle);
		return 0;
	}

	return -1;
}
#endif
