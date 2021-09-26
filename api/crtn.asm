; x86 crtn.s
section .init
	; clang will nicely put the contents of crtend.o's .init section here.
	pop ebp
	ret

section .fini
	; clang will nicely put the contents of crtend.o's .fini section here.
	pop ebp
	ret