static inline void* memcpy(void* dest, const void* src, size_t num)
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

static inline void* memset(void* dest, int value, size_t num)
{
    void* od = dest;
    __asm__ volatile(
        "   cld\n"
        "   rep stosb\n"
        : "+D"(dest), "+c" (num)  // output
        : "a"((uint8_t)value) // input
        : "cc", "memory" // clobbered register
        );
    return od;
}