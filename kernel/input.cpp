#include <kernel/locks.h>
#include <kernel/task.h>

#include "input.h"

#define INPUT_BUFFER_SIZE 64

static volatile size_t input_buf_front = 0;
static size_t input_buf_back = 0;
static input_event input_buf[INPUT_BUFFER_SIZE];

static kernel_cv input_waiting_cv = {-1, 1};

void handle_input_event(input_event e)
{
	input_buf[input_buf_front] = e;
	input_buf_front = (input_buf_front + 1) % INPUT_BUFFER_SIZE;

	kernel_signal_cv(&input_waiting_cv);
}

SYSCALL_HANDLER int get_input_event(input_event* e)
{
	if(this_task_is_active())
	{
		if(input_buf_back != input_buf_front)
		{
			*e = input_buf[input_buf_back];
			input_buf_back = (input_buf_back + 1) % INPUT_BUFFER_SIZE;

			return 0;
		}
	}

	return -1;
};