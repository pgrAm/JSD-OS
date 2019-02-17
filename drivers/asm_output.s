	.file	"idt.c"
	.text
.globl idt_setup_handler
	.type	idt_setup_handler, @function
idt_setup_handler:
	pushl	%ebp
	movl	%esp, %ebp
	movl	12(%ebp), %ecx
	movzbl	8(%ebp), %edx
	movl	20(%ebp), %eax
	movb	%al, idt+5(,%edx,8)
	movl	16(%ebp), %eax
	movw	%ax, idt+2(,%edx,8)
	movb	$0, idt+4(,%edx,8)
	movw	%cx, idt(,%edx,8)
	shrl	$16, %ecx
	movw	%cx, idt+6(,%edx,8)
	leave
	ret
	.size	idt_setup_handler, .-idt_setup_handler
.globl irq_install_handler
	.type	irq_install_handler, @function
irq_install_handler:
	pushl	%ebp
	movl	%esp, %ebp
	movl	12(%ebp), %edx
	movl	8(%ebp), %eax
	movl	%edx, irq_routines(,%eax,4)
	leave
	ret
	.size	irq_install_handler, .-irq_install_handler
.globl irq_uninstall_handler
	.type	irq_uninstall_handler, @function
irq_uninstall_handler:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	$0, irq_routines(,%eax,4)
	leave
	ret
	.size	irq_uninstall_handler, .-irq_uninstall_handler
.globl isrs_init
	.type	isrs_init, @function
isrs_init:
	pushl	%ebp
	movl	%esp, %ebp
	movb	$-114, idt+5
	movw	$8, idt+2
	movb	$0, idt+4
	movl	$isr0, %eax
	movw	%ax, idt
	shrl	$16, %eax
	movw	%ax, idt+6
	movb	$-114, idt+13
	movw	$8, idt+10
	movb	$0, idt+12
	movl	$isr1, %eax
	movw	%ax, idt+8
	shrl	$16, %eax
	movw	%ax, idt+14
	movb	$-114, idt+21
	movw	$8, idt+18
	movb	$0, idt+20
	movl	$isr2, %eax
	movw	%ax, idt+16
	shrl	$16, %eax
	movw	%ax, idt+22
	movb	$-114, idt+29
	movw	$8, idt+26
	movb	$0, idt+28
	movl	$isr3, %eax
	movw	%ax, idt+24
	shrl	$16, %eax
	movw	%ax, idt+30
	movb	$-114, idt+37
	movw	$8, idt+34
	movb	$0, idt+36
	movl	$isr4, %eax
	movw	%ax, idt+32
	shrl	$16, %eax
	movw	%ax, idt+38
	movb	$-114, idt+45
	movw	$8, idt+42
	movb	$0, idt+44
	movl	$isr5, %eax
	movw	%ax, idt+40
	shrl	$16, %eax
	movw	%ax, idt+46
	movb	$-114, idt+53
	movw	$8, idt+50
	movb	$0, idt+52
	movl	$isr6, %eax
	movw	%ax, idt+48
	shrl	$16, %eax
	movw	%ax, idt+54
	movb	$-114, idt+61
	movw	$8, idt+58
	movb	$0, idt+60
	movl	$isr7, %eax
	movw	%ax, idt+56
	shrl	$16, %eax
	movw	%ax, idt+62
	movb	$-114, idt+69
	movw	$8, idt+66
	movb	$0, idt+68
	movl	$isr8, %eax
	movw	%ax, idt+64
	shrl	$16, %eax
	movw	%ax, idt+70
	movb	$-114, idt+77
	movw	$8, idt+74
	movb	$0, idt+76
	movl	$isr9, %eax
	movw	%ax, idt+72
	shrl	$16, %eax
	movw	%ax, idt+78
	movb	$-114, idt+85
	movw	$8, idt+82
	movb	$0, idt+84
	movl	$isr10, %eax
	movw	%ax, idt+80
	shrl	$16, %eax
	movw	%ax, idt+86
	movb	$-114, idt+93
	movw	$8, idt+90
	movb	$0, idt+92
	movl	$isr11, %eax
	movw	%ax, idt+88
	shrl	$16, %eax
	movw	%ax, idt+94
	movb	$-114, idt+101
	movw	$8, idt+98
	movb	$0, idt+100
	movl	$isr12, %eax
	movw	%ax, idt+96
	shrl	$16, %eax
	movw	%ax, idt+102
	movb	$-114, idt+109
	movw	$8, idt+106
	movb	$0, idt+108
	movl	$isr13, %eax
	movw	%ax, idt+104
	shrl	$16, %eax
	movw	%ax, idt+110
	movb	$-114, idt+117
	movw	$8, idt+114
	movb	$0, idt+116
	movl	$isr14, %eax
	movw	%ax, idt+112
	shrl	$16, %eax
	movw	%ax, idt+118
	movb	$-114, idt+125
	movw	$8, idt+122
	movb	$0, idt+124
	movl	$isr15, %eax
	movw	%ax, idt+120
	shrl	$16, %eax
	movw	%ax, idt+126
	movb	$-114, idt+133
	movw	$8, idt+130
	movb	$0, idt+132
	movl	$isr16, %eax
	movw	%ax, idt+128
	shrl	$16, %eax
	movw	%ax, idt+134
	movb	$-114, idt+141
	movw	$8, idt+138
	movb	$0, idt+140
	movl	$isr17, %eax
	movw	%ax, idt+136
	shrl	$16, %eax
	movw	%ax, idt+142
	movb	$-114, idt+149
	movw	$8, idt+146
	movb	$0, idt+148
	movl	$isr18, %eax
	movw	%ax, idt+144
	shrl	$16, %eax
	movw	%ax, idt+150
	movb	$-114, idt+157
	movw	$8, idt+154
	movb	$0, idt+156
	movl	$isr19, %eax
	movw	%ax, idt+152
	shrl	$16, %eax
	movw	%ax, idt+158
	movb	$-114, idt+165
	movw	$8, idt+162
	movb	$0, idt+164
	movl	$isr20, %eax
	movw	%ax, idt+160
	shrl	$16, %eax
	movw	%ax, idt+166
	movb	$-114, idt+173
	movw	$8, idt+170
	movb	$0, idt+172
	movl	$isr21, %eax
	movw	%ax, idt+168
	shrl	$16, %eax
	movw	%ax, idt+174
	movb	$-114, idt+181
	movw	$8, idt+178
	movb	$0, idt+180
	movl	$isr22, %eax
	movw	%ax, idt+176
	shrl	$16, %eax
	movw	%ax, idt+182
	movb	$-114, idt+189
	movw	$8, idt+186
	movb	$0, idt+188
	movl	$isr23, %eax
	movw	%ax, idt+184
	shrl	$16, %eax
	movw	%ax, idt+190
	movb	$-114, idt+197
	movw	$8, idt+194
	movb	$0, idt+196
	movl	$isr24, %eax
	movw	%ax, idt+192
	shrl	$16, %eax
	movw	%ax, idt+198
	movb	$-114, idt+205
	movw	$8, idt+202
	movb	$0, idt+204
	movl	$isr25, %eax
	movw	%ax, idt+200
	shrl	$16, %eax
	movw	%ax, idt+206
	movb	$-114, idt+213
	movw	$8, idt+210
	movb	$0, idt+212
	movl	$isr26, %eax
	movw	%ax, idt+208
	shrl	$16, %eax
	movw	%ax, idt+214
	movb	$-114, idt+221
	movw	$8, idt+218
	movb	$0, idt+220
	movl	$isr27, %eax
	movw	%ax, idt+216
	shrl	$16, %eax
	movw	%ax, idt+222
	movb	$-114, idt+229
	movw	$8, idt+226
	movb	$0, idt+228
	movl	$isr28, %eax
	movw	%ax, idt+224
	shrl	$16, %eax
	movw	%ax, idt+230
	movb	$-114, idt+237
	movw	$8, idt+234
	movb	$0, idt+236
	movl	$isr29, %eax
	movw	%ax, idt+232
	shrl	$16, %eax
	movw	%ax, idt+238
	movb	$-114, idt+245
	movw	$8, idt+242
	movb	$0, idt+244
	movl	$isr30, %eax
	movw	%ax, idt+240
	shrl	$16, %eax
	movw	%ax, idt+246
	movb	$-114, idt+253
	movw	$8, idt+250
	movb	$0, idt+252
	movl	$isr31, %eax
	movw	%ax, idt+248
	shrl	$16, %eax
	movw	%ax, idt+254
	leave
	ret
	.size	isrs_init, .-isrs_init
.globl irq_handler
	.type	irq_handler, @function
irq_handler:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$4, %esp
	movl	8(%ebp), %ebx
	movl	48(%ebx), %eax
	movl	irq_routines-128(,%eax,4), %eax
	testl	%eax, %eax
	je	.L10
	subl	$12, %esp
	pushl	%ebx
	call	*%eax
	addl	$16, %esp
.L10:
	cmpl	$39, 48(%ebx)
	jbe	.L11
	pushl	%edx
	pushl	%edx
	pushl	$32
	pushl	$160
	call	outb
	addl	$16, %esp
.L11:
	pushl	%eax
	pushl	%eax
	pushl	$32
	pushl	$32
	call	outb
	addl	$16, %esp
	movl	-4(%ebp), %ebx
	leave
	ret
	.size	irq_handler, .-irq_handler
.globl irq_remap
	.type	irq_remap, @function
irq_remap:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	pushl	$17
	pushl	$32
	call	outb
	popl	%ecx
	popl	%eax
	pushl	$17
	pushl	$160
	call	outb
	popl	%eax
	popl	%edx
	pushl	$32
	pushl	$33
	call	outb
	popl	%ecx
	popl	%eax
	pushl	$40
	pushl	$161
	call	outb
	popl	%eax
	popl	%edx
	pushl	$4
	pushl	$33
	call	outb
	popl	%ecx
	popl	%eax
	pushl	$2
	pushl	$161
	call	outb
	popl	%eax
	popl	%edx
	pushl	$1
	pushl	$33
	call	outb
	popl	%ecx
	popl	%eax
	pushl	$1
	pushl	$161
	call	outb
	popl	%eax
	popl	%edx
	pushl	$0
	pushl	$33
	call	outb
	popl	%ecx
	popl	%eax
	pushl	$0
	pushl	$161
	call	outb
	addl	$16, %esp
	leave
	ret
	.size	irq_remap, .-irq_remap
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	" Exception. System Halted!\n"
.LC1:
	.string	"CS=%X\n"
.LC2:
	.string	"EIP=%X\n"
.LC3:
	.string	"ESP=%X\n"
.LC4:
	.string	"user "
.LC5:
	.string	"write "
.LC6:
	.string	"read "
.LC7:
	.string	"protection "
.LC8:
	.string	"invalid page "
.LC9:
	.string	"violation\n"
.LC10:
	.string	"At Adress %X"
.LC11:
	.string	"ERR CODE = %X\n"
	.text
.globl fault_handler
	.type	fault_handler, @function
fault_handler:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$4, %esp
	movl	8(%ebp), %ebx
	movl	48(%ebx), %eax
	cmpl	$31, %eax
	ja	.L24
	subl	$12, %esp
	pushl	exception_messages(,%eax,4)
	call	print_string
	movl	$.LC0, (%esp)
	call	print_string
	popl	%eax
	popl	%edx
	pushl	60(%ebx)
	pushl	$.LC1
	call	printf
	popl	%ecx
	popl	%eax
	pushl	56(%ebx)
	pushl	$.LC2
	call	printf
	popl	%eax
	popl	%edx
	pushl	28(%ebx)
	pushl	$.LC3
	call	printf
	movl	48(%ebx), %eax
	addl	$16, %esp
	cmpl	$14, %eax
	jne	.L17
	testb	$4, 52(%ebx)
	je	.L18
	subl	$12, %esp
	pushl	$.LC4
	call	print_string
	addl	$16, %esp
.L18:
	testb	$2, 52(%ebx)
	je	.L19
	subl	$12, %esp
	pushl	$.LC5
	jmp	.L26
.L19:
	subl	$12, %esp
	pushl	$.LC6
.L26:
	call	print_string
	addl	$16, %esp
	testb	$1, 52(%ebx)
	je	.L21
	subl	$12, %esp
	pushl	$.LC7
	jmp	.L27
.L21:
	subl	$12, %esp
	pushl	$.LC8
.L27:
	call	print_string
	movl	$.LC9, (%esp)
	call	print_string
	call	getcr2reg
	popl	%edx
	popl	%ecx
	pushl	%eax
	pushl	$.LC10
	call	printf
	addl	$16, %esp
	jmp	.L23
.L17:
	cmpl	$13, %eax
	jne	.L23
	pushl	%eax
	pushl	%eax
	pushl	52(%ebx)
	pushl	$.LC11
	call	printf
	addl	$16, %esp
.L23:
.L25:
	jmp	.L25
.L24:
	movl	-4(%ebp), %ebx
	leave
	ret
	.size	fault_handler, .-fault_handler
.globl irqs_init
	.type	irqs_init, @function
irqs_init:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$12, %esp
	pushl	$64
	pushl	$0
	pushl	$irq_routines
	call	memset
	call	irq_remap
	movb	$-114, idt+261
	movw	$8, idt+258
	movb	$0, idt+260
	movl	$irq0, %eax
	movw	%ax, idt+256
	shrl	$16, %eax
	movw	%ax, idt+262
	movb	$-114, idt+269
	movw	$8, idt+266
	movb	$0, idt+268
	movl	$irq1, %eax
	movw	%ax, idt+264
	shrl	$16, %eax
	movw	%ax, idt+270
	movb	$-114, idt+277
	movw	$8, idt+274
	movb	$0, idt+276
	movl	$irq2, %eax
	movw	%ax, idt+272
	shrl	$16, %eax
	movw	%ax, idt+278
	movb	$-114, idt+285
	movw	$8, idt+282
	movb	$0, idt+284
	movl	$irq3, %eax
	movw	%ax, idt+280
	shrl	$16, %eax
	movw	%ax, idt+286
	movb	$-114, idt+293
	movw	$8, idt+290
	movb	$0, idt+292
	movl	$irq4, %eax
	movw	%ax, idt+288
	shrl	$16, %eax
	movw	%ax, idt+294
	movb	$-114, idt+301
	movw	$8, idt+298
	movb	$0, idt+300
	movl	$irq5, %eax
	movw	%ax, idt+296
	shrl	$16, %eax
	movw	%ax, idt+302
	movb	$-114, idt+309
	movw	$8, idt+306
	movb	$0, idt+308
	movl	$irq6, %eax
	movw	%ax, idt+304
	shrl	$16, %eax
	movw	%ax, idt+310
	movb	$-114, idt+317
	movw	$8, idt+314
	movb	$0, idt+316
	movl	$irq7, %eax
	movw	%ax, idt+312
	shrl	$16, %eax
	movw	%ax, idt+318
	movb	$-114, idt+325
	movw	$8, idt+322
	movb	$0, idt+324
	movl	$irq8, %eax
	movw	%ax, idt+320
	shrl	$16, %eax
	movw	%ax, idt+326
	movb	$-114, idt+333
	movw	$8, idt+330
	movb	$0, idt+332
	movl	$irq9, %eax
	movw	%ax, idt+328
	shrl	$16, %eax
	movw	%ax, idt+334
	movb	$-114, idt+341
	movw	$8, idt+338
	movb	$0, idt+340
	movl	$irq10, %eax
	movw	%ax, idt+336
	shrl	$16, %eax
	movw	%ax, idt+342
	movb	$-114, idt+349
	movw	$8, idt+346
	movb	$0, idt+348
	movl	$irq11, %eax
	movw	%ax, idt+344
	shrl	$16, %eax
	movw	%ax, idt+350
	movb	$-114, idt+357
	movw	$8, idt+354
	movb	$0, idt+356
	movl	$irq12, %eax
	movw	%ax, idt+352
	shrl	$16, %eax
	movw	%ax, idt+358
	movb	$-114, idt+365
	movw	$8, idt+362
	movb	$0, idt+364
	movl	$irq13, %eax
	movw	%ax, idt+360
	shrl	$16, %eax
	movw	%ax, idt+366
	movb	$-114, idt+373
	movw	$8, idt+370
	movb	$0, idt+372
	movl	$irq14, %eax
	movw	%ax, idt+368
	shrl	$16, %eax
	movw	%ax, idt+374
	movb	$-114, idt+381
	movw	$8, idt+378
	movb	$0, idt+380
	movl	$irq15, %eax
	movw	%ax, idt+376
	shrl	$16, %eax
	movw	%ax, idt+382
	addl	$16, %esp
	leave
	ret
	.size	irqs_init, .-irqs_init
.globl idt_init
	.type	idt_init, @function
idt_init:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$12, %esp
	movw	$2047, idtp
	movl	$idt, idtp+2
	pushl	$2048
	pushl	$0
	pushl	$idt
	call	memset
	addl	$16, %esp
	leave
	jmp	idt_load
	.size	idt_init, .-idt_init
.globl exception_messages
	.section	.rodata.str1.1
.LC12:
	.string	"Division By Zero"
.LC13:
	.string	"Debug"
.LC14:
	.string	"Non Maskable Interrupt"
.LC15:
	.string	"Breakpoint"
.LC16:
	.string	"Into Detected Overflow"
.LC17:
	.string	"Out of Bounds"
.LC18:
	.string	"Invalid Opcode"
.LC19:
	.string	"No Coprocessor"
.LC20:
	.string	"Double Fault"
.LC21:
	.string	"Coprocessor Segment Overrun"
.LC22:
	.string	"Bad TSS"
.LC23:
	.string	"Segment Not Present"
.LC24:
	.string	"Stack Fault"
.LC25:
	.string	"General Protection Fault"
.LC26:
	.string	"Page Fault"
.LC27:
	.string	"Unknown Interrupt"
.LC28:
	.string	"Coprocessor Fault"
.LC29:
	.string	"Alignment Check"
.LC30:
	.string	"Machine Check"
.LC31:
	.string	"Reserved"
	.data
	.align 4
	.type	exception_messages, @object
	.size	exception_messages, 128
exception_messages:
	.long	.LC12
	.long	.LC13
	.long	.LC14
	.long	.LC15
	.long	.LC16
	.long	.LC17
	.long	.LC18
	.long	.LC19
	.long	.LC20
	.long	.LC21
	.long	.LC22
	.long	.LC23
	.long	.LC24
	.long	.LC25
	.long	.LC26
	.long	.LC27
	.long	.LC28
	.long	.LC29
	.long	.LC30
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
	.long	.LC31
.globl irq_routines
	.section	.bss
	.align 4
	.type	irq_routines, @object
	.size	irq_routines, 64
irq_routines:
	.zero	64
	.comm	idt,2048,4
	.comm	idtp,6,4
	.ident	"GCC: (eCosCentric GNU tools 4.3.2-sw) 4.3.2"
