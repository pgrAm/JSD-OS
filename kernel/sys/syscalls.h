#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#include <kernel/interrupt.h>
#include <kernel/syscall.h>
#include <kernel/filesystem.h>
#include <kernel/task.h>
#include <kernel/memorymanager.h>
#include <kernel/display.h>
#include <drivers/sysclock.h>
#include <drivers/kbrd.h>

#define print_string print_string_len
#define open filesystem_open_file
#define close filesystem_close_file
#define read filesystem_read_file
#define exit syscall_exit
#define master_time sysclock_get_master_time
#define clock_ticks sysclock_get_ticks
#define get_utc_offset sysclock_get_utc_offset
#define alloc_pages memmanager_virtual_alloc
#define free_pages memmanager_free_pages
#define getkey get_keypress
#define wait_and_getkey wait_and_get_keypress
#endif