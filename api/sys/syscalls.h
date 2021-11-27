#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <files.h>
#include <virtual_keys.h>
#include <common/display_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WAIT_FOR_PROCESS 0x01

enum syscall_indices
{
	SYSCALL_FIND_PATH = 0,
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
	SYSCALL_OPEN_FILE_HANDLE = 13,
	SYSCALL_OPEN_DIR_HANDLE = 14,
	SYSCALL_GET_FILE_IN_DIR = 15,
	SYSCALL_GET_FILE_INFO = 16,
	SYSCALL_GET_ROOT_DIR = 17,
	SYSCALL_SET_DISPLAY_MODE = 18,
	SYSCALL_MAP_DISPLAY_MEMORY = 19,
	SYSCALL_GET_DISPLAY_MODE = 20,
	SYSCALL_CLOSE_DIR = 21,
	SYSCALL_SET_DISPLAY_CURSOR = 22,
	SYSCALL_GET_KEYSTATE = 23,
	SYSCALL_GET_FREE_MEM = 24,
	SYSCALL_IOPL = 25,
	SYSCALL_SET_DISPLAY_OFFSET = 26
};

struct file_handle;
typedef struct file_handle file_handle;
struct directory_handle;
typedef struct directory_handle directory_handle;
struct file_stream;
typedef struct file_stream file_stream;

static inline uint32_t do_syscall_4(size_t syscall_index, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
	uint32_t retval;
	__asm__ volatile("int $0x80"
					 :"=a"(retval), "+c"(arg1), "+d"(arg2), "+D"(arg3)
					 : "b"(syscall_index), "a"(arg0)
					 : "memory");
	return retval;
}

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

static inline void iopl(int a)
{
	do_syscall_1(SYSCALL_IOPL, (uint32_t)a);
}

static inline void sys_exit(int a)
{
	do_syscall_1(SYSCALL_EXIT, (uint32_t)a);
}

static inline file_stream* open(directory_handle* rel, const char* path, size_t path_len, int flags)
{
	return (file_stream*)do_syscall_4(SYSCALL_OPEN, (uint32_t)rel, (uint32_t)path, (uint32_t)path_len, (uint32_t)flags);
}

static inline int close(file_stream* file)
{
	return (int)do_syscall_1(SYSCALL_CLOSE, (uint32_t)file);
}

static inline int read(void* dst, size_t len, file_stream* file)
{
	return (int)do_syscall_3(SYSCALL_READ, (uint32_t)dst, (uint32_t)len, (uint32_t)file);
}

static inline void spawn_process(const char* path, size_t path_len, int flags)
{
	do_syscall_3(SYSCALL_SPAWN, (uint32_t)path, (uint32_t)path_len, (uint32_t)flags);
}

static inline time_t master_time()
{
	return (time_t)do_syscall_0(SYSCALL_TIME);
}

static inline size_t get_free_memory()
{
	return (size_t)do_syscall_0(SYSCALL_GET_FREE_MEM);
}

static inline clock_t clock_ticks(size_t* rate)
{
	return (clock_t)do_syscall_1(SYSCALL_TICKS, (uint32_t)rate);
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

static inline key_type getkey()
{
	return (key_type)do_syscall_0(SYSCALL_KEYGET);
}

static inline key_type wait_and_getkey()
{
	return (key_type)do_syscall_0(SYSCALL_WAIT_KEYGET);
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

static inline int set_display_mode(display_mode* requested, display_mode* actual)
{
	return (int)do_syscall_2(SYSCALL_SET_DISPLAY_MODE, (uint32_t)requested, (uint32_t)actual);
}

static inline int get_display_mode(int index, display_mode* actual)
{
	return (int)do_syscall_2(SYSCALL_GET_DISPLAY_MODE, (uint32_t)index, (uint32_t)actual);
}

static inline int set_display_cursor(int offset)
{
	return (int)do_syscall_1(SYSCALL_SET_DISPLAY_CURSOR, (uint32_t)offset);
}

static inline uint8_t* map_display_memory()
{
	return (uint8_t*)do_syscall_0(SYSCALL_MAP_DISPLAY_MEMORY);
}

static inline int close_dir(directory_handle* dir) 
{
	return (int)do_syscall_1(SYSCALL_CLOSE_DIR, (uint32_t)dir);
}

static inline file_handle* find_path(const directory_handle* rel, const char* name, size_t path_len)
{
	return (file_handle*)do_syscall_3(SYSCALL_FIND_PATH, (uint32_t)rel, (uint32_t)name, (uint32_t)path_len);
}

static inline int set_display_offset(size_t offset, int on_retrace)
{
	return (int)do_syscall_2(SYSCALL_SET_DISPLAY_OFFSET, (uint32_t)offset, (uint32_t)on_retrace);
}

static inline directory_handle* open_dir_handle(const file_handle* f, int flags)
{
	return (directory_handle*)do_syscall_2(SYSCALL_OPEN_DIR_HANDLE, (uint32_t)f, (uint32_t)flags);
}

static inline file_stream* open_file_handle(file_handle* f, int flags)
{
	return (file_stream*)do_syscall_2(SYSCALL_OPEN_FILE_HANDLE, (uint32_t)f, (uint32_t)flags);
}

static inline int get_keystate(key_type key)
{
	return (int)do_syscall_1(SYSCALL_GET_KEYSTATE, (uint32_t)key);
}

static inline directory_handle* open_dir(directory_handle* rel, const char* path, size_t path_len, int flags)
{
	file_handle* f = find_path(rel, path, path_len);
	return open_dir_handle(f, flags);
}

#ifdef __cplusplus
}
#endif
#endif