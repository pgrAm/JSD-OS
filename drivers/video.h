#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

#include <string.h>

void set_color(uint8_t bgr, uint8_t fgr, uint8_t bright);

void initialize_video();

uint16_t get_cursor_offset();

void delete_chars(size_t num);

void print_char(const char c);

void set_cursor(uint16_t row, uint16_t col);

void print_string(const char* str);
void print_string_len(const char* str, size_t length);

void clear_screen();

void clear_row(uint16_t row);

#endif