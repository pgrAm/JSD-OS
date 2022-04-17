#ifndef KBRD_H
#define KBRD_H
#include <stdbool.h>
#include <api/virtual_keys.h>

#include <kernel/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

	//void keyboard_handler(struct interrupt_info *r);
	void handle_keyevent(uint32_t val, bool down);

	SYSCALL_HANDLER int get_keystate(key_type key);

#ifdef __cplusplus
}
#endif

#endif