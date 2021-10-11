#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/syscalls.h>

void print_string(const char* str, size_t length);
void video_clear();
void video_erase_chars(size_t num);
int initialize_text_mode(int col, int row);
int get_terminal_width();
int get_terminal_height();

#ifdef __cplusplus
}
#endif
#endif