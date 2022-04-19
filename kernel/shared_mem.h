#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <kernel/syscall.h>
#include <kernel/memorymanager.h>

#ifdef __cplusplus
extern "C" {
#endif

	SYSCALL_HANDLER uintptr_t create_shared_buffer(const char* name, size_t name_len, size_t size);
	SYSCALL_HANDLER uintptr_t open_shared_buffer(const char* name, size_t name_len);
	SYSCALL_HANDLER void close_shared_buffer(uintptr_t buf_handle);
	SYSCALL_HANDLER void* map_shared_buffer(uintptr_t buf_handle, size_t size, page_flags_t flags);

#ifdef __cplusplus
}
#endif

#endif