#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"
#include "../kernel/task.h"
#include "../kernel/syscall.h"
#include "kbrd.h"
#include "at_kbrd.h"

const key_type keymap[256]  = "\0\x1b""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;\'`\0\\zxcvbnm,./\0*\0 \0""1234567890\0\0""789-456+1230.";
const key_type shftmap[256] = "\0\x1b""!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0""1234567890\0\0""789-456+1230.";

#define KEY_BUFFER_SIZE 512
#define KEY_FAILURE 0xFF
volatile size_t keybuf_front = 0;
size_t keybuf_back = 0;
key_type keybuf[KEY_BUFFER_SIZE];

void add_keypress(key_type k)
{
	keybuf[keybuf_front] = k;
	keybuf_front = (keybuf_front + 1) % KEY_BUFFER_SIZE;
}

SYSCALL_HANDLER key_type get_keypress()
{
	key_type k = KEY_FAILURE;
	
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
	
	while((k = get_keypress()) == KEY_FAILURE)
	{
		//yield();
	}
	
	return k;
}

uint8_t keystate = 0;

#define SHIFT_PRESSED 0x01
#define CAPS_LOCKED 0x02
#define ALT_PRESSED 0x04

char keys_states[256];

void handle_keyevent(uint8_t scancode)
{
    if(scancode & 0x80)
    {
		if(scancode == 0xB6 || scancode == 0xAA)
		{
			keystate &= ~SHIFT_PRESSED;
		}
		if(scancode == 0xB8)
		{
			keystate &= ~ALT_PRESSED;
		}
    }
    else
    {
		switch(scancode)
		{
			case 0x0f: //tab
				if(keystate & ALT_PRESSED)
				{
					//we hereby interrupt this process
					run_next_task();
				}
				break;
			case 0x38: //alt
				keystate |= ALT_PRESSED;
				break;
			case 0x3A: //caps lock
				keystate ^= CAPS_LOCKED;
				break;
			case 0x36:
			case 0x2A:
				keystate |= SHIFT_PRESSED;
				break;
			default:
			{
				char letter = keystate & (CAPS_LOCKED | SHIFT_PRESSED) ? shftmap[scancode] : keymap[scancode];
				
				if(letter != '\0' )
				{
					size_t active_process = get_active_process();
					
					add_keypress(letter);
					
					if(!task_is_running(active_process))
					{
						switch_to_task(active_process);
					}
				}
				break;
			}
		}
    }
}

void keyboard_init()
{
	AT_keyboard_init();
}