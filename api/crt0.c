#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

extern void exit(int status);
extern int main(int argc, char** argv);
extern void _init();
extern void _fini();

typedef void func_t(void);
extern func_t *__init_array_start[], *__init_array_end[];
extern func_t *__fini_array_start[], *__fini_array_end[];

static void handle_init_array(void)
{
	for(func_t** func = __init_array_start; func != __init_array_end; func++)
		(*func)();
}

void handle_fini_array(void)
{
	for(func_t** func = __fini_array_start; func != __fini_array_end; func++)
		(*func)();
}

static const char* _find(const char* first, const char* last, char value)
{
	for(; first != last; ++first)
	{
		if(*first == value)
		{
			return first;
		}
	}
	return last;
}

static int count_tokens(const char* input, size_t input_size, char delim)
{
	int num_tokens = 0;

	const char* first = input;
	const char* last  = input + input_size;
	while(first < last)
	{
		const char* second = (const char*)_find(first, last, delim);

		if(first != second) num_tokens++;

		first = second + 1;
	}
	return num_tokens;
}

static void tokenize(const char** buf, const char* input, size_t input_size,
					 char delim)
{
	size_t index = 0;

	const char* first = input;
	const char* last  = input + input_size;
	while(first < last)
	{
		const char* second = (const char*)_find(first, last, delim);
		if(first != second)
		{
			buf[index++] = first;
		}
		first = second + 1;
	}
}

void _start(uintptr_t args)
{
	uintptr_t args_begin = (uintptr_t)&args + sizeof(uintptr_t);
	size_t args_size;
	memcpy(&args_size, (void*)args_begin, sizeof(size_t));

	char* args_ptr = (char*)(args_begin + sizeof(size_t));

	int argc = count_tokens(args_ptr, args_size, '\0');

	char** argv = (char**)malloc(sizeof(char**) * (size_t)argc);

	tokenize(argv, args_ptr, args_size, '\0');

	_init();

	handle_init_array();

	int r = main(argc, argv);

	handle_fini_array();

	_fini();

    exit(r);
}

void __cxa_pure_virtual() {
    // Do Nothing
}

#define ATEXIT_FUNC_MAX 128

typedef unsigned uarch_t;

struct atexitFuncEntry_t {
    void (*destructorFunc) (void*);
    void* objPtr;
    void* dsoHandle;

};

struct atexitFuncEntry_t __atexitFuncs[ATEXIT_FUNC_MAX];
uarch_t __atexitFuncCount = 0;

void* __dso_handle = 0;

int __cxa_atexit(void (*f)(void*), void* objptr, void* dso) {
    if(__atexitFuncCount >= ATEXIT_FUNC_MAX) {
        return -1;
    }
    __atexitFuncs[__atexitFuncCount].destructorFunc = f;
    __atexitFuncs[__atexitFuncCount].objPtr = objptr;
    __atexitFuncs[__atexitFuncCount].dsoHandle = dso;
    __atexitFuncCount++;
    return 0;
}

void __cxa_finalize(void* f) {
    unsigned int i = __atexitFuncCount;
    if(!f) {
        while(i--) {
            if(__atexitFuncs[i].destructorFunc) {
                (*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
            }
        }
        return;
    }

    for(; i >= 0; i--) {
        if(__atexitFuncs[i].destructorFunc == f) {
            (*__atexitFuncs[i].destructorFunc)(__atexitFuncs[i].objPtr);
            __atexitFuncs[i].destructorFunc = 0;
        }
    }
}