#ifndef INPUT_H
#define INPUT_H

#define DISPLAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <api/virtual_keys.h>
#include <common/input_event.h>
#include <kernel/syscall.h>

void handle_input_event(input_event e);
SYSCALL_HANDLER int get_input_event(input_event* e, bool wait);

SYSCALL_HANDLER int get_keystate(key_type key);


#ifdef __cplusplus
}
#endif

#endif