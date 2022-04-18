#include <kernel/locks.h>
#include <kernel/task.h>

#include "input.h"

static bool virtual_keystates[NUM_VIRTUAL_KEYS];

#define INPUT_BUFFER_SIZE 64

static volatile size_t input_buf_front = 0;
static size_t input_buf_back = 0;
static input_event input_buf[INPUT_BUFFER_SIZE];

static kernel_cv input_waiting_cv = sync::init_cv();

void handle_input_event(input_event e)
{
	if(e.type == event_type::KEY_DOWN || e.type == event_type::KEY_UP)
	{
		bool down = (e.type == event_type::KEY_DOWN);

		virtual_keystates[e.data] = down;

		if(down && (e.data == VK_TAB) && (virtual_keystates[VK_RALT] || virtual_keystates[VK_LALT]))
		{
			//we hereby interrupt this process
			run_next_task();
			return;
		}
	}

	input_buf[input_buf_front] = e;
	input_buf_front = (input_buf_front + 1) % INPUT_BUFFER_SIZE;

	kernel_signal_cv(&input_waiting_cv);
}

static int do_get_input_event(input_event* e)
{
	if(this_task_is_active())
	{
		if(input_buf_back != input_buf_front)
		{
			*e = input_buf[input_buf_back];
			input_buf_back = (input_buf_back + 1) % INPUT_BUFFER_SIZE;

			return 0;
		}

		return 1;
	}

	return -1;
}

SYSCALL_HANDLER int get_input_event(input_event* e, bool wait)
{
	if(wait)
	{
		while(do_get_input_event(e) != 0)
		{
			kernel_wait_cv(&input_waiting_cv);
		}
		return 0;
	}
	else
	{
		return do_get_input_event(e);
	}
};

SYSCALL_HANDLER int get_keystate(key_type key)
{
	if(key >= NUM_VIRTUAL_KEYS || !this_task_is_active())
	{
		return 0;
	}
	return virtual_keystates[key];
}
