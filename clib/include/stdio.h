#ifndef STDIO_H
#define STDIO_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <string.h>

#define EOF 0x04 //ascii EOT character

#ifndef NULL
#define NULL (void*)0
#endif

#define FILENAME_MAX 256

typedef size_t fpos_t;
typedef struct _internal_FILE FILE;

//extern FILE* stdin;

void clearerr(FILE* stream);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
int fputc(int character, FILE* stream);
int fputs(const char* str, FILE* stream);
int fprintf(FILE* stream, const char* format, ...);
int sprintf(char* s, const char * format, ...);
int snprintf(char* s, size_t n, const char* format, ...);
char *gets(char *str); //!blatantly unsafe function please do not use: BUFFER OVERFLOW!!!
char *gets_s(char *str, size_t length); //!safe version of gets, if you provide a valid length it will not overflow
int vsprintf(char *s, const char *format, va_list arg );
int vsnprintf(char *buffer, size_t n, const char* fmt, va_list args);
int printf(const char* format, ...);
int putchar(int character);
int getchar();
int getc(FILE* stream);
int puts(const char* str);
void perror(const char* str);
int fflush(FILE* stream);

#ifdef __cplusplus
}
#endif
#endif