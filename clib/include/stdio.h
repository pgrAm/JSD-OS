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
typedef struct FILE FILE;

#ifndef __KERNEL
extern FILE* stderr;
extern FILE* stdout;

size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
long int ftell(FILE* stream);
int fflush(FILE* stream);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int fseek(FILE* stream, long offset, int origin);

int remove(const char* filename);
int rename(const char* old_path, const char* new_path);

int vfprintf(FILE* stream, const char* format, va_list arg);

#endif

void clearerr(FILE* stream);
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
//int fputc(int character, FILE* stream);
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
//int getc(FILE* stream);
int puts(const char* str);
void perror(const char* str);
//int fflush(FILE* stream);

void set_stdout(size_t (*write)(const char* buf, size_t size, void* impl));
void set_cwd(const char* cwd, size_t cwd_len);

#ifdef __cplusplus
}
#endif
#endif