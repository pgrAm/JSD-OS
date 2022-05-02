
static inline void* __do_memcpy(void* dest, const void* src, size_t num)
{
    void* od = dest;
    uint32_t temp;
    __asm__ volatile(
        "   mov %%ecx, %[tmp]\n"
        "   shr $0x2, %%ecx\n"
        "   and $0x3, %[tmp]\n"
        "   cld\n"
        "   rep movsl\n"
        "   mov %[tmp], %%ecx\n"
        "   rep movsb\n"
        : "+D"(dest), "+S" (src), "+c" (num), [tmp]"=&r"(temp) // output
        :  // input
        : "cc", "memory" // clobbered register
        );
    return od;
}

static inline void* __do_memcpy4(void* dest, const void* src, size_t num)
{
    num /= 4;

    void* od = dest;
    __asm__ volatile(
        "   cld\n"
        "   rep movsl\n"
        : "+D"(dest), "+S" (src), "+c" (num) // output
        :  // input
        : "cc", "memory" // clobbered register
        );

    return od;
}

static inline void* memcpy(void* dst, const void* src, size_t size)
{
    if(__builtin_constant_p(size))
    {
        if(size <= sizeof(uintptr_t))
        {
			return __builtin_memcpy(dst, src, size);
        }
        else if((size & 3) == 0)
        {
			return __do_memcpy4(dst, src, size);
        }
    }

    if(__builtin_constant_p(size & 3) && (size & 3) == 0)
    {
		return __do_memcpy4(dst, src, size);
    }

    return __do_memcpy(dst, src, size);
}

static inline void* __do_memset4(void* dest, int value, size_t num)
{
    num /= 4;
    value &= 0xFF;

    const uint32_t v = (value << 24) | (value << 16) | (value << 8) | value;

    void* od = dest;
    __asm__ volatile(
        "   cld\n"
        "   rep stosl\n"
        : "+D"(dest), "+c" (num)  // output
        : "a"(v) // input
        : "cc", "memory" // clobbered register
        );
    return od;
}

static inline void* __do_memset(void* dest, int value, size_t num)
{
    const uint32_t v = (value << 24) | (value << 16) | (value << 8) | value;

    void* od = dest;
    uint32_t temp;
    __asm__ volatile(
        "   mov %%ecx, %[tmp]\n"
        "   shr $0x2, %%ecx\n"
        "   and $0x3, %[tmp]\n"
        "   cld\n"
        "   rep stosl\n"
        "   mov %[tmp], %%ecx\n"
        "   rep stosb\n"
        : "+D"(dest), "+c" (num), [tmp]"=&r"(temp) // output
        : "a" (v)// input
        : "cc", "memory" // clobbered register
        );
    return od;
}

static inline void* memset(void* a, int value, size_t size)
{
    if(__builtin_constant_p(size))
    {
        if(size <= sizeof(uintptr_t))
        {
            return __builtin_memset(a, value, size);
        }
        else if((size & 3) == 0)
        {
            return __do_memset4(a, value, size);
        }
    }

    if(__builtin_constant_p(size & 3) && (size & 3) == 0)
    {
        return __do_memset4(a, value, size);
    }

    return __do_memset(a, value, size);
}