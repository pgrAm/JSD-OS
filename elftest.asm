	.text
	.intel_syntax noprefix
	.file	"elftest.c"
	.globl	_start                  # -- Begin function _start
	.type	_start,@function
_start:                                 # @_start
# %bb.0:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	edi
	push	esi
	sub	esp, 5
	xor	esi, esi
	xor	ebx, ebx
	mov	eax, offset .L.str
	#APP
	int	128
	#NO_APP
	xor	ebx, ebx
	inc	ebx
	mov	eax, offset .L.str.1
	xor	ecx, ecx
	#APP
	int	128
	#NO_APP
	test	eax, eax
	je	.LBB0_1
# %bb.2:
	mov	edi, eax
	lea	esi, [ebp - 17]
	mov	ebx, 3
	mov	ecx, 4
	mov	eax, esi
	mov	edx, edi
	#APP
	int	128
	#NO_APP
	mov	ebx, 2
	mov	eax, edi
	#APP
	int	128
	#NO_APP
	mov	byte ptr [esi + 4], 0
	xor	ebx, ebx
	mov	eax, esi
	#APP
	int	128
	#NO_APP
	xor	ebx, ebx
	mov	eax, offset .L.str.3
	#APP
	int	128
	#NO_APP
	mov	esi, 42
	mov	ebx, 4
	mov	eax, 42
	#APP
	int	128
	#NO_APP
	jmp	.LBB0_3
.LBB0_1:
	xor	ebx, ebx
	mov	eax, offset .L.str.2
	#APP
	int	128
	#NO_APP
.LBB0_3:
	mov	eax, esi
	add	esp, 5
	pop	esi
	pop	edi
	pop	ebx
	pop	ebp
	ret
.Lfunc_end0:
	.size	_start, .Lfunc_end0-_start
                                        # -- End function
	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"testing\n"
	.size	.L.str, 9

	.type	.L.str.1,@object        # @.str.1
.L.str.1:
	.asciz	"test.elf"
	.size	.L.str.1, 9

	.type	.L.str.2,@object        # @.str.2
.L.str.2:
	.asciz	"cannot open file\n"
	.size	.L.str.2, 18

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	"\n"
	.size	.L.str.3, 2


	.ident	"clang version 7.0.0 (tags/RELEASE_700/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
