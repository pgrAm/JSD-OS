	.text
	.intel_syntax noprefix
	.file	"elftest.c"
	.globl	main                    # -- Begin function main
	.type	main,@function
main:                                   # @main
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	ebx
	xor	ebx, ebx
	mov	eax, offset .L.str
	#APP
	int	128
	#NO_APP
	mov	eax, 42
	pop	ebx
	pop	ebp
	ret
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
                                        # -- End function
	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"hello world\n"
	.size	.L.str, 13


	.ident	"clang version 7.0.0 (tags/RELEASE_700/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
