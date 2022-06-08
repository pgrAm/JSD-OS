#ifndef STRING_H
#define STRING_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

//extern void* memcpy(void* dest, const void* src, size_t num);
//extern void* memset(void* ptr, int value, size_t num);
extern void* memmove(void* dest, const void* src, size_t num);
void* memchr(const void* str, int c, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

size_t strlen(const char* str);
int strncmp(const char* lhs, const char* rhs, size_t count);
int strcmp(const char* str1, const char* str2);
char* strtok(char* str, const char* delimiters);
char* strchr(const char* str, int character);

char* strcat(char* destination, const char* source);
char* strcpy(char* destination, const char* source);

char* strncpy(char* dst, const char* src, size_t count);

char* strstr(const char* str, const char* substr);

char* strrchr(const char*, int);

#ifdef __cplusplus
}
#endif

#include <string386.inl>

#endif