#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"

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

void keyboard_handler(struct interrupt_info *r)
{
    uint8_t scancode = inb(0x60);
	
    if(scancode & 0x80)
    {
		if(scancode == 0xB6 || scancode == 0xAA)
		{
			keystate ^= SHIFT_PRESSED;
		}
    }
    else
    {
		//printf(" %X ", scancode);
		
		if(scancode == 0x3A) //caps lock
		{
			keystate ^= CAPS_LOCKED;
		}
		else if(scancode == 0x36 || scancode == 0x2A)
		{
			keystate |= SHIFT_PRESSED;
		}
		else
		{
			char letter = keystate & (CAPS_LOCKED | SHIFT_PRESSED) ? shftmap[scancode] : keymap[scancode];
			
			if(letter != '\0' )
			{
				keybuf_push(letter);
			}
		}
    }
}

void keyboard_init()
{
	keyboard_buffer_front = keyboard_buffer_back = keyboard_buffer_count = 0;

	memset(keyboard_buffer, 0, sizeof(char) * KEY_BUFFER_SIZE);
    irq_install_handler(1, keyboard_handler);
}