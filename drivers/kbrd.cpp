#include <kernel/task.h>
#include <kernel/syscall.h>
#include <kernel/locks.h>
#include <kernel/interrupt.h>
#include <kernel/input.h>
#include <kernel/sysclock.h>

#include <bit>

#include "kbrd.h"

static bool virtual_keystates[NUM_VIRTUAL_KEYS];

SYSCALL_HANDLER int get_keystate(key_type key)
{
	if (key >= NUM_VIRTUAL_KEYS || !this_task_is_active())
	{
		return 0;
	}
	return virtual_keystates[key];
}

extern "C" void handle_keyevent(uint32_t scancode, bool down)
{
	virtual_keystates[scancode] = down;

	if (down && scancode == VK_TAB && (virtual_keystates[VK_RALT] || virtual_keystates[VK_LALT]))
	{
		//we hereby interrupt this process
		run_next_task();
		return;
	}

	if(scancode != VK_NONE)
	{
		handle_input_event(input_event{0, 0, 
						   std::bit_cast<int>(scancode), 
						   down ? event_type::KEY_DOWN : event_type::KEY_UP,
						   sysclock_get_ticks()});
	}
}