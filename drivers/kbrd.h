#ifndef KBRD_H
#define KBRD_H

//#include "../kernel/interrupt.h"
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

void handle_keyevent(char val);

#endif