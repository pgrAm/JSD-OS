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

static void load_driver(fs::dir_stream_ref cwd, const std::string_view filename, const std::string_view func_name)
{
	k_assert(cwd);

	dynamic_object ob {	
		&driver_lib_set, 
		&driver_symbol_map, 
		&driver_glob_data_symbol_map
	};

	auto f = cwd.find_file_by_path(filename);
	if(!f)
	{
		print_strings("Can't find driver ", filename, '\n');
		return;
	}

	std::string_view lib_path{};
	if(auto slash = filename.find_last_of('/'); slash != std::string_view::npos)
	{
		lib_path = filename.substr(0, slash);
	}

	fs::dir_stream lib_dir{cwd, lib_path, 0};

	if(load_elf(&(*f), &ob, false, lib_dir ? lib_dir.get_ptr() : cwd.get_ptr()))
	{
		uint32_t func_address;
		if(ob.symbol_map->lookup(func_name, &func_address))
		{
			((driver_init_func)func_address)(cwd.get_ptr());
		}
		else
		{
			print_string("Can't find function\n");
		}
	}
	else
	{
		print_strings("Can't find ", filename, '\n');
	}
}

static constexpr std::array func_list
{
	func_info{"printf"sv,						(void*)&printf},
	func_info{"puts"sv,							(void*)&puts},
	func_info{"time"sv,							(void*)&time},
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
	func_info{"filesystem_seek_file"sv,			(void*)&filesystem_seek_file},
	func_info{"filesystem_get_pos"sv,			(void*)&filesystem_get_pos},
	func_info{"filesystem_read_file"sv,			(void*)&filesystem_read_file},
	func_info{"filesystem_write_file"sv,		(void*)&filesystem_write_file},
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
#ifndef NDEBUG
	func_info{"__kassert_fail"sv,				(void*)&__kassert_fail},
#endif
};

static void process_init_file(fs::dir_stream_ref cwd, fs::stream_ref f);

static void execute_line(std::string_view line, fs::dir_stream_ref cwd)
{
	auto space = line.find_first_of(' ');
	auto token = line.substr(0, space);

	if(line.size() >= 2 && line[0] == '/' && line[1] == '/')
	{
	}
	else if(token == "load_driver"sv)
	{
		auto filename = line.substr(space + 1);
		auto slash = filename.find_last_of('/');

		if(slash == std::string_view::npos)
			slash = 0;
		else
			slash++;

		auto dot = filename.find_first_of('.');
		std::string init_func = std::string{filename.substr(slash, dot - slash)};
		init_func += "_init"sv;

		print_strings("Loading driver "sv, filename, ", calling ", init_func, '\n');

		load_driver(cwd, filename, init_func);
	}
	else if(token == "load_file"sv)
	{
		auto filename = line.substr(space + 1);

		size_t num_drives = filesystem_get_num_drives();

		for(size_t drive = 0; drive < num_drives; drive++)
		{
			auto root = filesystem_get_root_directory(drive);

			if(!root)
				continue;

			fs::dir_stream dir{root, 0};

			if(!dir)
				continue;

			if(fs::stream stream{dir.get_ptr(), filename}; !!stream)
			{
				print_strings("Processing file ", filename, ", set init drive\n");

				process_init_file(dir, stream);
				return;
			}
		}
		print_strings("Could not find ", filename, '\n');
	}
	else if(token == "execute"sv)
	{
		auto filename = line.substr(space + 1);
		auto f = cwd.find_file_by_path(filename);

		if(!f)
		{
			print_strings("Can't find ", filename, '\n');
		}
		else
		{
			print_strings("Executing ", filename, '\n');

			spawn_process(&(*f), cwd.get_ptr(), WAIT_FOR_PROCESS);
		}
	}
}

static void process_init_file(fs::dir_stream_ref cwd, fs::stream_ref f)
{
	k_assert(cwd);

	std::string buffer;
	bool eof = false;
	while(!eof)
	{
		char c;
		if(f.read(&c, sizeof(char)) == -1)
		{
			eof = true;
			c = '\n';
		}
		switch(c)
		{
		case '\r':
			continue;
		case '\n':
			execute_line(buffer, cwd);
			buffer.clear();
			continue;
		default:
			buffer += c;
		}
	}
}

extern "C" void load_drivers()
{
	print_string("Loading drivers\n"sv);

	for(auto&& func : func_list)
	{
		driver_symbol_map.insert(func.name, (uintptr_t)func.address);
	}

	std::string_view init_path = "init.sys";

	fs::dir_stream cwd{filesystem_get_root_directory(0), 0};

	if(!cwd)
	{
		print_string("Corrupted boot drive!\n");
		while(true);
	}

	if(fs::stream f{cwd.get_ptr(), init_path, 0}; !!f)
	{
		process_init_file(cwd, f);
		return;
	}

	print_string("Can't find init.sys!\n");
	while(true);
}

#include <stddef.h>
#include <stdlib.h>

void* operator new (size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* p)
{
	free(p);
}

void operator delete[](void* p)
{
	free(p);
}