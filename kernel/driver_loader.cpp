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

extern "C" {
#include <drivers/kbrd.h>
}

#include <vector>
#include <string>

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

	dynamic_object ob{};
	ob.lib_set = &driver_symbol_map;
	ob.symbol_map = &driver_symbol_map;
	ob.glob_data_symbol_map = &driver_glob_data_symbol_map;

	auto f = cwd.find_file_by_path(filename);
	if(f == nullptr)
	{
		printf("could not find driver %s\n", std::string(filename).c_str());
		return;
	}

	std::string_view lib_path{};
	if(auto slash = filename.find_last_of('/'); slash != std::string_view::npos)
	{
		lib_path = filename.substr(0, slash);
	}

	fs::dir_stream lib_dir{cwd, lib_path, 0};

	if(load_elf(f, &ob, false, lib_dir ? lib_dir.get_ptr() : cwd.get_ptr()))
	{
		uint32_t func_address;
		if(ob.symbol_map->lookup(func_name, &func_address))
		{
			((driver_init_func)func_address)(cwd.get_ptr());
		}
		else
		{
			printf("cannot find function\n");
		}
	}
	else
	{
		printf("Cannot find ");
		print_string_len(filename.data(), filename.size());
		putchar('\n');
	}
}

typedef SYSCALL_HANDLER clock_t (*clock_func)(size_t*);

static constexpr func_info func_list[] = {
	{"printf",	(void*)&printf},
	{"time",	(void*)&time},
	{"malloc",	(void*)&malloc},
	{"calloc",	(void*)&calloc},
	{"free",	(void*)&free},
	{"strlen",	(void*)&strlen},
	{"mktime",	(void*)&mktime},
	{"memcmp",	(void*)&memcmp},
	{"memset",	(void*)&memset},
	{"filesystem_add_virtual_drive",(void*)&filesystem_add_virtual_drive},
	{"filesystem_add_partitioner",	(void*)&filesystem_add_partitioner},
	{"filesystem_add_drive",		(void*)&filesystem_add_drive},
	{"filesystem_add_driver",		(void*)&filesystem_add_driver},
	{"filesystem_allocate_buffer",	(void*)&filesystem_allocate_buffer},
	{"filesystem_free_buffer",		(void*)&filesystem_free_buffer},
	{"filesystem_read_from_disk",	(void*)&filesystem_read_from_disk},
	{"filesystem_write_to_disk",	(void*)&filesystem_write_to_disk},
	{"filesystem_create_stream",	(void*)&filesystem_create_stream},
	{"filesystem_open_file",	(void*)&filesystem_open_file},
	{"filesystem_close_file",	(void*)&filesystem_close_file},
	{"filesystem_read_file",	(void*)&filesystem_read_file},
	{"irq_install_handler",			(void*)&irq_install_handler},
	{"sysclock_sleep",				(void*)&sysclock_sleep},
	{"__regcall3__sysclock_get_ticks", (void*)(clock_func)sysclock_get_ticks},
	{"memmanager_virtual_alloc", (void*)&memmanager_virtual_alloc},
	{"physical_memory_allocate_in_range", (void*)&physical_memory_allocate_in_range},
	{"physical_memory_allocate",	(void*)&physical_memory_allocate},
	{"memmanager_map_to_new_pages", (void*)&memmanager_map_to_new_pages},
	{"memmanager_get_physical",		(void*)&memmanager_get_physical},
	{"memmanager_unmap_pages",		(void*)&memmanager_unmap_pages},
	{"memmanager_free_pages", (void*)&memmanager_free_pages},
	{"kernel_lock_mutex",	(void*)&kernel_lock_mutex},
	{"kernel_unlock_mutex", (void*)&kernel_unlock_mutex},
	{"handle_keyevent",		(void*)&handle_keyevent},
	{"kernel_signal_cv",	(void*)&kernel_signal_cv},
	{"kernel_wait_cv",		(void*)&kernel_wait_cv},
	{"display_add_driver",	(void*)&display_add_driver},
	{"acknowledge_irq",		(void*)&acknowledge_irq},
	{"irq_enable",			(void*)&irq_enable},
	{"add_realtime_device",	(void*)&add_realtime_device},
	{"find_realtime_device",(void*)&find_realtime_device},
	{"handle_input_event",	(void*)&handle_input_event}
};

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

		if(c == '\n')
		{
			auto line = std::string_view(buffer);

			auto space = line.find_first_of(' ');
			auto token = line.substr(0, space);

			if(line.size() >= 2 && line[0] == '/' && line[1] == '/')
			{
			}
			else if(token == "load_driver")
			{
				auto filename = line.substr(space + 1);
				auto slash = filename.find_last_of('/');

				if(slash == std::string_view::npos)
					slash = 0;
				else
					slash++;

				auto dot = filename.find_first_of('.');
				std::string init_func = std::string{filename.substr(slash, dot - slash)};
				init_func += "_init";

				printf("loading driver ");
				print_string_len(filename.data(), filename.size());
				printf(", calling ");
				print_string_len(init_func.data(), init_func.size());
				putchar('\n');

				load_driver(cwd, filename, init_func);
			}
			else if(token == "load_file")
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

					auto file = dir.find_file_by_path(filename);

					if(file)
					{
						printf("processing file ");
						print_string_len(filename.data(), filename.size());
						printf(", set init drive\n");

						if(fs::stream stream{file, 0}; !!stream)
						{
							process_init_file(dir, stream);
						}
						break;
					}
				}
			}
			else if(token == "execute")
			{

				auto filename = line.substr(space + 1);
				auto f = cwd.find_file_by_path(filename);

				if(!f)
				{
					printf("can't find ");
					print_string_len(filename.data(), filename.size());
					printf("\n");
				}
				else
				{
					printf("executing ");
					print_string_len(filename.data(), filename.size());
					printf("\n");

					spawn_process(f, cwd.get_ptr(), WAIT_FOR_PROCESS);
				}
			}

			buffer.clear();
		}
		else if(c != '\r')
		{
			buffer += c;
		}
	}
}

extern "C" void load_drivers()
{
	printf("loading drivers\n");

	const auto num_funcs = sizeof(func_list) / sizeof(func_info);
	for(size_t i = 0; i < num_funcs; i++)
	{
		driver_symbol_map.insert(func_list[i].name, (uintptr_t)func_list[i].address);
	}

	std::string_view init_path = "init.sys";

	fs::dir_stream cwd{filesystem_get_root_directory(0), 0};

	if(!cwd)
	{
		printf("Corrupted boot drive!\n");
		while(true);
	}

	fs::stream f{cwd.get_ptr(), init_path, 0};

	if(!f)
	{
		printf("Cannot find init.sys!\n");
		while(true);
	}

	process_init_file(cwd, f);
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