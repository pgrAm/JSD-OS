#include <sys/syscalls.h>
#include <stdlib.h>
#include <bit>
#include <common/util.h>
#include <common/task_data.h>
#include <thread.h>

struct tls_thread_block
{
	tls_thread_block* self;
	task_id tid;
	void (*start_func)();
};

tls_thread_block* get_thread_ptr()
{
	tls_thread_block* self;
	asm("mov %%gs:0, %0" : "=r"(self));
	return self;
}

struct tls_data
{
	size_t alloc_align;
	size_t alloc_size;
	size_t thread_block_offset;
	size_t local_image_offset;
	void* master_image_ptr;
	size_t image_size;
};

tls_data gen_process_tls_data(tls_image_data img)
{
	auto align = std::max(img.alignment, alignof(tls_thread_block));

	return tls_data{
		.alloc_align = align,
		.alloc_size =
			align_addr(img.total_size, align) + sizeof(tls_thread_block),
		.thread_block_offset = align_addr(img.total_size, align),
		.local_image_offset	 = align_addr(img.total_size, img.alignment),
		.master_image_ptr	 = img.pointer,
		.image_size			 = img.image_size,
	};
}

tls_data tls;

void* create_tls_block(void (*func)())
{
	auto buffer = aligned_alloc(tls.alloc_align, tls.alloc_size);

	auto thread_block_location =
		std::bit_cast<uintptr_t>(buffer) + tls.thread_block_offset;

	auto tls_image_base = thread_block_location - tls.local_image_offset;

	tls_thread_block* thread_block =
		std::bit_cast<tls_thread_block*>(thread_block_location);

	thread_block->self		 = thread_block;
	thread_block->start_func = func;

	memcpy(std::bit_cast<void*>(tls_image_base), tls.master_image_ptr,
		   tls.image_size);
}

void init_first_thread()
{
	process_info p_info;
	get_process_info(&p_info);

	tls = gen_process_tls_data(p_info.tls);

	set_tls_addr(create_tls_block(nullptr));
}

task_id spawn_thread(void (*func)())
{
	return sys_spawn_thread(
		[](task_id this_thread)
		{
			get_thread_ptr()->tid = this_thread;
			get_thread_ptr()->start_func();

			auto buf_loc =
				(uintptr_t)get_thread_ptr() - tls.thread_block_offset;

			free(std::bit_cast<void*>(buf_loc));

			exit_thread(0);
		},
		create_tls_block(func));
}