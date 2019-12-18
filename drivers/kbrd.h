#ifndef KBRD_H
#define KBRD_H

#include "../kernel/syscall.h"
void keybuf_push(char val);
char keybuf_pop();

//void keyboard_handler(struct interrupt_info *r);
void keyboard_init();

enum virtual_keycode
{
	VK_BACKSPACE,
	VK_TAB,
	VK_ENTER,
	VK_SHIFT,
	VK_CTRL,
	VK_ALT,
	VK_ESC,
	VK_LEFT,
	VK_UP,
	VK_RIGHT,
	VK_DOWN,
};

void handle_keyevent(uint8_t val);

typedef uint8_t key_type;

SYSCALL_HANDLER key_type get_keypress(void);
SYSCALL_HANDLER key_type wait_and_get_keypress(void);

#endif