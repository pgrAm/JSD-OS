#ifndef KBRD_H
#define KBRD_H
#include <stdbool.h>
#include <api/virtual_keys.h>

#include "../kernel/syscall.h"
void keybuf_push(char val);
char keybuf_pop();

//void keyboard_handler(struct interrupt_info *r);
void keyboard_init();
void handle_keyevent(uint32_t val, bool down);

SYSCALL_HANDLER key_type get_keypress(void);
SYSCALL_HANDLER key_type wait_and_get_keypress(void);
SYSCALL_HANDLER int get_keystate(key_type key);
#endif