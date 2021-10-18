#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <virtual_keys.h>

#ifdef __cplusplus
extern "C" {
#endif
	char get_ascii_from_vk(key_type k);
#ifdef __cplusplus
}
#endif
#endif