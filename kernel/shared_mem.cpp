#include <kernel/util/hash.h>
#include <kernel/locks.h>
#include <kernel/physical_manager.h>
#include <kernel/memorymanager.h>

#include <kernel/shared_mem.h>

struct shared_buffer
{
	std::string name;
	sync::mutex mtx;
	uintptr_t physical;
	size_t num_pages;
	size_t size;
	size_t num_refs;
};

using shared_map = hash_map<std::string, shared_buffer>;
using buf_map = hash_map<void*, shared_buffer*>;

constinit sync::shared_mutex map_mtx;
shared_map shared_buffers;

SYSCALL_HANDLER uintptr_t create_shared_buffer(const char* name_data, size_t name_len, size_t size)
{
	std::string_view name{name_data, name_len};

	sync::unique_lock map_lock{map_mtx};

	auto data = shared_buffers.emplace(name);
	if(!data)
		return 0; //could not create new buffer, likely already exists

	sync::lock_guard l{data->mtx};

	map_lock.unlock();

	data->name = name;
	data->size = size;
	data->num_pages = memmanager_minimum_pages(size);
	data->physical = physical_memory_allocate(data->num_pages * PAGE_SIZE, PAGE_SIZE);
	data->num_refs = 1;

	return !!(data->physical) ? (uintptr_t)data : 0;
}

SYSCALL_HANDLER uintptr_t open_shared_buffer(const char* name, size_t name_len)
{
	sync::shared_lock map_lock{map_mtx};

	if(auto data = shared_buffers.lookup(std::string_view{name, name_len}))
	{
		sync::lock_guard l{data->mtx};

		data->num_refs++;

		return (uintptr_t)data;
	}

	return 0; //buffer does not exist
}

SYSCALL_HANDLER void close_shared_buffer(uintptr_t buf_handle)
{
	auto data = (shared_buffer*)buf_handle;

	sync::unique_lock map_lock{map_mtx};

	sync::lock_guard l{data->mtx};

	if(--data->num_refs == 0)
	{
		physical_memory_free(data->physical, data->num_pages * PAGE_SIZE);

		shared_buffers.remove(data->name);
	}
}

SYSCALL_HANDLER void* map_shared_buffer(uintptr_t buf_handle, size_t size, page_flags_t flags)
{
	auto data = (shared_buffer*)buf_handle;

	sync::lock_guard l{data->mtx};

	if(data->size < size)
		return nullptr;

	return memmanager_map_to_new_pages(data->physical,
									   memmanager_minimum_pages(size), 
									   PAGE_USER | PAGE_PRESENT | flags);
}