#include <stdint.h>

extern uint8_t* gdt_tss;

struct tss_entry_struct
{
   uint32_t dont_care;
   uint32_t esp0;
   uint32_t ss0;
   uint32_t padding[23];
} __packed;