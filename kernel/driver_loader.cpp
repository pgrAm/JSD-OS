#include <stdlib.h>
#include <stdio.h>

#include <kernel/fs_driver.h>
#include <kernel/util/hash.h>
#include <kernel/dynamic_object.h>

extern "C" {
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/physical_manager.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/locks.h>
#include <kernel/display.h>
#include <drivers/sysclock.h>
#include <drivers/kbrd.h>
}

#include <vector>
#include <string>

struct func_info {
	std::string name;
	void* address;
};

dynamic_object::sym_map driver_lib_set{};
dynamic_object::sym_map driver_symbol_map{};
dynamic_object::sym_map driver_glob_data_symbol_map{};

static void load_driver(std::string filename, std::string func_name)
{
	dynamic_object ob{};
	ob.lib_set = &driver_symbol_map;
	ob.symbol_map = &driver_symbol_map;
	ob.glob_data_symbol_map = &driver_glob_data_symbol_map;

	load_elf(filename.c_str(), &ob, false);

	uint32_t func_address;
	if(ob.symbol_map->lookup(func_name.c_str(), &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}
}

static const func_info func_list[] = {
	{"printf",	(void*)&printf},
	{"vsnprintf",(void*)&vsnprintf},
	{"strcat",	(void*)&strcat},
	{"time",	(void*)&time},
	{"malloc",	(void*)&malloc},
	{"calloc",	(void*)&calloc},
	{"free",	(void*)&free},
	{"strtok",	(void*)&strtok},
	{"strlen",	(void*)&strlen},
	{"mktime",	(void*)&mktime},
	{"memcmp",	(void*)&memcmp},
	{"memset",	(void*)&memset},
	{"strcmp",	(void*)&strcmp},
	{"filesystem_add_virtual_drive",	(void*)&filesystem_add_virtual_drive},
	{"filesystem_add_partitioner",		(void*)&filesystem_add_partitioner},
	{"filesystem_add_drive",		(void*)&filesystem_add_drive},
	{"filesystem_add_driver",		(void*)&filesystem_add_driver},
	{"filesystem_allocate_buffer",	(void*)&filesystem_allocate_buffer},
	{"filesystem_free_buffer",		(void*)&filesystem_free_buffer},
	{"filesystem_read_blocks_from_disk", (void*)&filesystem_read_blocks_from_disk},
	{"filesystem_create_stream", (void*)&filesystem_create_stream},
	{"__regcall3__filesystem_open_file", (void*)&filesystem_open_file},
	{"__regcall3__filesystem_read_file", (void*)&filesystem_read_file},
	{"__regcall3__filesystem_close_file", (void*)&filesystem_close_file},
	{"irq_install_handler", (void*)&irq_install_handler},
	{"sysclock_sleep", (void*)&sysclock_sleep},
	{"physical_memory_allocate_in_range", (void*)&physical_memory_allocate_in_range},
	{"memmanager_map_to_new_pages", (void*)&memmanager_map_to_new_pages},
	{"memmanager_get_physical", (void*)&memmanager_get_physical},
	{"__regcall3__memmanager_free_pages", (void*)&memmanager_free_pages},
	{"kernel_lock_mutex", (void*)&kernel_lock_mutex},
	{"kernel_unlock_mutex", (void*)&kernel_unlock_mutex},
	{"handle_keyevent", (void*)&handle_keyevent},
	{"kernel_signal_cv", (void*)&kernel_signal_cv},
	{"kernel_wait_cv", (void*)&kernel_wait_cv},
	{"display_add_driver", (void*)&display_add_driver},
	{"display_mode_satisfied", (void*)&display_mode_satisfied}
};

extern "C" void load_drivers()
{
	auto num_funcs = sizeof(func_list) / sizeof(func_info);
	for(size_t i = 0; i < num_funcs; i++)
	{
		driver_symbol_map.insert(func_list[i].name,
							  (uint32_t)func_list[i].address);
	}

	file_stream* f = filesystem_open_file(nullptr, "init.sys", 0);

	if(!f)
	{
		printf("Cannot find init.sys!\n");
		while(true);
	}

	std::string buffer;
	bool eof = false;
	while(!eof)
	{
		char c;
		if(filesystem_read_file(&c, sizeof(char), f) == -1)
		{
			eof = true;
			c = '\n';
		}

		if(c == '\n')
		{
			auto space = buffer.find_first_of(' ');
			auto token = buffer.substr(0, space);

			if(token == "load_driver")
			{
				auto filename = buffer.substr(space + 1);
				auto slash = filename.find_last_of('/');

				if(slash == std::string::npos)
					slash = 0;
				else
					slash++;

				auto dot = filename.find_first_of('.');
				auto init_func = filename.substr(slash, dot - slash) + "_init";

				printf("loading driver %s, calling %s\n", filename.c_str(), init_func.c_str());

				load_driver(filename, init_func);
			}
			else if(token == "set_drive")
			{
				auto filename = buffer.substr(space + 1);

				filesystem_set_default_drive(atoi(filename.c_str()));
			}

			buffer.clear();
		}
		else if(c != '\r')
		{
			buffer += c;
		}
	}
}

extern "C" {
#include <stddef.h>
#include <stdlib.h>
}

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