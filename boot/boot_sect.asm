[bits 16]
[org 0x7c00]
start: jmp begin
nop

%assign _bytes_per_sector		512
%assign _sectors_per_cluster	1
%assign _reserved_sectors		1
%assign _number_of_FATs			2
%assign _root_entries			224
%assign _total_sectors			2880
%assign _media					0xf8
%assign _sectors_per_FAT		9
%assign _sectors_per_track		18
%assign _heads_per_cylinder		2
%assign _hidden_sectors			0
%assign _total_sectors_big		0
%assign _drive_number			0
%assign _unused					0
%assign _ext_boot_signature		0x29
%assign _serial_number			0xa0a1a2a3

; BIOS Parameter Block
bp_OEM					db "EMS OS  "
bp_bytes_per_sector:  	dw _bytes_per_sector
bp_sectors_per_cluster: db _sectors_per_cluster
bp_reserved_sectors: 	dw _reserved_sectors
bp_number_of_FATs: 		db _number_of_FATs
bp_root_entries: 		dw _root_entries 	
bp_total_sectors: 		dw _total_sectors
bp_media: 				db _media	
bp_sectors_per_FAT: 	dw _sectors_per_FAT
bp_sectors_per_track: 	dw _sectors_per_track
bp_heads_per_cylinder: 	dw _heads_per_cylinder
bp_hidden_sectors: 		dd _hidden_sectors
bp_total_sectors_big:   dd _total_sectors_big
;bs_drive_number: 	   	db _drive_number 
;bs_unused: 				db _unused		
;bs_ext_boot_signature: 	db _ext_boot_signature
;bs_serial_number:	    dd _serial_number	    
;bs_volume_label: 	    db "EMS FLOPPY "
;bs_file_system: 	    db "FAT12   "

KERNEL_OFFSET 	equ 0x8000 ; This is the memory address where our kernel goes

begin:

	xor	ax, ax
	mov	ds, ax
	mov es, ax
	mov	ss, ax
	mov	sp, 0x6000
	mov	bp, sp
	
	mov [boot_info + multiboot_info.bootDevice], dl 	; Save the index of the boot drive

	call getmem
	
	call fat_load

switchtopm:
	cli ; We must switch of interrupts until we have
	; set -up the protected mode interrupt vector
	; otherwise interrupts will run riot.
	lgdt [gdt_descriptor] ; Load our global descriptor table , which defines
	; the protected mode segments ( e.g. for code and data )
	mov eax, cr0 ; To make the switch to protected mode , we set
	or eax, 0x1 ; the first bit of CR0 , a control register
	mov cr0, eax
	jmp CODE_SEG : init_pm ; Make a far jump ( i.e. to a new segment ) to our 32 - bit
	; code. This also forces the CPU to flush its cache of
	; pre - fetched and real - mode decoded instructions , which can
	; cause problems.
	[bits 32]
	; Initialise registers and the stack once in PM.
init_pm :
	mov ax, DATA_SEG ; Now in PM , our old segments are meaningless ,
	mov ds, ax ; so we point our segment registers to the
	mov ss, ax ; data selector we defined in our GDT
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ebp, 0x90000 ; Update our stack position so it is right
	mov esp, ebp	 ; at the top of the free space.

; In protected mode now
BEGIN_PM :
	mov 	eax, 0x2badb002			;multiboot magic
	mov 	ebx, boot_info
	
	call 	KERNEL_OFFSET 			; Jumping into C and Kernel land
	
%include "disk.asm"
%include "memmap.asm"
%include "gdt.asm"
%include "multiboot.asm"
;%include "switchtopm.asm"
%include "fatload.asm"

; Multiboot header
boot_info:
istruc multiboot_info
	at multiboot_info.flags,				dd 0
	at multiboot_info.memoryLo,				dd 0
	at multiboot_info.memoryHi,				dd 0
	at multiboot_info.bootDevice,			dd 0
	at multiboot_info.cmdLine,				dd 0
	at multiboot_info.mods_count,			dd 0
	at multiboot_info.mods_addr,			dd 0
	at multiboot_info.syms0,				dd 0
	at multiboot_info.syms1,				dd 0
	at multiboot_info.syms2,				dd 0
	at multiboot_info.mmap_length,			dd 0
	at multiboot_info.mmap_addr,			dd 0
	at multiboot_info.drives_length,		dd 0
	at multiboot_info.drives_addr,			dd 0
	at multiboot_info.config_table,			dd 0
	at multiboot_info.bootloader_name,		dd 0
	at multiboot_info.apm_table,			dd 0
	;at multiboot_info.vbe_control_info,	dd 0
	;at multiboot_info.vbe_mode_info,		dw 0
	;at multiboot_info.vbe_interface_seg,	dw 0
	;at multiboot_info.vbe_interface_off,	dw 0
	;at multiboot_info.vbe_interface_len,	dw 0
iend

; Bootsector padding
times 510 -( $ - $$ ) db 0

; magic number
dw 0xaa55