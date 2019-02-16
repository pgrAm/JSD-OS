	.file	"video.c"
	.text
	.globl	_port_byte_out
	.def	_port_byte_out;	.scl	2;	.type	32;	.endef
_port_byte_out:
LFB0:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$8, %esp
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	movw	%dx, -4(%ebp)
	movb	%al, -8(%ebp)
	movzbl	-8(%ebp), %eax
	movzwl	-4(%ebp), %edx
/APP
 # 14 "video.c" 1
	 out %al , %dx 
 # 0 "" 2
/NO_APP
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE0:
	.globl	_set_cursor
	.def	_set_cursor;	.scl	2;	.type	32;	.endef
_set_cursor:
LFB1:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$32, %esp
	movl	8(%ebp), %edx
	movl	12(%ebp), %eax
	movw	%dx, -20(%ebp)
	movw	%ax, -24(%ebp)
	movzwl	-20(%ebp), %edx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	sall	$4, %eax
	movl	%eax, %edx
	movzwl	-24(%ebp), %eax
	addl	%edx, %eax
	movw	%ax, -2(%ebp)
	movl	$15, 4(%esp)
	movl	$980, (%esp)
	call	_port_byte_out
	movzwl	-2(%ebp), %eax
	movzbl	%al, %eax
	movl	%eax, 4(%esp)
	movl	$981, (%esp)
	call	_port_byte_out
	movl	$14, 4(%esp)
	movl	$980, (%esp)
	call	_port_byte_out
	movzwl	-2(%ebp), %eax
	shrw	$8, %ax
	movzbl	%al, %eax
	movl	%eax, 4(%esp)
	movl	$981, (%esp)
	call	_port_byte_out
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE1:
	.globl	_print_string
	.def	_print_string;	.scl	2;	.type	32;	.endef
_print_string:
LFB2:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	$753664, -4(%ebp)
	movl	-4(%ebp), %eax
	addl	$4000, %eax
	movl	%eax, -12(%ebp)
	movl	8(%ebp), %eax
	movl	%eax, -8(%ebp)
	jmp	L4
L8:
	movl	-8(%ebp), %eax
	movzbl	(%eax), %eax
	cmpb	$10, %al
	jne	L5
	movl	-4(%ebp), %eax
	leal	-753664(%eax), %ecx
	movl	$1717986919, %edx
	movl	%ecx, %eax
	imull	%edx
	sarl	$5, %edx
	movl	%ecx, %eax
	sarl	$31, %eax
	subl	%eax, %edx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	sall	$4, %eax
	subl	%eax, %ecx
	movl	%ecx, %edx
	movl	$160, %eax
	subl	%edx, %eax
	addl	%eax, -4(%ebp)
	jmp	L6
L5:
	movl	-8(%ebp), %eax
	movzbl	(%eax), %edx
	movl	-4(%ebp), %eax
	movb	%dl, (%eax)
	addl	$2, -4(%ebp)
L6:
	addl	$1, -8(%ebp)
L4:
	movl	-8(%ebp), %eax
	movzbl	(%eax), %eax
	testb	%al, %al
	je	L3
	movl	-4(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jbe	L8
L3:
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE2:
	.globl	_clear_screen
	.def	_clear_screen;	.scl	2;	.type	32;	.endef
_clear_screen:
LFB3:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$16, %esp
	movl	$753664, -4(%ebp)
	movl	-4(%ebp), %eax
	addl	$4000, %eax
	movl	%eax, -8(%ebp)
	jmp	L10
L11:
	movl	-4(%ebp), %eax
	movw	$3840, (%eax)
	addl	$2, -4(%ebp)
L10:
	movl	-4(%ebp), %eax
	cmpl	-8(%ebp), %eax
	jb	L11
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE3:
	.globl	_clear_row
	.def	_clear_row;	.scl	2;	.type	32;	.endef
_clear_row:
LFB4:
	.cfi_startproc
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset 5, -8
	movl	%esp, %ebp
	.cfi_def_cfa_register 5
	subl	$20, %esp
	movl	8(%ebp), %eax
	movw	%ax, -20(%ebp)
	movzwl	-20(%ebp), %edx
	movl	%edx, %eax
	sall	$2, %eax
	addl	%edx, %eax
	sall	$5, %eax
	addl	$753664, %eax
	movl	%eax, -4(%ebp)
	movl	-4(%ebp), %eax
	addl	$160, %eax
	movl	%eax, -8(%ebp)
	jmp	L13
L14:
	movl	-4(%ebp), %eax
	movw	$3840, (%eax)
	addl	$2, -4(%ebp)
L13:
	movl	-4(%ebp), %eax
	cmpl	-8(%ebp), %eax
	jb	L14
	leave
	.cfi_restore 5
	.cfi_def_cfa 4, 4
	ret
	.cfi_endproc
LFE4:
	.ident	"GCC: (GNU) 4.8.1"
