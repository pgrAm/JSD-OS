	.text
	.file	"memorymanager.c"
	.globl	memmanager_get_pt_entry # -- Begin function memmanager_get_pt_entry
	.type	memmanager_get_pt_entry,@function
memmanager_get_pt_entry:                # @memmanager_get_pt_entry
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %ecx
	movl	%ecx, %edx
	shrl	$22, %edx
	movl	-4096(,%edx,4), %eax
	testb	$1, %al
	je	.LBB0_2
# %bb.1:
	shrl	$10, %ecx
	movl	$-4194304, %eax         # imm = 0xFFC00000
	shll	$12, %edx
	addl	%eax, %edx
	andl	$4092, %ecx             # imm = 0xFFC
	movl	(%ecx,%edx), %eax
.LBB0_2:
	popl	%ebp
	retl
.Lfunc_end0:
	.size	memmanager_get_pt_entry, .Lfunc_end0-memmanager_get_pt_entry
                                        # -- End function
	.globl	memmanager_get_physical # -- Begin function memmanager_get_physical
	.type	memmanager_get_physical,@function
memmanager_get_physical:                # @memmanager_get_physical
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	movl	8(%ebp), %eax
	movl	%eax, %ecx
	shrl	$22, %ecx
	movl	-4096(,%ecx,4), %edx
	testb	$1, %dl
	je	.LBB1_2
# %bb.1:
	movl	$-4194304, %edx         # imm = 0xFFC00000
	shll	$12, %ecx
	addl	%edx, %ecx
	movl	%eax, %edx
	shrl	$10, %edx
	andl	$4092, %edx             # imm = 0xFFC
	movl	(%edx,%ecx), %edx
.LBB1_2:
	andl	$-4096, %edx            # imm = 0xF000
	andl	$4095, %eax             # imm = 0xFFF
	orl	%edx, %eax
	popl	%ebp
	retl
.Lfunc_end1:
	.size	memmanager_get_physical, .Lfunc_end1-memmanager_get_physical
                                        # -- End function
	.globl	memmanager_allocate_physical_page # -- Begin function memmanager_allocate_physical_page
	.type	memmanager_allocate_physical_page,@function
memmanager_allocate_physical_page:      # @memmanager_allocate_physical_page
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	movl	num_memory_blocks, %ecx
	testl	%ecx, %ecx
	je	.LBB2_5
# %bb.1:
	xorl	%edx, %edx
.LBB2_3:                                # =>This Inner Loop Header: Depth=1
	movl	memory_map(,%edx,8), %esi
	leal	4095(%esi), %eax
	andl	$-4096, %eax            # imm = 0xF000
	movl	%eax, %edi
	subl	%esi, %edi
	movl	memory_map+4(,%edx,8), %esi
	addl	$4096, %edi             # imm = 0x1000
	subl	%edi, %esi
	jae	.LBB2_4
# %bb.2:                                #   in Loop: Header=BB2_3 Depth=1
	incl	%edx
	cmpl	%ecx, %edx
	jb	.LBB2_3
.LBB2_5:
	pushl	$.L.str
	calll	printf
	addl	$4, %esp
	xorl	%eax, %eax
	jmp	.LBB2_6
.LBB2_4:
	leal	4096(%eax), %ecx
	movl	%ecx, memory_map(,%edx,8)
	movl	%esi, memory_map+4(,%edx,8)
.LBB2_6:
	popl	%esi
	popl	%edi
	popl	%ebp
	retl
.Lfunc_end2:
	.size	memmanager_allocate_physical_page, .Lfunc_end2-memmanager_allocate_physical_page
                                        # -- End function
	.globl	memmanager_create_new_page_table # -- Begin function memmanager_create_new_page_table
	.type	memmanager_create_new_page_table,@function
memmanager_create_new_page_table:       # @memmanager_create_new_page_table
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	movl	8(%ebp), %esi
	calll	memmanager_allocate_physical_page
	orl	$7, %eax
	movl	%eax, -4096(,%esi,4)
	pushl	%eax
	pushl	$.L.str.1
	calll	printf
	addl	$8, %esp
	shll	$12, %esi
	addl	$-4194304, %esi         # imm = 0xFFC00000
	pushl	$4096                   # imm = 0x1000
	pushl	$0
	pushl	%esi
	calll	memset
	addl	$12, %esp
	popl	%esi
	popl	%ebp
	retl
.Lfunc_end3:
	.size	memmanager_create_new_page_table, .Lfunc_end3-memmanager_create_new_page_table
                                        # -- End function
	.globl	memmanager_get_unmapped_pages # -- Begin function memmanager_get_unmapped_pages
	.type	memmanager_get_unmapped_pages,@function
memmanager_get_unmapped_pages:          # @memmanager_get_unmapped_pages
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%eax
	movl	8(%ebp), %edi
	movl	$-4194304, %ebx         # imm = 0xFFC00000
	xorl	%esi, %esi
	shll	$12, %edi
	negl	%edi
	movl	%edi, -16(%ebp)         # 4-byte Spill
.LBB4_1:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB4_7 Depth 2
	testb	$1, -4096(,%esi,4)
	jne	.LBB4_4
# %bb.2:                                #   in Loop: Header=BB4_1 Depth=1
	pushl	%esi
	calll	memmanager_create_new_page_table
	addl	$4, %esp
	cmpl	$1023, 8(%ebp)          # imm = 0x3FF
	jbe	.LBB4_3
.LBB4_4:                                #   in Loop: Header=BB4_1 Depth=1
	movl	12(%ebp), %eax
	testb	$4, %al
	je	.LBB4_6
# %bb.5:                                #   in Loop: Header=BB4_1 Depth=1
	leal	-4096(,%esi,4), %eax
	testb	$4, (%eax)
	je	.LBB4_9
.LBB4_6:                                #   in Loop: Header=BB4_1 Depth=1
	movl	-16(%ebp), %eax         # 4-byte Reload
	xorl	%ecx, %ecx
	xorl	%edx, %edx
.LBB4_7:                                #   Parent Loop BB4_1 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	cmpl	$1023, %edx             # imm = 0x3FF
	ja	.LBB4_9
# %bb.8:                                #   in Loop: Header=BB4_7 Depth=2
	incl	%ecx
	testb	$1, (%ebx,%edx,4)
	movl	$0, %edi
	leal	1(%edx), %edx
	cmovnel	%edi, %ecx
	addl	$4096, %eax             # imm = 0x1000
	cmpl	8(%ebp), %ecx
	jne	.LBB4_7
	jmp	.LBB4_11
.LBB4_9:                                #   in Loop: Header=BB4_1 Depth=1
	addl	$4194304, -16(%ebp)     # 4-byte Folded Spill
                                        # imm = 0x400000
	incl	%esi
	addl	$4096, %ebx             # imm = 0x1000
	cmpl	$1024, %esi             # imm = 0x400
	jb	.LBB4_1
# %bb.10:
	xorl	%eax, %eax
	jmp	.LBB4_11
.LBB4_3:
	shll	$22, %esi
	movl	%esi, %eax
.LBB4_11:
	addl	$4, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end4:
	.size	memmanager_get_unmapped_pages, .Lfunc_end4-memmanager_get_unmapped_pages
                                        # -- End function
	.globl	memmanager_map_page     # -- Begin function memmanager_map_page
	.type	memmanager_map_page,@function
memmanager_map_page:                    # @memmanager_map_page
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	movl	8(%ebp), %edi
	movl	16(%ebp), %ecx
	movl	12(%ebp), %edx
	movl	$-4194304, %ebx         # imm = 0xFFC00000
	movl	%edi, %esi
	shrl	$22, %esi
	testb	$1, -4096(,%esi,4)
	jne	.LBB5_2
# %bb.1:
	pushl	%esi
	calll	memmanager_create_new_page_table
	movl	16(%ebp), %ecx
	movl	12(%ebp), %edx
	addl	$4, %esp
.LBB5_2:
	shll	$12, %esi
	movl	%edx, %eax
	addl	%esi, %ebx
	movl	%edi, %esi
	andl	$-4096, %eax            # imm = 0xF000
	shrl	$10, %esi
	orl	%ecx, %eax
	andl	$4092, %esi             # imm = 0xFFC
	movl	%eax, (%esi,%ebx)
	pushl	%edx
	pushl	%edi
	pushl	$.L.str.2
	calll	printf
	addl	$12, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end5:
	.size	memmanager_map_page, .Lfunc_end5-memmanager_map_page
                                        # -- End function
	.globl	memmanager_virtual_alloc # -- Begin function memmanager_virtual_alloc
	.type	memmanager_virtual_alloc,@function
memmanager_virtual_alloc:               # @memmanager_virtual_alloc
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%eax
	movl	12(%ebp), %esi
	pushl	%esi
	pushl	$.L.str.3
	calll	printf
	addl	$8, %esp
	testl	%esi, %esi
	je	.LBB6_1
# %bb.2:
	movl	8(%ebp), %edx
	movl	num_memory_blocks, %ecx
	movl	%edx, -16(%ebp)         # 4-byte Spill
.LBB6_6:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB6_9 Depth 2
                                        #       Child Loop BB6_10 Depth 3
	xorl	%eax, %eax
	testl	%ecx, %ecx
	je	.LBB6_7
# %bb.8:                                #   in Loop: Header=BB6_6 Depth=1
	xorl	%ebx, %ebx
.LBB6_9:                                #   Parent Loop BB6_6 Depth=1
                                        # =>  This Loop Header: Depth=2
                                        #       Child Loop BB6_10 Depth 3
	negl	%esi
	movl	%esi, %edi
.LBB6_10:                               #   Parent Loop BB6_6 Depth=1
                                        #     Parent Loop BB6_9 Depth=2
                                        # =>    This Inner Loop Header: Depth=3
	movl	memory_map+4(,%ebx,8), %eax
	cmpl	$4096, %eax             # imm = 0x1000
	jb	.LBB6_3
# %bb.11:                               #   in Loop: Header=BB6_10 Depth=3
	movl	memory_map(,%ebx,8), %edx
	testl	$4095, %edx             # imm = 0xFFF
	je	.LBB6_12
# %bb.13:                               #   in Loop: Header=BB6_10 Depth=3
	leal	4095(%edx), %esi
	andl	$-4096, %esi            # imm = 0xF000
	movl	%esi, %ecx
	subl	%edx, %ecx
	leal	4096(%ecx), %edx
	cmpl	%edx, %eax
	jb	.LBB6_3
# %bb.14:                               #   in Loop: Header=BB6_10 Depth=3
	subl	%ecx, %eax
	movl	%esi, memory_map(,%ebx,8)
	movl	%eax, memory_map+4(,%ebx,8)
	jmp	.LBB6_15
.LBB6_12:                               #   in Loop: Header=BB6_10 Depth=3
	movl	%edx, %esi
.LBB6_15:                               #   in Loop: Header=BB6_10 Depth=3
	leal	4096(%esi), %ecx
	addl	$-4096, %eax            # imm = 0xF000
	movl	%ecx, memory_map(,%ebx,8)
	movl	%eax, memory_map+4(,%ebx,8)
	pushl	16(%ebp)
	pushl	%esi
	movl	-16(%ebp), %esi         # 4-byte Reload
	pushl	%esi
	calll	memmanager_map_page
	addl	$12, %esp
	addl	$4096, %esi             # imm = 0x1000
	incl	%edi
	xorl	%eax, %eax
	incl	%eax
	movl	%esi, -16(%ebp)         # 4-byte Spill
	cmpl	%eax, %edi
	jne	.LBB6_10
	jmp	.LBB6_16
.LBB6_3:                                #   in Loop: Header=BB6_9 Depth=2
	movl	num_memory_blocks, %ecx
	movl	%edi, %esi
	incl	%ebx
	negl	%esi
	cmpl	%ecx, %ebx
	jb	.LBB6_9
# %bb.4:                                #   in Loop: Header=BB6_6 Depth=1
	negl	%edi
	xorl	%eax, %eax
	movl	%edi, %esi
	jmp	.LBB6_5
.LBB6_7:                                #   in Loop: Header=BB6_6 Depth=1
	xorl	%ecx, %ecx
.LBB6_5:                                #   in Loop: Header=BB6_6 Depth=1
	testl	%esi, %esi
	jne	.LBB6_6
	jmp	.LBB6_17
.LBB6_16:
	movl	12(%ebp), %eax
	movl	%eax, %esi
	shll	$12, %eax
	pushl	%eax
	pushl	$0
	movl	8(%ebp), %eax
	movl	%eax, %edi
	pushl	%eax
	calll	memset
	addl	$12, %esp
	pushl	%edi
	pushl	%esi
	pushl	$.L.str.4
	calll	printf
	addl	$12, %esp
	movl	%edi, %eax
	jmp	.LBB6_17
.LBB6_1:
	xorl	%eax, %eax
.LBB6_17:
	addl	$4, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end6:
	.size	memmanager_virtual_alloc, .Lfunc_end6-memmanager_virtual_alloc
                                        # -- End function
	.globl	memmanager_allocate_pages # -- Begin function memmanager_allocate_pages
	.type	memmanager_allocate_pages,@function
memmanager_allocate_pages:              # @memmanager_allocate_pages
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	movl	12(%ebp), %edi
	movl	8(%ebp), %esi
	pushl	%edi
	pushl	%esi
	calll	memmanager_get_unmapped_pages
	addl	$8, %esp
	pushl	%edi
	pushl	%esi
	pushl	%eax
	calll	memmanager_virtual_alloc
	addl	$12, %esp
	popl	%esi
	popl	%edi
	popl	%ebp
	retl
.Lfunc_end7:
	.size	memmanager_allocate_pages, .Lfunc_end7-memmanager_allocate_pages
                                        # -- End function
	.globl	memmanager_free_pages   # -- Begin function memmanager_free_pages
	.type	memmanager_free_pages,@function
memmanager_free_pages:                  # @memmanager_free_pages
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$12, %esp
	movl	12(%ebp), %edi
	testl	%edi, %edi
	je	.LBB8_1
# %bb.3:
	movl	8(%ebp), %esi
	movl	$-4194304, %ecx         # imm = 0xFFC00000
.LBB8_4:                                # =>This Loop Header: Depth=1
                                        #     Child Loop BB8_11 Depth 2
	movl	%esi, %eax
	shrl	$22, %eax
	testb	$1, 4190208(%ecx,%eax,4)
	je	.LBB8_5
# %bb.6:                                #   in Loop: Header=BB8_4 Depth=1
	movl	%esi, %edx
	shll	$12, %eax
	shrl	$10, %edx
	addl	%ecx, %eax
	andl	$4092, %edx             # imm = 0xFFC
	movl	(%edx,%eax), %ebx
	xorl	%eax, %eax
	pushl	%eax
	pushl	%eax
	pushl	%esi
	calll	memmanager_map_page
	addl	$12, %esp
	testb	$1, %bl
	je	.LBB8_7
# %bb.8:                                #   in Loop: Header=BB8_4 Depth=1
	movl	num_memory_blocks, %eax
	decl	%edi
	movl	%esi, -20(%ebp)         # 4-byte Spill
	movl	%edi, -16(%ebp)         # 4-byte Spill
	testl	%eax, %eax
	je	.LBB8_13
# %bb.9:                                #   in Loop: Header=BB8_4 Depth=1
	andl	$-4096, %ebx            # imm = 0xF000
	leal	4096(%ebx), %ecx
	movl	%ecx, -24(%ebp)         # 4-byte Spill
	xorl	%ecx, %ecx
.LBB8_11:                               #   Parent Loop BB8_4 Depth=1
                                        # =>  This Inner Loop Header: Depth=2
	movl	memory_map(,%ecx,8), %edi
	movl	memory_map+4(,%ecx,8), %esi
	leal	(%esi,%edi), %edx
	cmpl	%ebx, %edx
	je	.LBB8_12
# %bb.14:                               #   in Loop: Header=BB8_11 Depth=2
	cmpl	-24(%ebp), %edi         # 4-byte Folded Reload
	je	.LBB8_15
# %bb.10:                               #   in Loop: Header=BB8_11 Depth=2
	incl	%ecx
	cmpl	%eax, %ecx
	jb	.LBB8_11
	jmp	.LBB8_13
.LBB8_15:                               #   in Loop: Header=BB8_4 Depth=1
	movl	%ebx, memory_map(,%ecx,8)
.LBB8_12:                               #   in Loop: Header=BB8_4 Depth=1
	addl	$4096, %esi             # imm = 0x1000
	movl	%esi, memory_map+4(,%ecx,8)
.LBB8_13:                               #   in Loop: Header=BB8_4 Depth=1
	movl	-20(%ebp), %esi         # 4-byte Reload
	movl	-16(%ebp), %edi         # 4-byte Reload
	movl	$-4194304, %ecx         # imm = 0xFFC00000
	addl	$4096, %esi             # imm = 0x1000
	testl	%edi, %edi
	jne	.LBB8_4
.LBB8_1:
	xorl	%eax, %eax
	jmp	.LBB8_2
.LBB8_5:
	xorl	%eax, %eax
	pushl	%eax
	pushl	%eax
	pushl	%esi
	calll	memmanager_map_page
	addl	$12, %esp
.LBB8_7:
	xorl	%eax, %eax
	decl	%eax
.LBB8_2:
	addl	$12, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end8:
	.size	memmanager_free_pages, .Lfunc_end8-memmanager_free_pages
                                        # -- End function
	.globl	memmanager_init_page_dir # -- Begin function memmanager_init_page_dir
	.type	memmanager_init_page_dir,@function
memmanager_init_page_dir:               # @memmanager_init_page_dir
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	movl	$-4092, %edx            # imm = 0xF004
	movl	12(%ebp), %eax
	movl	8(%ebp), %ecx
.LBB9_1:                                # =>This Inner Loop Header: Depth=1
	movl	$0, 4092(%ecx,%edx)
	addl	$4, %edx
	jne	.LBB9_1
# %bb.2:
	orl	$3, %eax
	movl	%eax, 4092(%ecx)
	popl	%ebp
	retl
.Lfunc_end9:
	.size	memmanager_init_page_dir, .Lfunc_end9-memmanager_init_page_dir
                                        # -- End function
	.globl	memmanager_new_memory_space # -- Begin function memmanager_new_memory_space
	.type	memmanager_new_memory_space,@function
memmanager_new_memory_space:            # @memmanager_new_memory_space
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	pushl	%eax
	xorl	%ebx, %ebx
	movl	$-4194304, %edi         # imm = 0xFFC00000
	movl	$3, %esi
	incl	%ebx
	pushl	%esi
	pushl	%ebx
	calll	memmanager_get_unmapped_pages
	addl	$8, %esp
	pushl	%esi
	pushl	%ebx
	pushl	%eax
	calll	memmanager_virtual_alloc
	addl	$12, %esp
	movl	%eax, %ebx
	movl	%eax, %esi
	shrl	$22, %ebx
	movl	-4096(,%ebx,4), %eax
	testb	$1, %al
	je	.LBB10_2
# %bb.1:
	movl	%ebx, %eax
	movl	%esi, %ecx
	shll	$12, %eax
	shrl	$10, %ecx
	addl	%edi, %eax
	andl	$4092, %ecx             # imm = 0xFFC
	movl	(%ecx,%eax), %eax
.LBB10_2:
	andl	$-4096, %eax            # imm = 0xF000
	movl	$-4092, %ecx            # imm = 0xF004
.LBB10_3:                               # =>This Inner Loop Header: Depth=1
	movl	$0, 4092(%esi,%ecx)
	addl	$4, %ecx
	jne	.LBB10_3
# %bb.4:
	movl	%esi, %ecx
	andl	$4095, %ecx             # imm = 0xFFF
	orl	%ecx, %eax
	movl	%ecx, -16(%ebp)         # 4-byte Spill
	orl	$3, %eax
	movl	%eax, 4092(%esi)
	pushl	$.L.str.5
	calll	printf
	addl	$4, %esp
	movl	$-4092, %eax            # imm = 0xF004
.LBB10_5:                               # =>This Inner Loop Header: Depth=1
	movl	4194300(%edi,%eax), %ecx
	andl	$-5, %ecx
	movl	%ecx, 4092(%esi,%eax)
	addl	$4, %eax
	jne	.LBB10_5
# %bb.6:
	pushl	$.L.str.6
	calll	printf
	addl	$4, %esp
	movl	4190208(%edi,%ebx,4), %eax
	testb	$1, %al
	je	.LBB10_8
# %bb.7:
	movl	%esi, %eax
	shll	$12, %ebx
	shrl	$10, %eax
	addl	%ebx, %edi
	andl	$4092, %eax             # imm = 0xFFC
	movl	(%eax,%edi), %eax
.LBB10_8:
	andl	$-4096, %eax            # imm = 0xF000
	addl	-16(%ebp), %eax         # 4-byte Folded Reload
	pushl	%eax
	calll	set_page_directory
	addl	$4, %esp
	pushl	$.L.str.7
	calll	printf
	addl	$4, %esp
	movl	%esi, %eax
	addl	$4, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end10:
	.size	memmanager_new_memory_space, .Lfunc_end10-memmanager_new_memory_space
                                        # -- End function
	.globl	memmanager_exit_memory_space # -- Begin function memmanager_exit_memory_space
	.type	memmanager_exit_memory_space,@function
memmanager_exit_memory_space:           # @memmanager_exit_memory_space
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	movl	8(%ebp), %esi
	pushl	kernel_page_directory
	calll	set_page_directory
	addl	$4, %esp
	pushl	$1
	pushl	%esi
	calll	memmanager_free_pages
	addl	$8, %esp
	xorl	%eax, %eax
	incl	%eax
	popl	%esi
	popl	%ebp
	retl
.Lfunc_end11:
	.size	memmanager_exit_memory_space, .Lfunc_end11-memmanager_exit_memory_space
                                        # -- End function
	.globl	load_exe                # -- Begin function load_exe
	.type	load_exe,@function
load_exe:                               # @load_exe
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%edi
	pushl	%esi
	subl	$12, %esp
	movl	8(%ebp), %ebx
	pushl	%ebx
	calll	filesystem_open_handle
	addl	$4, %esp
	movl	$4095, %esi             # imm = 0xFFF
	movl	%eax, %edi
	addl	20(%ebx), %esi
	movl	$3, %ebx
	shrl	$12, %esi
	pushl	%ebx
	pushl	%esi
	calll	memmanager_get_unmapped_pages
	addl	$8, %esp
	pushl	%ebx
	pushl	%esi
	pushl	%eax
	calll	memmanager_virtual_alloc
	addl	$12, %esp
	movl	%eax, %esi
	pushl	%edi
	movl	8(%ebp), %eax
	pushl	20(%eax)
	pushl	%esi
	calll	filesystem_read_file
	addl	$12, %esp
	pushl	%edi
	calll	filesystem_close_file
	addl	$4, %esp
	movl	%esi, %eax
	shrl	$22, %eax
	movl	-4096(,%eax,4), %ecx
	testb	$1, %cl
	je	.LBB12_2
# %bb.1:
	shll	$12, %eax
	movl	$-4194304, %ecx         # imm = 0xFFC00000
	addl	%ecx, %eax
	movl	%esi, %ecx
	shrl	$10, %ecx
	andl	$4092, %ecx             # imm = 0xFFC
	movl	(%ecx,%eax), %ecx
.LBB12_2:
	movl	%esi, %edi
	andl	$-4096, %ecx            # imm = 0xF000
	movl	%esi, -24(%ebp)         # 4-byte Spill
	movl	$3, %ebx
	andl	$4095, %edi             # imm = 0xFFF
	xorl	%esi, %esi
	movl	%ecx, -20(%ebp)         # 4-byte Spill
	incl	%esi
	pushl	%ebx
	pushl	%esi
	calll	memmanager_get_unmapped_pages
	addl	$8, %esp
	pushl	%ebx
	pushl	%esi
	pushl	%eax
	calll	memmanager_virtual_alloc
	addl	$12, %esp
	movl	$-4096, %ecx            # imm = 0xF000
	movl	$-4194304, %edx         # imm = 0xFFC00000
	movl	$-4194304, %esi         # imm = 0xFFC00000
	andl	-4, %ecx
	movl	%ecx, -16(%ebp)         # 4-byte Spill
	movl	%eax, %ecx
	shrl	$22, %ecx
	movl	4190208(%edx,%ecx,4), %ebx
	testb	$1, %bl
	je	.LBB12_4
# %bb.3:
	movl	%ecx, %edx
	movl	%eax, %ebx
	shll	$12, %edx
	shrl	$10, %ebx
	addl	%esi, %edx
	andl	$4092, %ebx             # imm = 0xFFC
	movl	(%ebx,%edx), %ebx
.LBB12_4:
	movl	-20(%ebp), %edx         # 4-byte Reload
	andl	$-4096, %ebx            # imm = 0xF000
	orl	%edi, %edx
	movl	%edx, %edi
	movl	$-4092, %edx            # imm = 0xF004
.LBB12_5:                               # =>This Inner Loop Header: Depth=1
	movl	$0, 4092(%eax,%edx)
	addl	$4, %edx
	jne	.LBB12_5
# %bb.6:
	movl	%eax, %edx
	andl	$4095, %edx             # imm = 0xFFF
	orl	%edx, %ebx
	orl	$3, %ebx
	movl	%ebx, 4092(%eax)
	movl	-4096, %esi
	andl	$-5, %esi
	movl	%esi, (%eax)
	movl	$-4194304, %esi         # imm = 0xFFC00000
	movl	4190208(%esi,%ecx,4), %ebx
	testb	$1, %bl
	je	.LBB12_8
# %bb.7:
	shrl	$10, %eax
	shll	$12, %ecx
	addl	%ecx, %esi
	andl	$4092, %eax             # imm = 0xFFC
	movl	(%eax,%esi), %ebx
.LBB12_8:
	andl	$-4096, %ebx            # imm = 0xF000
	orl	%edx, %ebx
	pushl	%ebx
	calll	set_page_directory
	addl	$4, %esp
	movl	$7, %esi
	pushl	%esi
	pushl	%edi
	pushl	$327680                 # imm = 0x50000
	calll	memmanager_map_page
	addl	$12, %esp
	movl	$753664, %eax           # imm = 0xB8000
	pushl	%esi
	pushl	%eax
	pushl	%eax
	calll	memmanager_map_page
	addl	$12, %esp
	calll	327680
	movl	%eax, %edi
	pushl	%eax
	pushl	$.L.str.8
	calll	printf
	addl	$8, %esp
	pushl	-16(%ebp)               # 4-byte Folded Reload
	calll	set_page_directory
	addl	$4, %esp
	pushl	$1
	pushl	-24(%ebp)               # 4-byte Folded Reload
	calll	memmanager_free_pages
	addl	$8, %esp
	movl	%edi, %eax
	addl	$12, %esp
	popl	%esi
	popl	%edi
	popl	%ebx
	popl	%ebp
	retl
.Lfunc_end12:
	.size	load_exe, .Lfunc_end12-load_exe
                                        # -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4               # -- Begin function memmanager_init
.LCPI13_0:
	.long	0                       # 0x0
	.long	1                       # 0x1
	.long	2                       # 0x2
	.long	3                       # 0x3
.LCPI13_1:
	.long	3                       # 0x3
	.long	3                       # 0x3
	.long	3                       # 0x3
	.long	3                       # 0x3
.LCPI13_2:
	.long	4                       # 0x4
	.long	4                       # 0x4
	.long	4                       # 0x4
	.long	4                       # 0x4
	.text
	.globl	memmanager_init
	.type	memmanager_init,@function
memmanager_init:                        # @memmanager_init
# %bb.0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	$.L.str.9
	calll	puts
	addl	$4, %esp
	movl	_multiboot, %eax
	movl	8(%eax), %eax
	shll	$10, %eax
	movl	%eax, memory_map+4
	calll	memmanager_allocate_physical_page
	movl	%eax, kernel_page_directory
	calll	memmanager_allocate_physical_page
	movl	%eax, %esi
	movl	kernel_page_directory, %eax
	movl	$-4092, %ecx            # imm = 0xF004
.LBB13_1:                               # =>This Inner Loop Header: Depth=1
	movl	$0, 4092(%eax,%ecx)
	addl	$4, %ecx
	jne	.LBB13_1
# %bb.2:
	movl	%eax, %ecx
	orl	$3, %ecx
	movl	%ecx, 4092(%eax)
	pushl	$.L.str.10
	calll	puts
	addl	$4, %esp
	pushl	%esi
	pushl	$.L.str.11
	calll	printf
	addl	$8, %esp
	movdqa	.LCPI13_0, %xmm0        # xmm0 = [0,1,2,3]
	movdqa	.LCPI13_1, %xmm1        # xmm1 = [3,3,3,3]
	movdqa	.LCPI13_2, %xmm2        # xmm2 = [4,4,4,4]
	movl	$-4096, %eax            # imm = 0xF000
.LBB13_3:                               # =>This Inner Loop Header: Depth=1
	movdqa	%xmm0, %xmm3
	paddd	%xmm2, %xmm0
	pslld	$12, %xmm3
	por	%xmm1, %xmm3
	movdqu	%xmm3, 4096(%esi,%eax)
	addl	$16, %eax
	jne	.LBB13_3
# %bb.4:
	pushl	$.L.str.12
	calll	puts
	addl	$4, %esp
	movl	kernel_page_directory, %eax
	orl	$3, %esi
	movl	%esi, (%eax)
	pushl	%eax
	calll	set_page_directory
	addl	$4, %esp
	calll	enable_paging
	pushl	$.L.str.13
	calll	printf
	addl	$4, %esp
	popl	%esi
	popl	%ebp
	retl
.Lfunc_end13:
	.size	memmanager_init, .Lfunc_end13-memmanager_init
                                        # -- End function
	.type	last_pde_address,@object # @last_pde_address
	.section	.rodata,"a",@progbits
	.globl	last_pde_address
	.p2align	2
last_pde_address:
	.long	4290772992
	.size	last_pde_address, 4

	.type	current_page_directory,@object # @current_page_directory
	.globl	current_page_directory
	.p2align	2
current_page_directory:
	.long	4294963200
	.size	current_page_directory, 4

	.type	num_memory_blocks,@object # @num_memory_blocks
	.data
	.globl	num_memory_blocks
	.p2align	2
num_memory_blocks:
	.long	2                       # 0x2
	.size	num_memory_blocks, 4

	.type	memory_map,@object      # @memory_map
	.globl	memory_map
	.p2align	2
memory_map:
	.long	1048576                 # 0x100000
	.long	0                       # 0x0
	.zero	8
	.size	memory_map, 16

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"could not allocate enough pages\n"
	.size	.L.str, 33

	.type	.L.str.1,@object        # @.str.1
.L.str.1:
	.asciz	"added new page table, %X\n"
	.size	.L.str.1, 26

	.type	.L.str.2,@object        # @.str.2
.L.str.2:
	.asciz	"%X mapped to physical address %X\n"
	.size	.L.str.2, 34

	.type	.L.str.3,@object        # @.str.3
.L.str.3:
	.asciz	"Attempting to allocate %d pages\n"
	.size	.L.str.3, 33

	.type	.L.str.4,@object        # @.str.4
.L.str.4:
	.asciz	"Successfully allocated %d pages at %X\n"
	.size	.L.str.4, 39

	.type	.L.str.5,@object        # @.str.5
.L.str.5:
	.asciz	"new dir initialized\n"
	.size	.L.str.5, 21

	.type	.L.str.6,@object        # @.str.6
.L.str.6:
	.asciz	"new table initialized\n"
	.size	.L.str.6, 23

	.type	.L.str.7,@object        # @.str.7
.L.str.7:
	.asciz	"page dir set\n"
	.size	.L.str.7, 14

	.type	kernel_page_directory,@object # @kernel_page_directory
	.comm	kernel_page_directory,4,4
	.type	.L.str.8,@object        # @.str.8
.L.str.8:
	.asciz	"Process returned %d\n"
	.size	.L.str.8, 21

	.type	.L.str.9,@object        # @.str.9
.L.str.9:
	.asciz	"memmanager init"
	.size	.L.str.9, 16

	.type	.L.str.10,@object       # @.str.10
.L.str.10:
	.asciz	"page dir init"
	.size	.L.str.10, 14

	.type	.L.str.11,@object       # @.str.11
.L.str.11:
	.asciz	"%X\n"
	.size	.L.str.11, 4

	.type	.L.str.12,@object       # @.str.12
.L.str.12:
	.asciz	"page table init"
	.size	.L.str.12, 16

	.type	.L.str.13,@object       # @.str.13
.L.str.13:
	.asciz	"paging enabled\n"
	.size	.L.str.13, 16


	.ident	"clang version 7.0.0 (tags/RELEASE_700/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
