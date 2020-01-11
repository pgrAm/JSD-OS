#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

extern void* memcpy(void* dest, const void* src, size_t num);
extern void* memset(void* ptr, int value, size_t num);
extern void* memmove(void* dest, const void* src, size_t num);

int memcmp ( const void * ptr1, const void * ptr2, size_t num );

size_t strlen(const char* str);
int strcmp(const char* str1, const char* str2);
int strcasecmp(const char* str1, const char* str2);
char* strtok(char* str, const char* delimiters);
const char* strchr(const char* str, int character);

char* strcat(char* destination, const char* source);
char* strcpy(char* destination, const char* source);

#endif