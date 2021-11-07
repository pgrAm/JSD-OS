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
bp_OEM:					db "JSD OS  "
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
bs_drive_number: 	   	db _drive_number 
bs_unused: 				db _unused		
bs_ext_boot_signature: 	db _ext_boot_signature
bs_serial_number:	    dd _serial_number	    
bs_volume_label: 	    db "FLOPPY     "
bs_file_system: 	    db "FAT12   "

KERNEL_OFFSET  equ 0x9000 ; This is the memory address where our kernel goes

MULTIBOOT_OFFSET equ 0x7000

RAMDISK_DATA equ MULTIBOOT_OFFSET + multiboot_info_size

begin:

	xor	ax, ax
	mov	ds, ax
	mov es, ax
	mov	ss, ax
	mov	sp, 0x6000
	mov	bp, sp 

	mov [MULTIBOOT_OFFSET + multiboot_info.bootDevice], dl 	; Save the index of the boot drive

	call getmem

	mov bx, KERNEL_OFFSET
	;mov si, ramdisk_begin
	;call store_as_long_address

	mov si, kernel_file_name
	call fat_load

	mov si, RAMDISK_DATA + multiboot_module.begin
	call store_as_long_address

	mov si, raminit_file_name
	call fat_load

	mov si, RAMDISK_DATA + multiboot_module.end
	call store_as_long_address

switchtopm:
	cli
	lgdt [gdt_descriptor] ; Load our global descriptor table , which defines
	mov eax, cr0	; To make the switch to protected mode , we set
	or	eax, 0x01	; the first bit of CR0 , a control register
	mov cr0, eax
	jmp CODE_SEG : init_pm ; Make a far jump ( i.e. to a new segment ) to our 32 - bit

	[bits 32]
; Initialise registers and the stack once in PM.
init_pm :
	mov ax, DATA_SEG	; Now in PM , our old segments are meaningless ,
	mov ds, ax			; so we point our segment registers to the
	mov ss, ax			; data selector we defined in our GDT
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ebp, 0x90000	; Update our stack position so it is right
	mov esp, ebp		; at the top of the free space.

; In protected mode now
BEGIN_PM :

	mov ebx, MULTIBOOT_OFFSET
	xor eax, eax
	inc eax
	mov [ebx + multiboot_info.mods_count], eax
	mov eax, RAMDISK_DATA
	mov [ebx + multiboot_info.mods_addr], eax

	mov 	eax, 0x2badb002			;multiboot magic	

	call 	KERNEL_OFFSET 			; Jumping into C and Kernel land
	
%include "disk.asm"
%include "memmap.asm"
%include "gdt.asm"
%include "multiboot.asm"
%include "fatload.asm"

store_as_long_address:
	mov ax, es
	;mov cx, ax
	;shl cx, 4
	;add bx, cx
	mov word [si], bx
	shr ax, 0x0c
	mov word [si + 0x02], ax
	ret

kernel_file_name   db "KERNAL  SYS"
raminit_file_name  db "INIT    RFS"


; Bootsector padding
times 510 -( $ - $$ ) db 0

; magic number
dw 0xaa55