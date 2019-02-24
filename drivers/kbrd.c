#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"
#include "../kernel/task.h"
#include "kbrd.h"
#include "at_kbrd.h"

const char keymap[256]  = "\0\x1b""1234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;\'`\0\\zxcvbnm,./\0*\0 \0""1234567890\0\0""789-456+1230.";
const char shftmap[256] = "\0\x1b""!@#$%^&*()_+\b\tQWERTYUIOP{}\n\0ASDFGHJKL:\"~\0\\ZXCVBNM<>?\0*\0 \0""1234567890\0\0""789-456+1230.";

#define KEY_BUFFER_SIZE 641

volatile char keyboard_buffer[KEY_BUFFER_SIZE];
volatile size_t keyboard_buffer_front;
volatile size_t keyboard_buffer_back;
volatile size_t keyboard_buffer_count; 

uint8_t keystate = 0;

#define SHIFT_PRESSED 0x01
#define CAPS_LOCKED 0x02
#define ALT_PRESSED 0x04

char keys_states[256];

void keybuf_push(char val)
{
	keyboard_buffer[keyboard_buffer_back] = val;
	keyboard_buffer_back = (keyboard_buffer_back + 1) % KEY_BUFFER_SIZE;
	
	if(keyboard_buffer_count >= KEY_BUFFER_SIZE) //if we overrun the buffer
	{
		keyboard_buffer_front++; //discard the oldest data
	}
	else
	{
		keyboard_buffer_count++;
	}
}

char keybuf_pop()
{
	while(keyboard_buffer_count == 0); //wait till we get another character
	
	char c = keyboard_buffer[keyboard_buffer_front];
	keyboard_buffer_front = (keyboard_buffer_front + 1) % KEY_BUFFER_SIZE;
	
	keyboard_buffer_count--;
	return c;
}

void handle_keyevent(char scancode)
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
					
					fputc(letter, stdin);
					
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
	keyboard_buffer_front = keyboard_buffer_back = keyboard_buffer_count = 0;

	memset(keyboard_buffer, 0, sizeof(char) * KEY_BUFFER_SIZE);
    
	AT_keyboard_init();
}