[bits 32]
global set_page_directory
set_page_directory:
	mov eax, [esp + 4]
	mov cr3, eax
	ret

global enable_paging
enable_paging:
	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax
	ret
	
global get_page_directory
get_page_directory:
	mov eax, cr0
	ret