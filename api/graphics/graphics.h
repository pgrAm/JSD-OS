#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <sys/syscalls.h>

#ifdef __cplusplus
extern "C" {
#endif

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