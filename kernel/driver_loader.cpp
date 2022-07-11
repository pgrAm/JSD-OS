#include <stdlib.h>
#include <stdio.h>

#include <kernel/filesystem/fs_driver.h>
#include <kernel/util/hash.h>
#include <kernel/dynamic_object.h>
#include <kernel/driver.h>

#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/locks.h>
#include <kernel/display.h>
#include <kernel/sysclock.h>
#include <kernel/rt_device.h>
#include <kernel/kassert.h>
#include <kernel/input.h>

#include <vector>
#include <string>
#include <array>

using namespace std::literals;

struct func_info {
	const std::string_view name;
	const void* address;
};

static dynamic_object::sym_map driver_lib_set{32};
static dynamic_object::sym_map driver_symbol_map{32};
static dynamic_object::sym_map driver_glob_data_symbol_map{};

extern "C" SYSCALL_HANDLER int load_driver(directory_stream* ,
											const file_handle* file)
{
	if(!this_task_is_active())
	{
		return -1;
	}

	assert(file);

	std::string_view filename{file->name};

	auto slash = filename.find_last_of('/');
	if(slash == std::string_view::npos)
		slash = 0;
	else
		slash++;

	auto dot			  = filename.find_first_of('.');
	std::string init_func = std::string{filename.substr(slash, dot - slash)};
	init_func += "_init"sv;

	//TODO: really gotta find a better way of doing this
	for(auto& c : init_func)
		c = (char)tolower(c);

	fs::dir_stream lib_dir{nullptr, *file->dir_path, 0};
	assert(lib_dir);

	dynamic_object ob{
		&driver_lib_set,
		&driver_symbol_map,
		&driver_glob_data_symbol_map,
	};

	if(load_elf(file, &ob, false, lib_dir.get_ptr()))
	{
		uint32_t func_address;
		if(ob.symbol_map->lookup(init_func, &func_address))
		{
			((driver_init_func)func_address)(lib_dir.get_ptr());
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}

	return 0;
}

static constexpr std::array func_list
{
	func_info{"_Znwj"sv, (void*)static_cast<void* (*)(size_t)>(::operator new)},
	func_info{"_Znaj"sv, (void*)static_cast<void* (*)(size_t)>(::operator new[])},
	func_info{"_ZdlPv"sv,
			  (void*)static_cast<void (*)(void*)>(::operator delete)},
	func_info{"_ZdaPv"sv,
			  (void*)static_cast<void (*)(void*)>(::operator delete[])},
	func_info{"_ZnwjSt11align_val_t"sv,
			  (void*)static_cast<void* (*)(size_t, std::align_val_t)>(
				  ::operator new)},
	func_info{"_ZnajSt11align_val_t"sv,
			  (void*)static_cast<void* (*)(size_t, std::align_val_t)>(
				  ::operator new[])},
	func_info{"printf"sv,						(void*)&printf},
	func_info{"puts"sv,							(void*)&puts},
	func_info{"time"sv,							(void*)&time},
	func_info{"aligned_alloc"sv,				(void*)&aligned_alloc},
	func_info{"malloc"sv,						(void*)&malloc},
	func_info{"calloc"sv,						(void*)&calloc},
	func_info{"free"sv,							(void*)&free},
	func_info{"mktime"sv,						(void*)&mktime},
	func_info{"gmtime"sv,						(void*)&gmtime},
	func_info{"memcmp"sv,						(void*)&memcmp},
	func_info{"memset"sv,						(void*)&memset},
	func_info{"memmove"sv,						(void*)&memmove},
	func_info{"memcpy"sv,						(void*)&memcpy},
	func_info{"filesystem_add_virtual_drive"sv,	(void*)&filesystem_add_virtual_drive},
	func_info{"filesystem_add_partitioner"sv,	(void*)&filesystem_add_partitioner},
	func_info{"filesystem_add_drive"sv,			(void*)&filesystem_add_drive},
	func_info{"filesystem_add_driver"sv,		(void*)&filesystem_add_driver},
	func_info{"filesystem_read_from_disk"sv,	(void*)&filesystem_read_from_disk},
	func_info{"filesystem_write_to_disk"sv,		(void*)&filesystem_write_to_disk},
	func_info{"filesystem_create_stream"sv,		(void*)&filesystem_create_stream},
	func_info{"filesystem_open_file"sv,			(void*)&filesystem_open_file},
	func_info{"filesystem_close_file"sv,		(void*)&filesystem_close_file},
	func_info{"filesystem_read_file"sv,			(void*)&filesystem_read_file},
	func_info{"filesystem_write_file"sv,		(void*)&filesystem_write_file},
	func_info{"filesystem_open_directory"sv,	(void*)&filesystem_open_directory},
	func_info{"filesystem_close_directory"sv,	(void*)&filesystem_close_directory},
	func_info{"irq_install_handler"sv,			(void*)&irq_install_handler},
	func_info{"sysclock_sleep"sv,				(void*)&sysclock_sleep},
	func_info{"sysclock_get_ticks"sv,			(void*)&sysclock_get_ticks},
	func_info{"physical_memory_allocate_in_range"sv, (void*)&physical_memory_allocate_in_range},
	func_info{"physical_memory_allocate"sv,		(void*)&physical_memory_allocate},
	func_info{"memmanager_virtual_alloc"sv,		(void*)&memmanager_virtual_alloc},
	func_info{"memmanager_map_to_new_pages"sv,	(void*)&memmanager_map_to_new_pages},
	func_info{"memmanager_get_physical"sv,		(void*)&memmanager_get_physical},
	func_info{"memmanager_unmap_pages"sv,		(void*)&memmanager_unmap_pages},
	func_info{"memmanager_free_pages"sv,		(void*)&memmanager_free_pages},
	func_info{"kernel_lock_mutex"sv,			(void*)&kernel_lock_mutex},
	func_info{"kernel_unlock_mutex"sv,			(void*)&kernel_unlock_mutex},
	func_info{"kernel_signal_cv"sv,				(void*)&kernel_signal_cv},
	func_info{"kernel_wait_cv"sv,				(void*)&kernel_wait_cv},
	func_info{"display_add_driver"sv,			(void*)&display_add_driver},
	func_info{"acknowledge_irq"sv,				(void*)&acknowledge_irq},
	func_info{"irq_enable"sv,					(void*)&irq_enable},
	func_info{"add_realtime_device"sv,			(void*)&add_realtime_device},
	func_info{"find_realtime_device"sv,			(void*)&find_realtime_device},
	func_info{"handle_input_event"sv,			(void*)&handle_input_event},
	func_info{"switch_to_active_task"sv,		(void*)&switch_to_active_task},
	func_info{"run_background_tasks"sv,			(void*)&run_background_tasks},
	func_info{"add_cpu"sv,						(void*)&add_cpu},
	func_info{"cpu_entry_point"sv,				(void*)&cpu_entry_point},

#ifndef NDEBUG
	func_info{"__assert_fail"sv,				(void*)&__assert_fail},
#endif
	func_info{"clock"sv, (void*)&clock},
};

extern "C" void load_drivers()
{
	for(auto&& func : func_list)
	{
		driver_symbol_map.insert(func.name, (uintptr_t)func.address);
	}

	std::string_view init_path = "init.elf";

	auto root = filesystem_get_root_directory(0);

	fs::dir_stream cwd{&*root, 0};

	if(!cwd)
	{
		print_string("Corrupted boot drive!\n");
		while(true);
	}

	auto f = cwd.find_file_by_path(init_path);

	if(!f)
	{
		print_string("Corrupted boot drive!\n");
		while(true);
	}

	spawn_process(&(*f), nullptr, 0, WAIT_FOR_PROCESS);
}