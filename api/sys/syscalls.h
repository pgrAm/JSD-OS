#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <files.h>
#include <virtual_keys.h>
#include <common/display_mode.h>
#include <common/input_event.h>
#include <common/task_data.h>

#ifdef __cplusplus
extern "C" {

typedef bool booltype_t;
#else
typedef _Bool booltype_t;
#endif

enum syscall_indices
{
	SYSCALL_FIND_PATH			 = 0,
	SYSCALL_OPEN				 = 1,
	SYSCALL_CLOSE				 = 2,
	SYSCALL_READ				 = 3,
	SYSCALL_EXIT				 = 4,
	SYSCALL_SPAWN				 = 5,
	SYSCALL_TIME				 = 6,
	SYSCALL_TICKS				 = 7,
	SYSCALL_TIMEZONE			 = 8,
	SYSCALL_ALLOC_PAGES			 = 9,
	SYSCALL_FREE_PAGES			 = 10,
	SYSCALL_UNMAP_PAGES			 = 11,
	SYSCALL_DELETE_FILE			 = 12,
	SYSCALL_OPEN_FILE_HANDLE	 = 13,
	SYSCALL_OPEN_DIR_HANDLE		 = 14,
	SYSCALL_GET_FILE_IN_DIR		 = 15,
	SYSCALL_GET_FILE_INFO		 = 16,
	SYSCALL_GET_ROOT_DIR		 = 17,
	SYSCALL_SET_DISPLAY_MODE	 = 18,
	SYSCALL_MAP_DISPLAY_MEMORY	 = 19,
	SYSCALL_GET_DISPLAY_MODE	 = 20,
	SYSCALL_CLOSE_DIR			 = 21,
	SYSCALL_SET_DISPLAY_CURSOR	 = 22,
	SYSCALL_GET_KEYSTATE		 = 23,
	SYSCALL_GET_FREE_MEM		 = 24,
	SYSCALL_IOPL				 = 25,
	SYSCALL_SET_DISPLAY_OFFSET	 = 26,
	SYSCALL_GET_INPUT_EVENT		 = 27,
	SYSCALL_WRITE				 = 28,
	SYSCALL_DISPOSE_FILE_HANDLE	 = 29,
	SYSCALL_CREATE_SHARED_BUFFER = 30,
	SYSCALL_OPEN_SHARED_BUFFER	 = 31,
	SYSCALL_CLOSE_SHARED_BUFFER	 = 32,
	SYSCALL_MAP_SHARED_BUFFER	 = 33,
	SYSCALL_SPAWN_THREAD		 = 34,
	SYSCALL_EXIT_THREAD			 = 35,
	SYSCALL_YIELD				 = 36,
	SYSCALL_LOAD_DRIVER			 = 37,
	SYSCALL_DIAGNOSTIC_MESSAGE	 = 38,
	SYSCALL_CURRENT_PROCESS_INFO = 39,
	SYSCALL_SET_TLS_ADDR		 = 40,
};

struct file_handle;
typedef struct file_handle file_handle;
struct directory_stream;
typedef struct directory_stream directory_stream;
struct file_stream;
typedef struct file_stream file_stream;

static inline uint32_t do_syscall_5(size_t syscall_index, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
	uint32_t retval;
	__asm__ volatile("int $0x80"
					 :"=a"(retval), "+c"(arg1), "+d"(arg2), "+D"(arg3), "+S"(arg4)
					 : "b"(syscall_index), "a"(arg0)
					 : "memory");
	return retval;
}

static inline uint64_t do_syscall_4l_0l(size_t syscall_index, uint64_t arg0,
										uint32_t arg1, uint32_t arg2,
										uint32_t arg3)
{
	uint32_t lretval, hretval;
	__asm__ volatile("int $0x80"
					 : "=a"(lretval), "=c"(hretval), "+d"(arg1), "+D"(arg2),
					   "+S"(arg3)
					 : "b"(syscall_index), "a"((uint32_t)arg0),
					   "c"((uint32_t)(arg0 >> 32))
					 : "memory");
	return lretval | ((uint64_t)hretval << 32);
}


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

static inline uint64_t do_syscall_1l(size_t syscall_index, const uint32_t arg)
{
	uint32_t lretval, hretval;
	__asm__ volatile("int $0x80"
					 : "=a"(lretval), "=c"(hretval)
					 : "b"(syscall_index), "a"(arg)
					 : "%edx", "memory");
	return lretval | ((uint64_t)hretval << 32);
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

static inline uint64_t do_syscall_0l(size_t syscall_index)
{
	uint32_t lretval, hretval;
	__asm__ volatile("int $0x80"
					 : "=a"(lretval), "=c"(hretval)
					 : "b"(syscall_index)
					 : "%edx", "memory");
	return lretval | ((uint64_t)hretval << 32);
}

static inline void iopl(int a)
{
	do_syscall_1(SYSCALL_IOPL, (uint32_t)a);
}

static inline void sys_exit(int a)
{
	do_syscall_1(SYSCALL_EXIT, (uint32_t)a);
}

static inline file_stream* open(directory_stream* rel, const char* path, size_t path_len, int flags)
{
	return (file_stream*)do_syscall_4(SYSCALL_OPEN, 
									  (uint32_t)rel, (uint32_t)path, (uint32_t)path_len, (uint32_t)flags);
}

static inline int close(file_stream* file)
{
	return (int)do_syscall_1(SYSCALL_CLOSE, (uint32_t)file);
}

static inline file_size_t read(file_size_t offset, void* dst, size_t len,
							   file_stream* file)
{
	return (file_size_t)do_syscall_4l_0l(SYSCALL_READ, (uint64_t)offset,
										 (uint32_t)dst, (uint32_t)len,
										 (uint32_t)file);
}

static inline file_size_t write(file_size_t offset, const void* dst, size_t len,
								file_stream* file)
{
	return (file_size_t)do_syscall_4l_0l(SYSCALL_WRITE, (uint64_t)offset,
										 (uint32_t)dst, (uint32_t)len,
										 (uint32_t)file);
}

static inline task_id spawn_process(const file_handle* file,
									directory_stream* cwd, int flags)
{
	return (task_id)do_syscall_3(SYSCALL_SPAWN, (uint32_t)file, (uint32_t)cwd,
								 (uint32_t)flags);
}

static inline time_t master_time()
{
	return (time_t)do_syscall_0l(SYSCALL_TIME);
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

#define PAGE_SIZE 4096
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

static inline int unmap_pages(void* p, size_t n)
{
	return (int)do_syscall_2(SYSCALL_FREE_PAGES, (uint32_t)p, (uint32_t)n);
}

static inline const file_handle* get_file_in_dir(const directory_stream* d, size_t index)
{
	return (const file_handle*)do_syscall_2(SYSCALL_GET_FILE_IN_DIR, (uint32_t)d, (uint32_t)index);
}

static inline int get_file_info(file_info* dst, const file_handle* src)
{
	return (int)do_syscall_2(SYSCALL_GET_FILE_INFO, (uint32_t)dst, (uint32_t)src);
}

static inline const file_handle* get_root_directory(size_t drive_index)
{
	return (const file_handle*)do_syscall_1(SYSCALL_GET_ROOT_DIR, (uint32_t)drive_index);
}

static inline int set_display_mode(const display_mode* requested, display_mode* actual)
{
	return (int)do_syscall_2(SYSCALL_SET_DISPLAY_MODE, (uint32_t)requested, (uint32_t)actual);
}

static inline int get_display_mode(int index, display_mode* actual)
{
	return (int)do_syscall_2(SYSCALL_GET_DISPLAY_MODE, (uint32_t)index, (uint32_t)actual);
}

static inline int set_display_cursor(size_t offset)
{
	return (int)do_syscall_1(SYSCALL_SET_DISPLAY_CURSOR, (uint32_t)offset);
}

static inline uint8_t* map_display_memory()
{
	return (uint8_t*)do_syscall_0(SYSCALL_MAP_DISPLAY_MEMORY);
}

static inline int close_dir(directory_stream* dir) 
{
	return (int)do_syscall_1(SYSCALL_CLOSE_DIR, (uint32_t)dir);
}

static inline int delete_file(const file_handle* f)
{
	return (int)do_syscall_1(SYSCALL_DELETE_FILE, (uint32_t)f);
}

static inline int dispose_file_handle(const file_handle *f)
{
	return (int)do_syscall_1(SYSCALL_DISPOSE_FILE_HANDLE, (uint32_t)f);
}

static inline const file_handle* find_path(directory_stream* rel, const char* name, size_t path_len, int mode, int flags)
{
	return (const file_handle*)do_syscall_5(SYSCALL_FIND_PATH, 
											(uint32_t)rel, (uint32_t)name, (uint32_t)path_len, (uint32_t)mode, (uint32_t)flags);
}

static inline int set_display_offset(size_t offset, int on_retrace)
{
	return (int)do_syscall_2(SYSCALL_SET_DISPLAY_OFFSET, (uint32_t)offset, (uint32_t)on_retrace);
}

static inline directory_stream* open_dir_handle(const file_handle* f, int flags)
{
	return (directory_stream*)do_syscall_2(SYSCALL_OPEN_DIR_HANDLE, (uint32_t)f, (uint32_t)flags);
}

static inline file_stream* open_file_handle(const file_handle* f, int flags)
{
	return (file_stream*)do_syscall_2(SYSCALL_OPEN_FILE_HANDLE, (uint32_t)f, (uint32_t)flags);
}

static inline int get_keystate(key_type key)
{
	return (int)do_syscall_1(SYSCALL_GET_KEYSTATE, (uint32_t)key);
}

static inline directory_stream* open_dir(directory_stream* rel, const char* path, size_t path_len, int mode)
{
	const file_handle* f = find_path(rel, path, path_len, mode, IS_DIR);
	directory_stream* ds = open_dir_handle(f, mode);
	dispose_file_handle(f);
	return ds;
}

static inline int get_input_event(input_event* e, booltype_t wait)
{
	return (int)do_syscall_2(SYSCALL_GET_INPUT_EVENT, (uint32_t)e, (uint32_t)wait);
}

static inline uintptr_t create_shared_buffer(const char* name, size_t name_len,
											 size_t size)
{
	return (uintptr_t)do_syscall_3(SYSCALL_CREATE_SHARED_BUFFER, (uint32_t)name,
								   (uint32_t)name_len, (uint32_t)size);
}

static inline uintptr_t open_shared_buffer(const char* name, size_t name_len)
{
	return (uintptr_t)do_syscall_2(SYSCALL_OPEN_SHARED_BUFFER, (uint32_t)name,
								   (uint32_t)name_len);
}

static inline void close_shared_buffer(uintptr_t buf_handle)
{
	do_syscall_1(SYSCALL_CLOSE_SHARED_BUFFER, buf_handle);
}

static inline void* map_shared_buffer(uintptr_t buf_handle, size_t size, int flags)
{
	return (void*)do_syscall_3(SYSCALL_MAP_SHARED_BUFFER,
							   (uint32_t)buf_handle, (uint32_t)size, (uint32_t)flags);
}

static inline task_id sys_spawn_thread(void (*func)(task_id), void* tls_ptr)
{
	return (task_id)do_syscall_2(SYSCALL_SPAWN_THREAD, (uintptr_t)func, (uintptr_t)tls_ptr);
}

static inline void exit_thread(int code)
{
	do_syscall_1(SYSCALL_EXIT_THREAD, (uintptr_t)code);
}

static inline void yield_to(task_id id)
{
	do_syscall_1(SYSCALL_YIELD, (uintptr_t)id);
}

static inline int load_driver(directory_stream* rel, const file_handle* file)
{
	return (int)do_syscall_2(SYSCALL_LOAD_DRIVER, (uintptr_t)rel, (uintptr_t)file);
}

static inline void diagnostic_message(const char* data, size_t len)
{
	do_syscall_2(SYSCALL_DIAGNOSTIC_MESSAGE, (uintptr_t)data, (uintptr_t)len);
}

static inline void get_process_info(process_info* data)
{
	do_syscall_1(SYSCALL_CURRENT_PROCESS_INFO, (uintptr_t)data);
}

static inline void set_tls_addr(void* tls_ptr)
{
	do_syscall_1(SYSCALL_SET_TLS_ADDR, (uintptr_t)tls_ptr);
}


#ifdef __cplusplus
}
#endif
#endif