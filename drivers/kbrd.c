#include <stdio.h>

#include <kernel/task.h>
#include <kernel/syscall.h>
#include <kernel/locks.h>
#include <kernel/interrupt.h>
#include <drivers/portio.h>

#include "kbrd.h"

#define KEY_BUFFER_SIZE 512
#define KEY_FAILURE 0xFF
volatile size_t keybuf_front = 0;
size_t keybuf_back = 0;
key_type keybuf[KEY_BUFFER_SIZE];

static kernel_cv key_pressed_cv = {0, -1};

static void add_keypress(key_type k)
{
	keybuf[keybuf_front] = k;
	keybuf_front = (keybuf_front + 1) % KEY_BUFFER_SIZE;
}

SYSCALL_HANDLER key_type get_keypress()
{
	key_type k = VK_NONE;
	
	int_lock l = lock_interrupts();
	
	if(this_task_is_active())
	{
		if(keybuf_back != keybuf_front)
		{
			k = keybuf[keybuf_back];
			keybuf_back = (keybuf_back + 1) % KEY_BUFFER_SIZE;
		}
	}
	
	unlock_interrupts(l);
	
	return k;
};

SYSCALL_HANDLER key_type wait_and_get_keypress()
{
	key_type k;
	
	while((k = get_keypress()) == VK_NONE)
	{
		kernel_wait_cv(&key_pressed_cv);
	}
	
	return k;
}

static bool virtual_keystates[NUM_VIRTUAL_KEYS];

SYSCALL_HANDLER int get_keystate(key_type key)
{
	if (key >= NUM_VIRTUAL_KEYS || !this_task_is_active())
	{
		return 0;
	}
	return virtual_keystates[key];
}

void handle_keyevent(uint32_t scancode, bool down)
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
		size_t active_process = get_active_process();
		
		add_keypress(scancode);
		
		kernel_signal_cv(&key_pressed_cv);
	}
}

void keyboard_init()
{
	memset(virtual_keystates, false, sizeof(bool) * NUM_VIRTUAL_KEYS);
}