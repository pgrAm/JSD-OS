#include <stdio.h>
#include "../kernel/interrupt.h"
#include "portio.h"
#include "kbrd.h"

void AT_keyboard_handler(interrupt_info *r)
{
	send_eoi(r);
	
	handle_keyevent(inb(0x60));
}

void AT_keyboard_init()
{
    irq_install_handler(1, AT_keyboard_handler);
}