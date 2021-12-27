#ifndef INPUT_H
#define INPUT_H

#define DISPLAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include <common/input_event.h>
#include <kernel/syscall.h>

void handle_input_event(input_event e);
SYSCALL_HANDLER int get_input_event(input_event* e);

#ifdef __cplusplus
}
#endif

#endif