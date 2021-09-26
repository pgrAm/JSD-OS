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

void _start()
{
	_init();

	handle_init_array();

	main(0, 0);

	handle_fini_array();

	_fini();
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
    signed i = __atexitFuncCount;
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