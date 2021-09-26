
extern "C" {
#include <kernel/filesystem.h>
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>

#include <kernel/task.h>
#include <kernel/elf.h>
#include <kernel/locks.h>
#include <drivers/video.h>
#include <drivers/sysclock.h>
#include <drivers/kbrd.h>

#include <stdlib.h>
#include <stdio.h>
}

#include <vector>
#include <string>

struct func_info {
	std::string name;
	void* address;
};

static void load_driver(std::string filename, std::string func_name,
				 const std::vector<func_info> &functions)
{
	dynamic_object ob;
	memset(&ob, 0, sizeof(dynamic_object));
	ob.lib_set = hashmap_create(16);
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	for(size_t i = 0; i < functions.size(); i++)
	{
		hashmap_insert(ob.symbol_map,
					   functions[i].name.c_str(), (uint32_t)functions[i].address);
	}

	load_elf(filename.c_str(), &ob, false);

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, func_name.c_str(), &func_address))
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
	{"malloc",	(void*)&malloc},
	{"free",	(void*)&free},
	{"strtok",	(void*)&strtok},
	{"strlen",	(void*)&strlen},
	{"mktime",	(void*)&mktime},
	{"filesystem_add_drive",		(void*)&filesystem_add_drive},
	{"filesystem_add_driver",		(void*)&filesystem_add_driver},
	{"filesystem_allocate_buffer",	(void*)&filesystem_allocate_buffer},
	{"filesystem_free_buffer",		(void*)&filesystem_free_buffer},
	{"filesystem_read_blocks_from_disk", (void*)&filesystem_read_blocks_from_disk},
	{"filesystem_create_stream", (void*)&filesystem_create_stream},
	{"__regcall3__filesystem_read_file", (void*)&filesystem_read_file},
	{"__regcall3__filesystem_close_file", (void*)&filesystem_close_file},
	{"irq_install_handler", (void*)&irq_install_handler},
	{"sysclock_sleep", (void*)&sysclock_sleep},
	{"memmanager_allocate_physical_in_range", (void*)&memmanager_allocate_physical_in_range},
	{"memmanager_map_to_new_pages", (void*)&memmanager_map_to_new_pages},
	{"memmanager_get_physical", (void*)&memmanager_get_physical},
	{"__regcall3__memmanager_free_pages", (void*)&memmanager_free_pages},
	{"kernel_lock_mutex", (void*)&kernel_lock_mutex},
	{"kernel_unlock_mutex", (void*)&kernel_unlock_mutex},
	{"handle_keyevent", (void*)&handle_keyevent},
	{"kernel_signal_cv", (void*)&kernel_signal_cv},
	{"kernel_wait_cv", (void*)&kernel_wait_cv}
};

extern "C" void load_drivers()
{
	std::vector<func_info> funcs(func_list, sizeof(func_list) / sizeof(func_info));

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

			printf("%s\n", buffer.c_str());

			if(token == "load_driver")
			{
				auto filename = buffer.substr(space + 1);

				auto dot = filename.find_first_of('.');
				auto init_func = filename.substr(0, dot) + "_init";

				printf("loading driver %s, calling %s\n", filename.c_str(), init_func.c_str());

				load_driver(filename, init_func, funcs);
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