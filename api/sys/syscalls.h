#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <files.h>

#define WAIT_FOR_PROCESS 0x01

enum syscall_indices
{
	SYSCALL_PRINT = 0,
	SYSCALL_OPEN = 1,
	SYSCALL_CLOSE = 2,
	SYSCALL_READ = 3,
	SYSCALL_EXIT = 4,
	SYSCALL_SPAWN = 5,
	SYSCALL_TIME = 6,
	SYSCALL_TICKS = 7,
	SYSCALL_TIMEZONE = 8,
	SYSCALL_ALLOC_PAGES = 9,
	SYSCALL_FREE_PAGES = 10,
	SYSCALL_KEYGET = 11,
	SYSCALL_WAIT_KEYGET = 12,
	SYSCALL_CLEAR_SCREEN = 13, //these can be axed later
	SYSCALL_ERASE_CHARS = 14, //these can be axed later
	SYSCALL_GET_FILE_IN_DIR = 15,
	SYSCALL_GET_FILE_INFO = 16,
	SYSCALL_GET_ROOT_DIR = 17,
	SYSCALL_SET_VIDEO_MODE = 18,
	SYSCALL_MAP_VIDEO_MEMORY = 19
};

struct file_handle;
typedef struct file_handle file_handle;
struct directory_handle;
typedef struct directory_handle directory_handle;
struct file_stream;
typedef struct file_stream file_stream;

static inline uint32_t do_syscall_3(size_t syscall_index, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval), "+c"(arg1), "+d"(arg2)
						:"b"(syscall_index), "a"(arg0)
						:"memory");
	return retval;
}

static inline uint32_t do_syscall_2(size_t syscall_index, uint32_t arg0, uint32_t arg1)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval), "+c"(arg1)
						:"b"(syscall_index), "a"(arg0)
						:"%edx", "memory");
	return retval;
}

//"int 0x80" parm[ebx] [eax] value [eax] modifies [ecx edx];
static inline uint32_t do_syscall_1(size_t syscall_index, const uint32_t arg)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval)
						:"b"(syscall_index), "a"(arg)
						:"%ecx", "%edx", "memory");
	return retval;
}

static inline uint32_t do_syscall_0(size_t syscall_index)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval)
						:"b"(syscall_index)
						:"%ecx", "%edx", "memory");
	return retval;
}

static inline void exit(int a)
{
	do_syscall_1(SYSCALL_EXIT, (uint32_t)a);
}

static inline void print_string(const char *a, size_t len)
{
	do_syscall_2(SYSCALL_PRINT, (uint32_t)a, (uint32_t)len);
}

static inline file_stream* open(const char *path, int flags)
{
	return (file_stream*)do_syscall_2(SYSCALL_OPEN, (uint32_t)path, (uint32_t)flags);
}

static inline int close(file_stream* file)
{
	return (int)do_syscall_1(SYSCALL_CLOSE, (uint32_t)file);
}

static inline int read(void* dst, size_t len, file_stream* file)
{
	return (int)do_syscall_3(SYSCALL_READ, (uint32_t)dst, (uint32_t)len, (uint32_t)file);
}

static inline void spawn_process(const char* path, int flags)
{
	do_syscall_2(SYSCALL_SPAWN, (uint32_t)path, (uint32_t)flags);
}

static inline time_t master_time()
{
	return (time_t)do_syscall_0(SYSCALL_TIME);
}

static inline clock_t clock_ticks()
{
	return (clock_t)do_syscall_0(SYSCALL_TICKS);
}

static inline int get_utc_offset()
{
	return (int)do_syscall_0(SYSCALL_TIMEZONE);
}

#define PAGE_PRESENT 0x01
#define PAGE_RW 0x02
#define PAGE_USER 0x04

static inline void* alloc_pages(void *p, size_t n, int flags)
{
	return (void*)do_syscall_3(SYSCALL_ALLOC_PAGES, (uint32_t)p, (uint32_t)n, (uint32_t)flags);
}

static inline int free_pages(void *p, size_t n)
{
	return (int)do_syscall_2(SYSCALL_FREE_PAGES, (uint32_t)p, (uint32_t)n);
}

static inline uint8_t getkey()
{
	return (uint8_t)do_syscall_0(SYSCALL_KEYGET);
}

static inline uint8_t wait_and_getkey()
{
	return (uint8_t)do_syscall_0(SYSCALL_WAIT_KEYGET);
}

static inline void video_clear()
{
	do_syscall_0(SYSCALL_CLEAR_SCREEN);
}

static inline void video_erase_chars(size_t n)
{
	do_syscall_1(SYSCALL_ERASE_CHARS, (uint32_t)n);
}

static inline file_handle* get_file_in_dir(const directory_handle* d, size_t index)
{
	return (file_handle*)do_syscall_2(SYSCALL_GET_FILE_IN_DIR, (uint32_t)d, (uint32_t)index);
}

static inline int get_file_info(file_info* dst, const file_handle* src)
{
	return (int)do_syscall_2(SYSCALL_GET_FILE_INFO, (uint32_t)dst, (uint32_t)src);
}

static inline directory_handle* get_root_directory(size_t drive_index)
{
	return (directory_handle*)do_syscall_1(SYSCALL_GET_ROOT_DIR, (uint32_t)drive_index);
}

#define VIDEO_8BPP 0x02
#define VIDEO_TEXT_MODE 0x01
static inline int set_video_mode(int width, int height, int flags)
{
	return (int)do_syscall_3(SYSCALL_SET_VIDEO_MODE, (uint32_t)width, (uint32_t)height, (uint32_t)flags);
}

static inline uint8_t* map_video_memory()
{
	return (uint8_t*)do_syscall_0(SYSCALL_MAP_VIDEO_MEMORY);
}
#endif