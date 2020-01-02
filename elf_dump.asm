
test.elf:	file format ELF32-i386


Disassembly of section .text:

08000000 _start:
 8000000: 55                           	push	ebp
 8000001: 89 e5                        	mov	ebp, esp
 8000003: 53                           	push	ebx
 8000004: 57                           	push	edi
 8000005: 56                           	push	esi
 8000006: 83 ec 08                     	sub	esp, 8
 8000009: 68 22 10 00 08               	push	134221858
 800000e: e8 1b 04 00 00               	call	1051 <puts>
 8000013: 83 c4 04                     	add	esp, 4
 8000016: e8 4f 05 00 00               	call	1359 <getSS>
 800001b: 0f b7 c0                     	movzx	eax, ax
 800001e: 50                           	push	eax
 800001f: 68 00 10 00 08               	push	134221824
 8000024: e8 ba 04 00 00               	call	1210 <printf>
 8000029: 83 c4 08                     	add	esp, 8
 800002c: 31 db                        	xor	ebx, ebx
 800002e: 43                           	inc	ebx
 800002f: 31 f6                        	xor	esi, esi
 8000031: b8 07 10 00 08               	mov	eax, 134221831
 8000036: 31 c9                        	xor	ecx, ecx
 8000038: cd 80                        	int	128
 800003a: 85 c0                        	test	eax, eax
 800003c: 74 35                        	je	53 <_start+0x73>
 800003e: 89 c7                        	mov	edi, eax
 8000040: 8d 75 ef                     	lea	esi, [ebp - 17]
 8000043: bb 03 00 00 00               	mov	ebx, 3
 8000048: 89 f0                        	mov	eax, esi
 800004a: b9 04 00 00 00               	mov	ecx, 4
 800004f: 89 fa                        	mov	edx, edi
 8000051: cd 80                        	int	128
 8000053: bb 02 00 00 00               	mov	ebx, 2
 8000058: 89 f8                        	mov	eax, edi
 800005a: cd 80                        	int	128
 800005c: c6 46 04 00                  	mov	byte ptr [esi + 4], 0
 8000060: be 2a 00 00 00               	mov	esi, 42
 8000065: bb 04 00 00 00               	mov	ebx, 4
 800006a: b8 2a 00 00 00               	mov	eax, 42
 800006f: cd 80                        	int	128
 8000071: eb 0d                        	jmp	13 <_start+0x80>
 8000073: 68 10 10 00 08               	push	134221840
 8000078: e8 b1 03 00 00               	call	945 <puts>
 800007d: 83 c4 04                     	add	esp, 4
 8000080: 89 f0                        	mov	eax, esi
 8000082: 83 c4 08                     	add	esp, 8
 8000085: 5e                           	pop	esi
 8000086: 5f                           	pop	edi
 8000087: 5b                           	pop	ebx
 8000088: 5d                           	pop	ebp
 8000089: c3                           	ret
 800008a: cc                           	int3
 800008b: cc                           	int3

0800008c fopen:
 800008c: 55                           	push	ebp
 800008d: 89 e5                        	mov	ebp, esp
 800008f: 31 c0                        	xor	eax, eax
 8000091: 5d                           	pop	ebp
 8000092: c3                           	ret

08000093 clearerr:
 8000093: 55                           	push	ebp
 8000094: 89 e5                        	mov	ebp, esp
 8000096: 5d                           	pop	ebp
 8000097: c3                           	ret

08000098 feof:
 8000098: 55                           	push	ebp
 8000099: 89 e5                        	mov	ebp, esp
 800009b: 31 c0                        	xor	eax, eax
 800009d: 5d                           	pop	ebp
 800009e: c3                           	ret

0800009f ferror:
 800009f: 55                           	push	ebp
 80000a0: 89 e5                        	mov	ebp, esp
 80000a2: 31 c0                        	xor	eax, eax
 80000a4: 5d                           	pop	ebp
 80000a5: c3                           	ret

080000a6 gets_s:
 80000a6: 55                           	push	ebp
 80000a7: 89 e5                        	mov	ebp, esp
 80000a9: 56                           	push	esi
 80000aa: 8b 55 0c                     	mov	edx, dword ptr [ebp + 12]
 80000ad: 8b 45 08                     	mov	eax, dword ptr [ebp + 8]
 80000b0: 8d 4c 10 ff                  	lea	ecx, [eax + edx - 1]
 80000b4: 39 c1                        	cmp	ecx, eax
 80000b6: 76 10                        	jbe	16 <gets_s+0x22>
 80000b8: 4a                           	dec	edx
 80000b9: 89 c6                        	mov	esi, eax
 80000bb: 80 3e 0a                     	cmp	byte ptr [esi], 10
 80000be: 74 06                        	je	6 <gets_s+0x20>
 80000c0: 46                           	inc	esi
 80000c1: 4a                           	dec	edx
 80000c2: 75 f7                        	jne	-9 <gets_s+0x15>
 80000c4: eb 02                        	jmp	2 <gets_s+0x22>
 80000c6: 89 f1                        	mov	ecx, esi
 80000c8: c6 01 00                     	mov	byte ptr [ecx], 0
 80000cb: 5e                           	pop	esi
 80000cc: 5d                           	pop	ebp
 80000cd: c3                           	ret

080000ce gets:
 80000ce: 55                           	push	ebp
 80000cf: 89 e5                        	mov	ebp, esp
 80000d1: 8b 45 08                     	mov	eax, dword ptr [ebp + 8]
 80000d4: 5d                           	pop	ebp
 80000d5: c3                           	ret

080000d6 fputc:
 80000d6: 55                           	push	ebp
 80000d7: 89 e5                        	mov	ebp, esp
 80000d9: 8b 45 08                     	mov	eax, dword ptr [ebp + 8]
 80000dc: 5d                           	pop	ebp
 80000dd: c3                           	ret

080000de putchar:
 80000de: 55                           	push	ebp
 80000df: 89 e5                        	mov	ebp, esp
 80000e1: 53                           	push	ebx
 80000e2: 56                           	push	esi
 80000e3: 50                           	push	eax
 80000e4: 8b 4d 08                     	mov	ecx, dword ptr [ebp + 8]
 80000e7: 8d 45 f7                     	lea	eax, [ebp - 9]
 80000ea: 88 08                        	mov	byte ptr [eax], cl
 80000ec: 89 ce                        	mov	esi, ecx
 80000ee: 31 c9                        	xor	ecx, ecx
 80000f0: 41                           	inc	ecx
 80000f1: 31 db                        	xor	ebx, ebx
 80000f3: cd 80                        	int	128
 80000f5: 89 f0                        	mov	eax, esi
 80000f7: 83 c4 04                     	add	esp, 4
 80000fa: 5e                           	pop	esi
 80000fb: 5b                           	pop	ebx
 80000fc: 5d                           	pop	ebp
 80000fd: c3                           	ret

080000fe getc:
 80000fe: 55                           	push	ebp
 80000ff: 89 e5                        	mov	ebp, esp
 8000101: 53                           	push	ebx
 8000102: 8b 0d 00 20 00 08            	mov	ecx, dword ptr [134225920]
 8000108: 31 c0                        	xor	eax, eax
 800010a: 3b 4d 08                     	cmp	ecx, dword ptr [ebp + 8]
 800010d: 75 0a                        	jne	10 <getc+0x1b>
 800010f: bb 0c 00 00 00               	mov	ebx, 12
 8000114: cd 80                        	int	128
 8000116: 0f b6 c0                     	movzx	eax, al
 8000119: 5b                           	pop	ebx
 800011a: 5d                           	pop	ebp
 800011b: c3                           	ret

0800011c getchar:
 800011c: 55                           	push	ebp
 800011d: 89 e5                        	mov	ebp, esp
 800011f: 53                           	push	ebx
 8000120: bb 0c 00 00 00               	mov	ebx, 12
 8000125: cd 80                        	int	128
 8000127: 0f b6 c0                     	movzx	eax, al
 800012a: 5b                           	pop	ebx
 800012b: 5d                           	pop	ebp
 800012c: c3                           	ret

0800012d vsnprintf:
 800012d: 55                           	push	ebp
 800012e: 89 e5                        	mov	ebp, esp
 8000130: 53                           	push	ebx
 8000131: 57                           	push	edi
 8000132: 56                           	push	esi
 8000133: 83 ec 30                     	sub	esp, 48
 8000136: 8b 75 10                     	mov	esi, dword ptr [ebp + 16]
 8000139: 8b 45 08                     	mov	eax, dword ptr [ebp + 8]
 800013c: 8a 0e                        	mov	cl, byte ptr [esi]
 800013e: 84 c9                        	test	cl, cl
 8000140: 0f 84 cd 02 00 00            	je	717 <vsnprintf+0x2e6>
 8000146: 8b 55 14                     	mov	edx, dword ptr [ebp + 20]
 8000149: 89 55 e0                     	mov	dword ptr [ebp - 32], edx
 800014c: 8b 7d 0c                     	mov	edi, dword ptr [ebp + 12]
 800014f: 4f                           	dec	edi
 8000150: 31 db                        	xor	ebx, ebx
 8000152: 89 45 ec                     	mov	dword ptr [ebp - 20], eax
 8000155: c7 45 e8 00 00 00 00         	mov	dword ptr [ebp - 24], 0
 800015c: 31 c0                        	xor	eax, eax
 800015e: 31 d2                        	xor	edx, edx
 8000160: 89 7d d4                     	mov	dword ptr [ebp - 44], edi
 8000163: 83 fa 02                     	cmp	edx, 2
 8000166: 74 5f                        	je	95 <vsnprintf+0x9a>
 8000168: 83 fa 01                     	cmp	edx, 1
 800016b: 74 0d                        	je	13 <vsnprintf+0x4d>
 800016d: 85 d2                        	test	edx, edx
 800016f: 75 2d                        	jne	45 <vsnprintf+0x71>
 8000171: 80 f9 25                     	cmp	cl, 37
 8000174: 75 2f                        	jne	47 <vsnprintf+0x78>
 8000176: 8a 4e 01                     	mov	cl, byte ptr [esi + 1]
 8000179: 46                           	inc	esi
 800017a: 8b 55 e8                     	mov	edx, dword ptr [ebp - 24]
 800017d: 80 f9 30                     	cmp	cl, 48
 8000180: 74 3e                        	je	62 <vsnprintf+0x93>
 8000182: 80 f9 2d                     	cmp	cl, 45
 8000185: 74 72                        	je	114 <vsnprintf+0xcc>
 8000187: 80 f9 25                     	cmp	cl, 37
 800018a: 75 3b                        	jne	59 <vsnprintf+0x9a>
 800018c: 39 fa                        	cmp	edx, edi
 800018e: 73 0a                        	jae	10 <vsnprintf+0x6d>
 8000190: 8b 45 ec                     	mov	eax, dword ptr [ebp - 20]
 8000193: c6 00 25                     	mov	byte ptr [eax], 37
 8000196: 40                           	inc	eax
 8000197: 89 45 ec                     	mov	dword ptr [ebp - 20], eax
 800019a: 42                           	inc	edx
 800019b: 89 55 e8                     	mov	dword ptr [ebp - 24], edx
 800019e: 31 d2                        	xor	edx, edx
 80001a0: e9 48 02 00 00               	jmp	584 <vsnprintf+0x2c0>
 80001a5: 8b 55 e8                     	mov	edx, dword ptr [ebp - 24]
 80001a8: 39 fa                        	cmp	edx, edi
 80001aa: 73 09                        	jae	9 <vsnprintf+0x88>
 80001ac: 8b 7d ec                     	mov	edi, dword ptr [ebp - 20]
 80001af: 88 0f                        	mov	byte ptr [edi], cl
 80001b1: 47                           	inc	edi
 80001b2: 89 7d ec                     	mov	dword ptr [ebp - 20], edi
 80001b5: 42                           	inc	edx
 80001b6: 89 55 e8                     	mov	dword ptr [ebp - 24], edx
 80001b9: 31 d2                        	xor	edx, edx
 80001bb: e9 31 02 00 00               	jmp	561 <vsnprintf+0x2c4>
 80001c0: 83 c8 40                     	or	eax, 64
 80001c3: 8a 4e 01                     	mov	cl, byte ptr [esi + 1]
 80001c6: 46                           	inc	esi
 80001c7: 0f be d1                     	movsx	edx, cl
 80001ca: 80 c1 d0                     	add	cl, -48
 80001cd: 80 f9 09                     	cmp	cl, 9
 80001d0: 77 11                        	ja	17 <vsnprintf+0xb6>
 80001d2: 8d 0c 9b                     	lea	ecx, [ebx + 4*ebx]
 80001d5: 8d 5c 4a d0                  	lea	ebx, [edx + 2*ecx - 48]
 80001d9: ba 02 00 00 00               	mov	edx, 2
 80001de: e9 0e 02 00 00               	jmp	526 <vsnprintf+0x2c4>
 80001e3: 83 fa 6c                     	cmp	edx, 108
 80001e6: 74 20                        	je	32 <vsnprintf+0xdb>
 80001e8: 83 fa 68                     	cmp	edx, 104
 80001eb: 74 20                        	je	32 <vsnprintf+0xe0>
 80001ed: 83 fa 46                     	cmp	edx, 70
 80001f0: 75 1e                        	jne	30 <vsnprintf+0xe3>
 80001f2: 0d 80 00 00 00               	or	eax, 128
 80001f7: eb 17                        	jmp	23 <vsnprintf+0xe3>
 80001f9: 89 c2                        	mov	edx, eax
 80001fb: 31 c9                        	xor	ecx, ecx
 80001fd: 41                           	inc	ecx
 80001fe: 21 ca                        	and	edx, ecx
 8000200: 74 40                        	je	64 <vsnprintf+0x115>
 8000202: 31 c0                        	xor	eax, eax
 8000204: 31 db                        	xor	ebx, ebx
 8000206: eb 3c                        	jmp	60 <vsnprintf+0x117>
 8000208: 83 c8 08                     	or	eax, 8
 800020b: eb 03                        	jmp	3 <vsnprintf+0xe3>
 800020d: 83 c8 10                     	or	eax, 16
 8000210: 89 45 f0                     	mov	dword ptr [ebp - 16], eax
 8000213: c6 45 d3 00                  	mov	byte ptr [ebp - 45], 0
 8000217: 0f be 06                     	movsx	eax, byte ptr [esi]
 800021a: bf 10 00 00 00               	mov	edi, 16
 800021f: 8d 48 9d                     	lea	ecx, [eax - 99]
 8000222: 83 f9 15                     	cmp	ecx, 21
 8000225: 89 5d e4                     	mov	dword ptr [ebp - 28], ebx
 8000228: 77 21                        	ja	33 <vsnprintf+0x11e>
 800022a: 31 d2                        	xor	edx, edx
 800022c: 31 c0                        	xor	eax, eax
 800022e: 31 db                        	xor	ebx, ebx
 8000230: ff 24 8d 2c 10 00 08         	jmp	dword ptr [4*ecx + 134221868]
 8000237: 83 4d f0 04                  	or	dword ptr [ebp - 16], 4
 800023b: bf 0a 00 00 00               	mov	edi, 10
 8000240: eb 4c                        	jmp	76 <vsnprintf+0x161>
 8000242: 09 c8                        	or	eax, ecx
 8000244: 31 ca                        	xor	edx, ecx
 8000246: e9 a6 01 00 00               	jmp	422 <vsnprintf+0x2c4>
 800024b: 83 f8 58                     	cmp	eax, 88
 800024e: 0f 85 4a ff ff ff            	jne	-182 <vsnprintf+0x71>
 8000254: 83 4d f0 02                  	or	dword ptr [ebp - 16], 2
 8000258: eb 34                        	jmp	52 <vsnprintf+0x161>
 800025a: 89 75 d8                     	mov	dword ptr [ebp - 40], esi
 800025d: 8b 55 f0                     	mov	edx, dword ptr [ebp - 16]
 8000260: 83 e2 bf                     	and	edx, -65
 8000263: 8b 4d e0                     	mov	ecx, dword ptr [ebp - 32]
 8000266: 8a 01                        	mov	al, byte ptr [ecx]
 8000268: 83 c1 04                     	add	ecx, 4
 800026b: 89 4d e0                     	mov	dword ptr [ebp - 32], ecx
 800026e: 88 45 d2                     	mov	byte ptr [ebp - 46], al
 8000271: 89 d0                        	mov	eax, edx
 8000273: 31 ff                        	xor	edi, edi
 8000275: 47                           	inc	edi
 8000276: 89 f9                        	mov	ecx, edi
 8000278: 89 7d dc                     	mov	dword ptr [ebp - 36], edi
 800027b: 8d 75 d2                     	lea	esi, [ebp - 46]
 800027e: 8b 5d d4                     	mov	ebx, dword ptr [ebp - 44]
 8000281: 8b 55 e8                     	mov	edx, dword ptr [ebp - 24]
 8000284: e9 97 00 00 00               	jmp	151 <vsnprintf+0x1f3>
 8000289: bf 08 00 00 00               	mov	edi, 8
 800028e: 8b 45 e0                     	mov	eax, dword ptr [ebp - 32]
 8000291: 8b 08                        	mov	ecx, dword ptr [eax]
 8000293: 8b 5d f0                     	mov	ebx, dword ptr [ebp - 16]
 8000296: 89 d8                        	mov	eax, ebx
 8000298: c0 e8 02                     	shr	al, 2
 800029b: 85 c9                        	test	ecx, ecx
 800029d: 0f 98 c2                     	sets	dl
 80002a0: 84 d0                        	test	al, dl
 80002a2: 89 75 d8                     	mov	dword ptr [ebp - 40], esi
 80002a5: 74 05                        	je	5 <vsnprintf+0x17f>
 80002a7: 83 cb 20                     	or	ebx, 32
 80002aa: f7 d9                        	neg	ecx
 80002ac: 89 5d f0                     	mov	dword ptr [ebp - 16], ebx
 80002af: f6 c3 02                     	test	bl, 2
 80002b2: 0f 94 c7                     	sete	bh
 80002b5: c0 e7 05                     	shl	bh, 5
 80002b8: 80 c7 37                     	add	bh, 55
 80002bb: 8d 75 d3                     	lea	esi, [ebp - 45]
 80002be: 89 c8                        	mov	eax, ecx
 80002c0: 31 d2                        	xor	edx, edx
 80002c2: f7 f7                        	div	edi
 80002c4: 83 fa 0a                     	cmp	edx, 10
 80002c7: b3 30                        	mov	bl, 48
 80002c9: 72 02                        	jb	2 <vsnprintf+0x1a0>
 80002cb: 88 fb                        	mov	bl, bh
 80002cd: 00 d3                        	add	bl, dl
 80002cf: 88 5e ff                     	mov	byte ptr [esi - 1], bl
 80002d2: 4e                           	dec	esi
 80002d3: 39 cf                        	cmp	edi, ecx
 80002d5: 89 c1                        	mov	ecx, eax
 80002d7: 76 e5                        	jbe	-27 <vsnprintf+0x191>
 80002d9: 83 45 e0 04                  	add	dword ptr [ebp - 32], 4
 80002dd: 56                           	push	esi
 80002de: e8 ad 03 00 00               	call	941 <strlen>
 80002e3: 83 c4 04                     	add	esp, 4
 80002e6: 8b 4d f0                     	mov	ecx, dword ptr [ebp - 16]
 80002e9: 89 ca                        	mov	edx, ecx
 80002eb: c1 ea 05                     	shr	edx, 5
 80002ee: 83 e2 01                     	and	edx, 1
 80002f1: 01 c2                        	add	edx, eax
 80002f3: 89 55 dc                     	mov	dword ptr [ebp - 36], edx
 80002f6: 89 c8                        	mov	eax, ecx
 80002f8: ba 60 00 00 00               	mov	edx, 96
 80002fd: 21 d0                        	and	eax, edx
 80002ff: 39 d0                        	cmp	eax, edx
 8000301: 8b 5d d4                     	mov	ebx, dword ptr [ebp - 44]
 8000304: 75 1f                        	jne	31 <vsnprintf+0x1f8>
 8000306: 8b 55 e8                     	mov	edx, dword ptr [ebp - 24]
 8000309: 39 da                        	cmp	edx, ebx
 800030b: 73 0a                        	jae	10 <vsnprintf+0x1ea>
 800030d: 8b 45 ec                     	mov	eax, dword ptr [ebp - 20]
 8000310: c6 00 2d                     	mov	byte ptr [eax], 45
 8000313: 40                           	inc	eax
 8000314: 89 45 ec                     	mov	dword ptr [ebp - 20], eax
 8000317: 31 ff                        	xor	edi, edi
 8000319: 47                           	inc	edi
 800031a: 89 f9                        	mov	ecx, edi
 800031c: 8b 45 f0                     	mov	eax, dword ptr [ebp - 16]
 800031f: 42                           	inc	edx
 8000320: 8b 7d ec                     	mov	edi, dword ptr [ebp - 20]
 8000323: eb 0e                        	jmp	14 <vsnprintf+0x206>
 8000325: 31 ff                        	xor	edi, edi
 8000327: 47                           	inc	edi
 8000328: 89 f9                        	mov	ecx, edi
 800032a: 8b 55 e8                     	mov	edx, dword ptr [ebp - 24]
 800032d: 8b 7d ec                     	mov	edi, dword ptr [ebp - 20]
 8000330: 8b 45 f0                     	mov	eax, dword ptr [ebp - 16]
 8000333: a8 01                        	test	al, 1
 8000335: 75 53                        	jne	83 <vsnprintf+0x25d>
 8000337: 89 45 f0                     	mov	dword ptr [ebp - 16], eax
 800033a: 8b 45 e4                     	mov	eax, dword ptr [ebp - 28]
 800033d: 3b 45 dc                     	cmp	eax, dword ptr [ebp - 36]
 8000340: 76 45                        	jbe	69 <vsnprintf+0x25a>
 8000342: 89 f9                        	mov	ecx, edi
 8000344: 8b 45 f0                     	mov	eax, dword ptr [ebp - 16]
 8000347: 0f b6 c0                     	movzx	eax, al
 800034a: c1 e8 06                     	shr	eax, 6
 800034d: c0 e0 04                     	shl	al, 4
 8000350: 0c 20                        	or	al, 32
 8000352: 8b 7d e4                     	mov	edi, dword ptr [ebp - 28]
 8000355: 01 d7                        	add	edi, edx
 8000357: 89 7d ec                     	mov	dword ptr [ebp - 20], edi
 800035a: 89 d7                        	mov	edi, edx
 800035c: 39 da                        	cmp	edx, ebx
 800035e: 73 03                        	jae	3 <vsnprintf+0x236>
 8000360: 88 01                        	mov	byte ptr [ecx], al
 8000362: 41                           	inc	ecx
 8000363: 89 fa                        	mov	edx, edi
 8000365: 42                           	inc	edx
 8000366: 8b 7d e4                     	mov	edi, dword ptr [ebp - 28]
 8000369: 4f                           	dec	edi
 800036a: 89 7d e4                     	mov	dword ptr [ebp - 28], edi
 800036d: 3b 7d dc                     	cmp	edi, dword ptr [ebp - 36]
 8000370: 77 e8                        	ja	-24 <vsnprintf+0x22d>
 8000372: 8b 45 dc                     	mov	eax, dword ptr [ebp - 36]
 8000375: 8b 55 ec                     	mov	edx, dword ptr [ebp - 20]
 8000378: 29 c2                        	sub	edx, eax
 800037a: 89 45 e4                     	mov	dword ptr [ebp - 28], eax
 800037d: 89 cf                        	mov	edi, ecx
 800037f: 8b 45 f0                     	mov	eax, dword ptr [ebp - 16]
 8000382: 31 c9                        	xor	ecx, ecx
 8000384: 41                           	inc	ecx
 8000385: eb 03                        	jmp	3 <vsnprintf+0x25d>
 8000387: 8b 45 f0                     	mov	eax, dword ptr [ebp - 16]
 800038a: 83 e0 60                     	and	eax, 96
 800038d: 83 f8 20                     	cmp	eax, 32
 8000390: 75 09                        	jne	9 <vsnprintf+0x26e>
 8000392: 39 da                        	cmp	edx, ebx
 8000394: 73 04                        	jae	4 <vsnprintf+0x26d>
 8000396: c6 07 2d                     	mov	byte ptr [edi], 45
 8000399: 47                           	inc	edi
 800039a: 42                           	inc	edx
 800039b: 8a 06                        	mov	al, byte ptr [esi]
 800039d: 84 c0                        	test	al, al
 800039f: 74 10                        	je	16 <vsnprintf+0x284>
 80003a1: 39 da                        	cmp	edx, ebx
 80003a3: 73 09                        	jae	9 <vsnprintf+0x281>
 80003a5: 88 07                        	mov	byte ptr [edi], al
 80003a7: 01 cf                        	add	edi, ecx
 80003a9: 8a 46 01                     	mov	al, byte ptr [esi + 1]
 80003ac: 01 ce                        	add	esi, ecx
 80003ae: 42                           	inc	edx
 80003af: eb ec                        	jmp	-20 <vsnprintf+0x270>
 80003b1: 89 7d ec                     	mov	dword ptr [ebp - 20], edi
 80003b4: 89 55 e8                     	mov	dword ptr [ebp - 24], edx
 80003b7: 31 d2                        	xor	edx, edx
 80003b9: 8b 45 e4                     	mov	eax, dword ptr [ebp - 28]
 80003bc: 2b 45 dc                     	sub	eax, dword ptr [ebp - 36]
 80003bf: b9 00 00 00 00               	mov	ecx, 0
 80003c4: 72 02                        	jb	2 <vsnprintf+0x29b>
 80003c6: 89 c1                        	mov	ecx, eax
 80003c8: 85 c9                        	test	ecx, ecx
 80003ca: 8b 75 d8                     	mov	esi, dword ptr [ebp - 40]
 80003cd: 74 1e                        	je	30 <vsnprintf+0x2c0>
 80003cf: 89 4d e4                     	mov	dword ptr [ebp - 28], ecx
 80003d2: 8b 7d e8                     	mov	edi, dword ptr [ebp - 24]
 80003d5: 39 df                        	cmp	edi, ebx
 80003d7: 73 0a                        	jae	10 <vsnprintf+0x2b6>
 80003d9: 8b 45 ec                     	mov	eax, dword ptr [ebp - 20]
 80003dc: c6 00 20                     	mov	byte ptr [eax], 32
 80003df: 40                           	inc	eax
 80003e0: 89 45 ec                     	mov	dword ptr [ebp - 20], eax
 80003e3: 47                           	inc	edi
 80003e4: 49                           	dec	ecx
 80003e5: 75 ee                        	jne	-18 <vsnprintf+0x2a8>
 80003e7: 8b 45 e4                     	mov	eax, dword ptr [ebp - 28]
 80003ea: 01 45 e8                     	add	dword ptr [ebp - 24], eax
 80003ed: 31 c0                        	xor	eax, eax
 80003ef: 31 db                        	xor	ebx, ebx
 80003f1: 8a 4e 01                     	mov	cl, byte ptr [esi + 1]
 80003f4: 46                           	inc	esi
 80003f5: 84 c9                        	test	cl, cl
 80003f7: 8b 7d d4                     	mov	edi, dword ptr [ebp - 44]
 80003fa: 0f 85 63 fd ff ff            	jne	-669 <vsnprintf+0x36>
 8000400: eb 18                        	jmp	24 <vsnprintf+0x2ed>
 8000402: 89 75 d8                     	mov	dword ptr [ebp - 40], esi
 8000405: 83 65 f0 bf                  	and	dword ptr [ebp - 16], -65
 8000409: 8b 45 e0                     	mov	eax, dword ptr [ebp - 32]
 800040c: 8b 30                        	mov	esi, dword ptr [eax]
 800040e: e9 c6 fe ff ff               	jmp	-314 <vsnprintf+0x1ac>
 8000413: c7 45 e8 00 00 00 00         	mov	dword ptr [ebp - 24], 0
 800041a: 8b 45 08                     	mov	eax, dword ptr [ebp + 8]
 800041d: 8b 4d e8                     	mov	ecx, dword ptr [ebp - 24]
 8000420: c6 04 08 00                  	mov	byte ptr [eax + ecx], 0
 8000424: 89 c8                        	mov	eax, ecx
 8000426: 83 c4 30                     	add	esp, 48
 8000429: 5e                           	pop	esi
 800042a: 5f                           	pop	edi
 800042b: 5b                           	pop	ebx
 800042c: 5d                           	pop	ebp
 800042d: c3                           	ret

0800042e puts:
 800042e: 55                           	push	ebp
 800042f: 89 e5                        	mov	ebp, esp
 8000431: 53                           	push	ebx
 8000432: 56                           	push	esi
 8000433: 50                           	push	eax
 8000434: 8b 75 08                     	mov	esi, dword ptr [ebp + 8]
 8000437: 56                           	push	esi
 8000438: e8 53 02 00 00               	call	595 <strlen>
 800043d: 83 c4 04                     	add	esp, 4
 8000440: 89 c1                        	mov	ecx, eax
 8000442: 31 db                        	xor	ebx, ebx
 8000444: 89 f0                        	mov	eax, esi
 8000446: cd 80                        	int	128
 8000448: 8d 45 f7                     	lea	eax, [ebp - 9]
 800044b: c6 00 0a                     	mov	byte ptr [eax], 10
 800044e: 31 f6                        	xor	esi, esi
 8000450: 46                           	inc	esi
 8000451: 31 db                        	xor	ebx, ebx
 8000453: 89 f1                        	mov	ecx, esi
 8000455: cd 80                        	int	128
 8000457: 89 f0                        	mov	eax, esi
 8000459: 83 c4 04                     	add	esp, 4
 800045c: 5e                           	pop	esi
 800045d: 5b                           	pop	ebx
 800045e: 5d                           	pop	ebp
 800045f: c3                           	ret

08000460 fputs:
 8000460: 55                           	push	ebp
 8000461: 89 e5                        	mov	ebp, esp
 8000463: 53                           	push	ebx
 8000464: 56                           	push	esi
 8000465: 50                           	push	eax
 8000466: 8d 45 f7                     	lea	eax, [ebp - 9]
 8000469: c6 00 0a                     	mov	byte ptr [eax], 10
 800046c: 31 f6                        	xor	esi, esi
 800046e: 46                           	inc	esi
 800046f: 31 db                        	xor	ebx, ebx
 8000471: 89 f1                        	mov	ecx, esi
 8000473: cd 80                        	int	128
 8000475: 89 f0                        	mov	eax, esi
 8000477: 83 c4 04                     	add	esp, 4
 800047a: 5e                           	pop	esi
 800047b: 5b                           	pop	ebx
 800047c: 5d                           	pop	ebp
 800047d: c3                           	ret

0800047e perror:
 800047e: 55                           	push	ebp
 800047f: 89 e5                        	mov	ebp, esp
 8000481: 5d                           	pop	ebp
 8000482: c3                           	ret

08000483 vsprintf:
 8000483: 55                           	push	ebp
 8000484: 89 e5                        	mov	ebp, esp
 8000486: ff 75 10                     	push	dword ptr [ebp + 16]
 8000489: ff 75 0c                     	push	dword ptr [ebp + 12]
 800048c: 68 00 01 00 00               	push	256
 8000491: ff 75 08                     	push	dword ptr [ebp + 8]
 8000494: e8 94 fc ff ff               	call	-876 <vsnprintf>
 8000499: 83 c4 10                     	add	esp, 16
 800049c: 5d                           	pop	ebp
 800049d: c3                           	ret

0800049e sprintf:
 800049e: 55                           	push	ebp
 800049f: 89 e5                        	mov	ebp, esp
 80004a1: 8d 45 10                     	lea	eax, [ebp + 16]
 80004a4: 50                           	push	eax
 80004a5: ff 70 fc                     	push	dword ptr [eax - 4]
 80004a8: 68 00 01 00 00               	push	256
 80004ad: ff 75 08                     	push	dword ptr [ebp + 8]
 80004b0: e8 78 fc ff ff               	call	-904 <vsnprintf>
 80004b5: 83 c4 10                     	add	esp, 16
 80004b8: 5d                           	pop	ebp
 80004b9: c3                           	ret

080004ba fprintf:
 80004ba: 55                           	push	ebp
 80004bb: 89 e5                        	mov	ebp, esp
 80004bd: 81 ec 00 01 00 00            	sub	esp, 256
 80004c3: 8d 45 10                     	lea	eax, [ebp + 16]
 80004c6: 8d 8d 00 ff ff ff            	lea	ecx, [ebp - 256]
 80004cc: 50                           	push	eax
 80004cd: ff 70 fc                     	push	dword ptr [eax - 4]
 80004d0: 68 00 01 00 00               	push	256
 80004d5: 51                           	push	ecx
 80004d6: e8 52 fc ff ff               	call	-942 <vsnprintf>
 80004db: 81 c4 10 01 00 00            	add	esp, 272
 80004e1: 5d                           	pop	ebp
 80004e2: c3                           	ret

080004e3 printf:
 80004e3: 55                           	push	ebp
 80004e4: 89 e5                        	mov	ebp, esp
 80004e6: 53                           	push	ebx
 80004e7: 57                           	push	edi
 80004e8: 56                           	push	esi
 80004e9: 81 ec 00 01 00 00            	sub	esp, 256
 80004ef: 8d 45 0c                     	lea	eax, [ebp + 12]
 80004f2: 8d b5 f4 fe ff ff            	lea	esi, [ebp - 268]
 80004f8: 50                           	push	eax
 80004f9: ff 70 fc                     	push	dword ptr [eax - 4]
 80004fc: 68 00 01 00 00               	push	256
 8000501: 56                           	push	esi
 8000502: e8 26 fc ff ff               	call	-986 <vsnprintf>
 8000507: 83 c4 10                     	add	esp, 16
 800050a: 89 c7                        	mov	edi, eax
 800050c: 31 db                        	xor	ebx, ebx
 800050e: 89 f0                        	mov	eax, esi
 8000510: 89 f9                        	mov	ecx, edi
 8000512: cd 80                        	int	128
 8000514: 89 f8                        	mov	eax, edi
 8000516: 81 c4 00 01 00 00            	add	esp, 256
 800051c: 5e                           	pop	esi
 800051d: 5f                           	pop	edi
 800051e: 5b                           	pop	ebx
 800051f: 5d                           	pop	ebp
 8000520: c3                           	ret

08000521 fclose:
 8000521: 55                           	push	ebp
 8000522: 89 e5                        	mov	ebp, esp
 8000524: 31 c0                        	xor	eax, eax
 8000526: 5d                           	pop	ebp
 8000527: c3                           	ret
 8000528: cc                           	int3
 8000529: cc                           	int3
 800052a: cc                           	int3
 800052b: cc                           	int3
 800052c: cc                           	int3
 800052d: cc                           	int3
 800052e: cc                           	int3
 800052f: cc                           	int3

08000530 memset:
 8000530: 57                           	push	edi
 8000531: 8b 4c 24 10                  	mov	ecx, dword ptr [esp + 16]
 8000535: 8a 44 24 0c                  	mov	al, byte ptr [esp + 12]
 8000539: 8b 7c 24 08                  	mov	edi, dword ptr [esp + 8]
 800053d: fc                           	cld
 800053e: f3 aa                        	rep		stosb	byte ptr es:[edi], al
 8000540: 8b 44 24 08                  	mov	eax, dword ptr [esp + 8]
 8000544: 5f                           	pop	edi
 8000545: c3                           	ret

08000546 memcpy:
 8000546: 57                           	push	edi
 8000547: 56                           	push	esi
 8000548: 8b 4c 24 14                  	mov	ecx, dword ptr [esp + 20]
 800054c: 8b 74 24 10                  	mov	esi, dword ptr [esp + 16]
 8000550: 8b 7c 24 0c                  	mov	edi, dword ptr [esp + 12]
 8000554: 89 c8                        	mov	eax, ecx
 8000556: c1 e9 02                     	shr	ecx, 2
 8000559: 83 e0 03                     	and	eax, 3
 800055c: fc                           	cld
 800055d: f3 a5                        	rep		movsd	dword ptr es:[edi], dword ptr [esi]
 800055f: 89 c1                        	mov	ecx, eax
 8000561: f3 a4                        	rep		movsb	byte ptr es:[edi], byte ptr [esi]
 8000563: 8b 44 24 0c                  	mov	eax, dword ptr [esp + 12]
 8000567: 5e                           	pop	esi
 8000568: 5f                           	pop	edi
 8000569: c3                           	ret

0800056a getSS:
 800056a: 66 8c d0                     	mov	ax, ss
 800056d: c3                           	ret
 800056e: cc                           	int3
 800056f: cc                           	int3

08000570 strtok:
 8000570: 55                           	push	ebp
 8000571: 89 e5                        	mov	ebp, esp
 8000573: 53                           	push	ebx
 8000574: 57                           	push	edi
 8000575: 56                           	push	esi
 8000576: 50                           	push	eax
 8000577: 8b 4d 08                     	mov	ecx, dword ptr [ebp + 8]
 800057a: 85 c9                        	test	ecx, ecx
 800057c: 74 08                        	je	8 <strtok+0x16>
 800057e: 89 0d 14 20 00 08            	mov	dword ptr [134225940], ecx
 8000584: eb 06                        	jmp	6 <strtok+0x1c>
 8000586: 8b 0d 14 20 00 08            	mov	ecx, dword ptr [134225940]
 800058c: 8a 19                        	mov	bl, byte ptr [ecx]
 800058e: 84 db                        	test	bl, bl
 8000590: 74 3e                        	je	62 <strtok+0x60>
 8000592: 8b 45 0c                     	mov	eax, dword ptr [ebp + 12]
 8000595: 40                           	inc	eax
 8000596: 89 45 f0                     	mov	dword ptr [ebp - 16], eax
 8000599: 89 c8                        	mov	eax, ecx
 800059b: 8b 55 0c                     	mov	edx, dword ptr [ebp + 12]
 800059e: 8a 12                        	mov	dl, byte ptr [edx]
 80005a0: 84 d2                        	test	dl, dl
 80005a2: 74 22                        	je	34 <strtok+0x56>
 80005a4: 8d 79 01                     	lea	edi, [ecx + 1]
 80005a7: 8b 75 f0                     	mov	esi, dword ptr [ebp - 16]
 80005aa: 38 d3                        	cmp	bl, dl
 80005ac: 75 11                        	jne	17 <strtok+0x4f>
 80005ae: c6 01 00                     	mov	byte ptr [ecx], 0
 80005b1: 89 3d 14 20 00 08            	mov	dword ptr [134225940], edi
 80005b7: 39 c1                        	cmp	ecx, eax
 80005b9: 75 17                        	jne	23 <strtok+0x62>
 80005bb: 31 db                        	xor	ebx, ebx
 80005bd: 89 f8                        	mov	eax, edi
 80005bf: 8a 16                        	mov	dl, byte ptr [esi]
 80005c1: 46                           	inc	esi
 80005c2: 84 d2                        	test	dl, dl
 80005c4: 75 e4                        	jne	-28 <strtok+0x3a>
 80005c6: 8a 59 01                     	mov	bl, byte ptr [ecx + 1]
 80005c9: 41                           	inc	ecx
 80005ca: 84 db                        	test	bl, bl
 80005cc: 75 cd                        	jne	-51 <strtok+0x2b>
 80005ce: eb 02                        	jmp	2 <strtok+0x62>
 80005d0: 31 c0                        	xor	eax, eax
 80005d2: 83 c4 04                     	add	esp, 4
 80005d5: 5e                           	pop	esi
 80005d6: 5f                           	pop	edi
 80005d7: 5b                           	pop	ebx
 80005d8: 5d                           	pop	ebp
 80005d9: c3                           	ret

080005da strcmp:
 80005da: 55                           	push	ebp
 80005db: 89 e5                        	mov	ebp, esp
 80005dd: 56                           	push	esi
 80005de: 8b 4d 0c                     	mov	ecx, dword ptr [ebp + 12]
 80005e1: 8b 55 08                     	mov	edx, dword ptr [ebp + 8]
 80005e4: 8a 02                        	mov	al, byte ptr [edx]
 80005e6: 84 c0                        	test	al, al
 80005e8: 74 12                        	je	18 <strcmp+0x22>
 80005ea: 42                           	inc	edx
 80005eb: 31 f6                        	xor	esi, esi
 80005ed: 46                           	inc	esi
 80005ee: 3a 01                        	cmp	al, byte ptr [ecx]
 80005f0: 75 0c                        	jne	12 <strcmp+0x24>
 80005f2: 01 f1                        	add	ecx, esi
 80005f4: 8a 02                        	mov	al, byte ptr [edx]
 80005f6: 01 f2                        	add	edx, esi
 80005f8: 84 c0                        	test	al, al
 80005fa: 75 f2                        	jne	-14 <strcmp+0x14>
 80005fc: 31 c0                        	xor	eax, eax
 80005fe: 0f b6 c0                     	movzx	eax, al
 8000601: 0f b6 09                     	movzx	ecx, byte ptr [ecx]
 8000604: 29 c8                        	sub	eax, ecx
 8000606: 5e                           	pop	esi
 8000607: 5d                           	pop	ebp
 8000608: c3                           	ret

08000609 strcasecmp:
 8000609: 55                           	push	ebp
 800060a: 89 e5                        	mov	ebp, esp
 800060c: 53                           	push	ebx
 800060d: 57                           	push	edi
 800060e: 56                           	push	esi
 800060f: 8b 4d 0c                     	mov	ecx, dword ptr [ebp + 12]
 8000612: 8b 55 08                     	mov	edx, dword ptr [ebp + 8]
 8000615: 8a 02                        	mov	al, byte ptr [edx]
 8000617: 84 c0                        	test	al, al
 8000619: 74 3c                        	je	60 <strcasecmp+0x4e>
 800061b: 42                           	inc	edx
 800061c: b3 02                        	mov	bl, 2
 800061e: 0f be f0                     	movsx	esi, al
 8000621: 84 9e 84 10 00 08            	test	byte ptr [esi + 134221956], bl
 8000627: 74 07                        	je	7 <strcasecmp+0x27>
 8000629: bf e0 ff ff ff               	mov	edi, 4294967264
 800062e: 01 fe                        	add	esi, edi
 8000630: 0f be 39                     	movsx	edi, byte ptr [ecx]
 8000633: 84 9f 84 10 00 08            	test	byte ptr [edi + 134221956], bl
 8000639: 74 09                        	je	9 <strcasecmp+0x3b>
 800063b: bb e0 ff ff ff               	mov	ebx, 4294967264
 8000640: 01 df                        	add	edi, ebx
 8000642: b3 02                        	mov	bl, 2
 8000644: 39 fe                        	cmp	esi, edi
 8000646: 75 11                        	jne	17 <strcasecmp+0x50>
 8000648: 31 c0                        	xor	eax, eax
 800064a: 40                           	inc	eax
 800064b: 89 c6                        	mov	esi, eax
 800064d: 01 c1                        	add	ecx, eax
 800064f: 8a 02                        	mov	al, byte ptr [edx]
 8000651: 01 f2                        	add	edx, esi
 8000653: 84 c0                        	test	al, al
 8000655: 75 c7                        	jne	-57 <strcasecmp+0x15>
 8000657: 31 c0                        	xor	eax, eax
 8000659: 0f b6 c0                     	movzx	eax, al
 800065c: 0f b6 09                     	movzx	ecx, byte ptr [ecx]
 800065f: 29 c8                        	sub	eax, ecx
 8000661: 5e                           	pop	esi
 8000662: 5f                           	pop	edi
 8000663: 5b                           	pop	ebx
 8000664: 5d                           	pop	ebp
 8000665: c3                           	ret

08000666 memcmp:
 8000666: 55                           	push	ebp
 8000667: 89 e5                        	mov	ebp, esp
 8000669: 53                           	push	ebx
 800066a: 57                           	push	edi
 800066b: 56                           	push	esi
 800066c: 31 c0                        	xor	eax, eax
 800066e: 8b 4d 10                     	mov	ecx, dword ptr [ebp + 16]
 8000671: 8b 55 0c                     	mov	edx, dword ptr [ebp + 12]
 8000674: 8b 75 08                     	mov	esi, dword ptr [ebp + 8]
 8000677: 31 ff                        	xor	edi, edi
 8000679: 39 f9                        	cmp	ecx, edi
 800067b: 74 0e                        	je	14 <memcmp+0x25>
 800067d: 8a 1c 3e                     	mov	bl, byte ptr [esi + edi]
 8000680: 2a 1c 3a                     	sub	bl, byte ptr [edx + edi]
 8000683: 8d 7f 01                     	lea	edi, [edi + 1]
 8000686: 74 f1                        	je	-15 <memcmp+0x13>
 8000688: 0f b6 c3                     	movzx	eax, bl
 800068b: 5e                           	pop	esi
 800068c: 5f                           	pop	edi
 800068d: 5b                           	pop	ebx
 800068e: 5d                           	pop	ebp
 800068f: c3                           	ret

08000690 strlen:
 8000690: 55                           	push	ebp
 8000691: 89 e5                        	mov	ebp, esp
 8000693: 8b 4d 08                     	mov	ecx, dword ptr [ebp + 8]
 8000696: 80 39 00                     	cmp	byte ptr [ecx], 0
 8000699: 74 0e                        	je	14 <strlen+0x19>
 800069b: 31 c0                        	xor	eax, eax
 800069d: 80 7c 01 01 00               	cmp	byte ptr [ecx + eax + 1], 0
 80006a2: 8d 40 01                     	lea	eax, [eax + 1]
 80006a5: 75 f6                        	jne	-10 <strlen+0xd>
 80006a7: eb 02                        	jmp	2 <strlen+0x1b>
 80006a9: 31 c0                        	xor	eax, eax
 80006ab: 5d                           	pop	ebp
 80006ac: c3                           	ret

080006ad strcat:
 80006ad: 55                           	push	ebp
 80006ae: 89 e5                        	mov	ebp, esp
 80006b0: 56                           	push	esi
 80006b1: 8b 45 0c                     	mov	eax, dword ptr [ebp + 12]
 80006b4: 8b 75 08                     	mov	esi, dword ptr [ebp + 8]
 80006b7: 80 3e 00                     	cmp	byte ptr [esi], 0
 80006ba: 74 0e                        	je	14 <strcat+0x1d>
 80006bc: 31 c9                        	xor	ecx, ecx
 80006be: 80 7c 0e 01 00               	cmp	byte ptr [esi + ecx + 1], 0
 80006c3: 8d 49 01                     	lea	ecx, [ecx + 1]
 80006c6: 75 f6                        	jne	-10 <strcat+0x11>
 80006c8: eb 02                        	jmp	2 <strcat+0x1f>
 80006ca: 31 c9                        	xor	ecx, ecx
 80006cc: 01 f1                        	add	ecx, esi
 80006ce: 80 38 00                     	cmp	byte ptr [eax], 0
 80006d1: 74 0e                        	je	14 <strcat+0x34>
 80006d3: 31 d2                        	xor	edx, edx
 80006d5: 42                           	inc	edx
 80006d6: 80 3c 10 00                  	cmp	byte ptr [eax + edx], 0
 80006da: 8d 52 01                     	lea	edx, [edx + 1]
 80006dd: 75 f7                        	jne	-9 <strcat+0x29>
 80006df: eb 03                        	jmp	3 <strcat+0x37>
 80006e1: 31 d2                        	xor	edx, edx
 80006e3: 42                           	inc	edx
 80006e4: 52                           	push	edx
 80006e5: 50                           	push	eax
 80006e6: 51                           	push	ecx
 80006e7: e8 5a fe ff ff               	call	-422 <memcpy>
 80006ec: 83 c4 0c                     	add	esp, 12
 80006ef: 89 f0                        	mov	eax, esi
 80006f1: 5e                           	pop	esi
 80006f2: 5d                           	pop	ebp
 80006f3: c3                           	ret

080006f4 strcpy:
 80006f4: 55                           	push	ebp
 80006f5: 89 e5                        	mov	ebp, esp
 80006f7: 56                           	push	esi
 80006f8: 8b 45 0c                     	mov	eax, dword ptr [ebp + 12]
 80006fb: 8b 75 08                     	mov	esi, dword ptr [ebp + 8]
 80006fe: 80 38 00                     	cmp	byte ptr [eax], 0
 8000701: 74 0e                        	je	14 <strcpy+0x1d>
 8000703: 31 c9                        	xor	ecx, ecx
 8000705: 41                           	inc	ecx
 8000706: 80 3c 08 00                  	cmp	byte ptr [eax + ecx], 0
 800070a: 8d 49 01                     	lea	ecx, [ecx + 1]
 800070d: 75 f7                        	jne	-9 <strcpy+0x12>
 800070f: eb 03                        	jmp	3 <strcpy+0x20>
 8000711: 31 c9                        	xor	ecx, ecx
 8000713: 41                           	inc	ecx
 8000714: 51                           	push	ecx
 8000715: 50                           	push	eax
 8000716: 56                           	push	esi
 8000717: e8 2a fe ff ff               	call	-470 <memcpy>
 800071c: 83 c4 0c                     	add	esp, 12
 800071f: 89 f0                        	mov	eax, esi
 8000721: 5e                           	pop	esi
 8000722: 5d                           	pop	ebp
 8000723: c3                           	ret

Disassembly of section .rodata:

08001000 .rodata:
 8001000: 53                           	push	ebx
 8001001: 53                           	push	ebx
 8001002: 3d 25 58 0a 00               	cmp	eax, 677925
 8001007: 74 65                        	je	101 <.rodata+0x6e>
 8001009: 73 74                        	jae	116 <.rodata+0x7f>
 800100b: 2e 65 6c                     	insb	byte ptr es:[edi], dx
 800100e: 66 00 63 61                  	add	byte ptr [ebx + 97], ah
 8001012: 6e                           	outsb	dx, byte ptr [esi]
 8001013: 6e                           	outsb	dx, byte ptr [esi]
 8001014: 6f                           	outsd	dx, dword ptr [esi]
 8001015: 74 20                        	je	32 <.rodata+0x37>
 8001017: 6f                           	outsd	dx, dword ptr [esi]
 8001018: 70 65                        	jo	101 <.rodata+0x7f>
 800101a: 6e                           	outsb	dx, byte ptr [esi]
 800101b: 20 66 69                     	and	byte ptr [esi + 105], ah
 800101e: 6c                           	insb	byte ptr es:[edi], dx
 800101f: 65 0a 00                     	or	al, byte ptr gs:[eax]
 8001022: 74 65                        	je	101 <_ctype+0x5>
 8001024: 73 74                        	jae	116 <_ctype+0x16>
 8001026: 69 6e 67 0a 00 00 5a         	imul	ebp, dword ptr [esi + 103], 1509949450
 800102d: 02 00                        	add	al, byte ptr [eax]
 800102f: 08 37                        	or	byte ptr [edi], dh
 8001031: 02 00                        	add	al, byte ptr [eax]
 8001033: 08 f1                        	or	cl, dh
 8001035: 03 00                        	add	eax, dword ptr [eax]
 8001037: 08 f1                        	or	cl, dh
 8001039: 03 00                        	add	eax, dword ptr [eax]
 800103b: 08 f1                        	or	cl, dh
 800103d: 03 00                        	add	eax, dword ptr [eax]
 800103f: 08 f1                        	or	cl, dh
 8001041: 03 00                        	add	eax, dword ptr [eax]
 8001043: 08 37                        	or	byte ptr [edi], dh
 8001045: 02 00                        	add	al, byte ptr [eax]
 8001047: 08 f1                        	or	cl, dh
 8001049: 03 00                        	add	eax, dword ptr [eax]
 800104b: 08 f1                        	or	cl, dh
 800104d: 03 00                        	add	eax, dword ptr [eax]
 800104f: 08 f1                        	or	cl, dh
 8001051: 03 00                        	add	eax, dword ptr [eax]
 8001053: 08 f1                        	or	cl, dh
 8001055: 03 00                        	add	eax, dword ptr [eax]
 8001057: 08 8e 02 00 08 89            	or	byte ptr [esi - 1995964414], cl
 800105d: 02 00                        	add	al, byte ptr [eax]
 800105f: 08 8e 02 00 08 f1            	or	byte ptr [esi - 251133950], cl
 8001065: 03 00                        	add	eax, dword ptr [eax]
 8001067: 08 f1                        	or	cl, dh
 8001069: 03 00                        	add	eax, dword ptr [eax]
 800106b: 08 02                        	or	byte ptr [edx], al
 800106d: 04 00                        	add	al, 0
 800106f: 08 f1                        	or	cl, dh
 8001071: 03 00                        	add	eax, dword ptr [eax]
 8001073: 08 3b                        	or	byte ptr [ebx], bh
 8001075: 02 00                        	add	al, byte ptr [eax]
 8001077: 08 f1                        	or	cl, dh
 8001079: 03 00                        	add	eax, dword ptr [eax]
 800107b: 08 f1                        	or	cl, dh
 800107d: 03 00                        	add	eax, dword ptr [eax]
 800107f: 08 8e 02 00 08 08            	or	byte ptr [esi + 134742018], cl

08001084 _ctype:
 8001084: 08 08                        	or	byte ptr [eax], cl
 8001086: 08 08                        	or	byte ptr [eax], cl
 8001088: 08 08                        	or	byte ptr [eax], cl
 800108a: 08 08                        	or	byte ptr [eax], cl
 800108c: 08 28                        	or	byte ptr [eax], ch
 800108e: 28 28                        	sub	byte ptr [eax], ch
 8001090: 28 28                        	sub	byte ptr [eax], ch
 8001092: 08 08                        	or	byte ptr [eax], cl
 8001094: 08 08                        	or	byte ptr [eax], cl
 8001096: 08 08                        	or	byte ptr [eax], cl
 8001098: 08 08                        	or	byte ptr [eax], cl
 800109a: 08 08                        	or	byte ptr [eax], cl
 800109c: 08 08                        	or	byte ptr [eax], cl
 800109e: 08 08                        	or	byte ptr [eax], cl
 80010a0: 08 08                        	or	byte ptr [eax], cl
 80010a2: 08 08                        	or	byte ptr [eax], cl
 80010a4: 20 10                        	and	byte ptr [eax], dl
 80010a6: 10 10                        	adc	byte ptr [eax], dl
 80010a8: 10 10                        	adc	byte ptr [eax], dl
 80010aa: 10 10                        	adc	byte ptr [eax], dl
 80010ac: 10 10                        	adc	byte ptr [eax], dl
 80010ae: 10 10                        	adc	byte ptr [eax], dl
 80010b0: 10 10                        	adc	byte ptr [eax], dl
 80010b2: 10 10                        	adc	byte ptr [eax], dl
 80010b4: 44                           	inc	esp
 80010b5: 44                           	inc	esp
 80010b6: 44                           	inc	esp
 80010b7: 44                           	inc	esp
 80010b8: 44                           	inc	esp
 80010b9: 44                           	inc	esp
 80010ba: 44                           	inc	esp
 80010bb: 44                           	inc	esp
 80010bc: 44                           	inc	esp
 80010bd: 44                           	inc	esp
 80010be: 10 10                        	adc	byte ptr [eax], dl
 80010c0: 10 10                        	adc	byte ptr [eax], dl
 80010c2: 10 10                        	adc	byte ptr [eax], dl
 80010c4: 10 41 41                     	adc	byte ptr [ecx + 65], al
 80010c7: 41                           	inc	ecx
 80010c8: 41                           	inc	ecx
 80010c9: 41                           	inc	ecx
 80010ca: 41                           	inc	ecx
 80010cb: 01 01                        	add	dword ptr [ecx], eax
 80010cd: 01 01                        	add	dword ptr [ecx], eax
 80010cf: 01 01                        	add	dword ptr [ecx], eax
 80010d1: 01 01                        	add	dword ptr [ecx], eax
 80010d3: 01 01                        	add	dword ptr [ecx], eax
 80010d5: 01 01                        	add	dword ptr [ecx], eax
 80010d7: 01 01                        	add	dword ptr [ecx], eax
 80010d9: 01 01                        	add	dword ptr [ecx], eax
 80010db: 01 01                        	add	dword ptr [ecx], eax
 80010dd: 01 01                        	add	dword ptr [ecx], eax
 80010df: 10 10                        	adc	byte ptr [eax], dl
 80010e1: 10 10                        	adc	byte ptr [eax], dl
 80010e3: 10 10                        	adc	byte ptr [eax], dl
 80010e5: 42                           	inc	edx
 80010e6: 42                           	inc	edx
 80010e7: 42                           	inc	edx
 80010e8: 42                           	inc	edx
 80010e9: 42                           	inc	edx
 80010ea: 42                           	inc	edx
 80010eb: 02 02                        	add	al, byte ptr [edx]
 80010ed: 02 02                        	add	al, byte ptr [edx]
 80010ef: 02 02                        	add	al, byte ptr [edx]
 80010f1: 02 02                        	add	al, byte ptr [edx]
 80010f3: 02 02                        	add	al, byte ptr [edx]
 80010f5: 02 02                        	add	al, byte ptr [edx]
 80010f7: 02 02                        	add	al, byte ptr [edx]
 80010f9: 02 02                        	add	al, byte ptr [edx]
 80010fb: 02 02                        	add	al, byte ptr [edx]
 80010fd: 02 02                        	add	al, byte ptr [edx]
 80010ff: 10 10                        	adc	byte ptr [eax], dl
 8001101: 10 10                        	adc	byte ptr [eax], dl
 8001103: 08                           	<unknown>

Disassembly of section .data:

08002000 stdin:
 8002000: 04 20                        	add	al, 32
 8002002: 00 08                        	add	byte ptr [eax], cl

Disassembly of section .bss:

08002004 _internal_stdin:
...

08002014 strtok.buffer:
...

Disassembly of section .comment:

00000000 .comment:
       0: 63 6c 61 6e                  	arpl	word ptr [ecx + 2*eiz + 110], bp
       4: 67 20 76 65                  	and	byte ptr [bp + 101], dh
       8: 72 73                        	jb	115 <.comment+0x7d>
       a: 69 6f 6e 20 39 2e 30         	imul	ebp, dword ptr [edi + 110], 808335648
      11: 2e 30 20                     	xor	byte ptr cs:[eax], ah
      14: 28 74 61 67                  	sub	byte ptr [ecx + 2*eiz + 103], dh
      18: 73 2f                        	jae	47 <.comment+0x49>
      1a: 52                           	push	edx
      1b: 45                           	inc	ebp
      1c: 4c                           	dec	esp
      1d: 45                           	inc	ebp
      1e: 41                           	inc	ecx
      1f: 53                           	push	ebx
      20: 45                           	inc	ebp
      21: 5f                           	pop	edi
      22: 39 30                        	cmp	dword ptr [eax], esi
      24: 30 2f                        	xor	byte ptr [edi], ch
      26: 66 69 6e 61 6c 29            	imul	bp, word ptr [esi + 97], 10604
      2c: 00 4c 69 6e                  	add	byte ptr [ecx + 2*ebp + 110], cl
      30: 6b 65 72 3a                  	imul	esp, dword ptr [ebp + 114], 58
      34: 20 4c 4c 44                  	and	byte ptr [esp + 2*ecx + 68], cl
      38: 20 39                        	and	byte ptr [ecx], bh
      3a: 2e 30 2e                     	xor	byte ptr cs:[esi], ch
      3d: 30 00                        	xor	byte ptr [eax], al
      3f: 00                           	<unknown>

Disassembly of section .symtab:

00000000 .symtab:
		...
      10: 01 00                        	add	dword ptr [eax], eax
		...
      1a: 00 00                        	add	byte ptr [eax], al
      1c: 04 00                        	add	al, 0
      1e: f1                           	<unknown>
      1f: ff 0b                        	dec	dword ptr [ebx]
		...
      29: 00 00                        	add	byte ptr [eax], al
      2b: 00 04 00                     	add	byte ptr [eax + eax], al
      2e: f1                           	<unknown>
      2f: ff 13                        	call	dword ptr [ebx]
		...
      39: 00 00                        	add	byte ptr [eax], al
      3b: 00 04 00                     	add	byte ptr [eax + eax], al
      3e: f1                           	<unknown>
      3f: ff 1e                        	call	[esi]
		...
      49: 00 00                        	add	byte ptr [eax], al
      4b: 00 04 00                     	add	byte ptr [eax + eax], al
      4e: f1                           	<unknown>
      4f: ff 27                        	jmp	dword ptr [edi]
      51: 00 00                        	add	byte ptr [eax], al
      53: 00 14 20                     	add	byte ptr [eax + eiz], dl
      56: 00 08                        	add	byte ptr [eax], cl
      58: 04 00                        	add	al, 0
      5a: 00 00                        	add	byte ptr [eax], al
      5c: 01 00                        	add	dword ptr [eax], eax
      5e: 04 00                        	add	al, 0
      60: 35 00 00 00 00               	xor	eax, 0
      65: 00 00                        	add	byte ptr [eax], al
      67: 00 00                        	add	byte ptr [eax], al
      69: 00 00                        	add	byte ptr [eax], al
      6b: 00 04 00                     	add	byte ptr [eax + eax], al
      6e: f1                           	<unknown>
      6f: ff 3d 00 00 00 00            	<unknown>
      75: 00 00                        	add	byte ptr [eax], al
      77: 08 8a 00 00 00 12            	or	byte ptr [edx + 301989888], cl
      7d: 00 01                        	add	byte ptr [ecx], al
      7f: 00 44 00 00                  	add	byte ptr [eax + eax], al
      83: 00 6a 05                     	add	byte ptr [edx + 5], ch
      86: 00 08                        	add	byte ptr [eax], cl
      88: 00 00                        	add	byte ptr [eax], al
      8a: 00 00                        	add	byte ptr [eax], al
      8c: 10 00                        	adc	byte ptr [eax], al
      8e: 01 00                        	add	dword ptr [eax], eax
      90: 4a                           	dec	edx
      91: 00 00                        	add	byte ptr [eax], al
      93: 00 e3                        	add	bl, ah
      95: 04 00                        	add	al, 0
      97: 08 3e                        	or	byte ptr [esi], bh
      99: 00 00                        	add	byte ptr [eax], al
      9b: 00 12                        	add	byte ptr [edx], dl
      9d: 00 01                        	add	byte ptr [ecx], al
      9f: 00 51 00                     	add	byte ptr [ecx], dl
      a2: 00 00                        	add	byte ptr [eax], al
      a4: 2e 04 00                     	add	al, 0
      a7: 08 32                        	or	byte ptr [edx], dh
      a9: 00 00                        	add	byte ptr [eax], al
      ab: 00 12                        	add	byte ptr [edx], dl
      ad: 00 01                        	add	byte ptr [ecx], al
      af: 00 56 00                     	add	byte ptr [esi], dl
      b2: 00 00                        	add	byte ptr [eax], al
      b4: 04 20                        	add	al, 32
      b6: 00 08                        	add	byte ptr [eax], cl
      b8: 10 00                        	adc	byte ptr [eax], al
      ba: 00 00                        	add	byte ptr [eax], al
      bc: 11 00                        	adc	dword ptr [eax], eax
      be: 04 00                        	add	al, 0
      c0: 66 00 00                     	add	byte ptr [eax], al
      c3: 00 93 00 00 08 05            	add	byte ptr [ebx + 84410368], dl
      c9: 00 00                        	add	byte ptr [eax], al
      cb: 00 12                        	add	byte ptr [edx], dl
      cd: 00 01                        	add	byte ptr [ecx], al
      cf: 00 6f 00                     	add	byte ptr [edi], ch
      d2: 00 00                        	add	byte ptr [eax], al
      d4: 21 05 00 08 07 00            	and	dword ptr [460800], eax
      da: 00 00                        	add	byte ptr [eax], al
      dc: 12 00                        	adc	al, byte ptr [eax]
      de: 01 00                        	add	dword ptr [eax], eax
      e0: 76 00                        	jbe	0 <.comment+0xe2>
      e2: 00 00                        	add	byte ptr [eax], al
      e4: 98                           	cwde
      e5: 00 00                        	add	byte ptr [eax], al
      e7: 08 07                        	or	byte ptr [edi], al
      e9: 00 00                        	add	byte ptr [eax], al
      eb: 00 12                        	add	byte ptr [edx], dl
      ed: 00 01                        	add	byte ptr [ecx], al
      ef: 00 7b 00                     	add	byte ptr [ebx], bh
      f2: 00 00                        	add	byte ptr [eax], al
      f4: 9f                           	lahf
      f5: 00 00                        	add	byte ptr [eax], al
      f7: 08 07                        	or	byte ptr [edi], al
      f9: 00 00                        	add	byte ptr [eax], al
      fb: 00 12                        	add	byte ptr [edx], dl
      fd: 00 01                        	add	byte ptr [ecx], al
      ff: 00 82 00 00 00 8c            	add	byte ptr [edx - 1946157056], al
     105: 00 00                        	add	byte ptr [eax], al
     107: 08 07                        	or	byte ptr [edi], al
     109: 00 00                        	add	byte ptr [eax], al
     10b: 00 12                        	add	byte ptr [edx], dl
     10d: 00 01                        	add	byte ptr [ecx], al
     10f: 00 88 00 00 00 ba            	add	byte ptr [eax - 1174405120], cl
     115: 04 00                        	add	al, 0
     117: 08 29                        	or	byte ptr [ecx], ch
     119: 00 00                        	add	byte ptr [eax], al
     11b: 00 12                        	add	byte ptr [edx], dl
     11d: 00 01                        	add	byte ptr [ecx], al
     11f: 00 90 00 00 00 d6            	add	byte ptr [eax - 704643072], dl
     125: 00 00                        	add	byte ptr [eax], al
     127: 08 08                        	or	byte ptr [eax], cl
     129: 00 00                        	add	byte ptr [eax], al
     12b: 00 12                        	add	byte ptr [edx], dl
     12d: 00 01                        	add	byte ptr [ecx], al
     12f: 00 96 00 00 00 60            	add	byte ptr [esi + 1610612736], dl
     135: 04 00                        	add	al, 0
     137: 08 1e                        	or	byte ptr [esi], bl
     139: 00 00                        	add	byte ptr [eax], al
     13b: 00 12                        	add	byte ptr [edx], dl
     13d: 00 01                        	add	byte ptr [ecx], al
     13f: 00 9c 00 00 00 fe 00         	add	byte ptr [eax + eax + 16646144], bl
     146: 00 08                        	add	byte ptr [eax], cl
     148: 1e                           	push	ds
     149: 00 00                        	add	byte ptr [eax], al
     14b: 00 12                        	add	byte ptr [edx], dl
     14d: 00 01                        	add	byte ptr [ecx], al
     14f: 00 a1 00 00 00 1c            	add	byte ptr [ecx + 469762048], ah
     155: 01 00                        	add	dword ptr [eax], eax
     157: 08 11                        	or	byte ptr [ecx], dl
     159: 00 00                        	add	byte ptr [eax], al
     15b: 00 12                        	add	byte ptr [edx], dl
     15d: 00 01                        	add	byte ptr [ecx], al
     15f: 00 a9 00 00 00 ce            	add	byte ptr [ecx - 838860800], ch
     165: 00 00                        	add	byte ptr [eax], al
     167: 08 08                        	or	byte ptr [eax], cl
     169: 00 00                        	add	byte ptr [eax], al
     16b: 00 12                        	add	byte ptr [edx], dl
     16d: 00 01                        	add	byte ptr [ecx], al
     16f: 00 ae 00 00 00 a6            	add	byte ptr [esi - 1509949440], ch
     175: 00 00                        	add	byte ptr [eax], al
     177: 08 28                        	or	byte ptr [eax], ch
     179: 00 00                        	add	byte ptr [eax], al
     17b: 00 12                        	add	byte ptr [edx], dl
     17d: 00 01                        	add	byte ptr [ecx], al
     17f: 00 b5 00 00 00 7e            	add	byte ptr [ebp + 2113929216], dh
     185: 04 00                        	add	al, 0
     187: 08 05 00 00 00 12            	or	byte ptr [301989888], al
     18d: 00 01                        	add	byte ptr [ecx], al
     18f: 00 bc 00 00 00 de 00         	add	byte ptr [eax + eax + 14548992], bh
     196: 00 08                        	add	byte ptr [eax], cl
     198: 20 00                        	and	byte ptr [eax], al
     19a: 00 00                        	add	byte ptr [eax], al
     19c: 12 00                        	adc	al, byte ptr [eax]
     19e: 01 00                        	add	dword ptr [eax], eax
     1a0: c4 00                        	les	eax, [eax]
     1a2: 00 00                        	add	byte ptr [eax], al
     1a4: 9e                           	sahf
     1a5: 04 00                        	add	al, 0
     1a7: 08 1c 00                     	or	byte ptr [eax + eax], bl
     1aa: 00 00                        	add	byte ptr [eax], al
     1ac: 12 00                        	adc	al, byte ptr [eax]
     1ae: 01 00                        	add	dword ptr [eax], eax
     1b0: cc                           	int3
     1b1: 00 00                        	add	byte ptr [eax], al
     1b3: 00 00                        	add	byte ptr [eax], al
     1b5: 20 00                        	and	byte ptr [eax], al
     1b7: 08 04 00                     	or	byte ptr [eax + eax], al
     1ba: 00 00                        	add	byte ptr [eax], al
     1bc: 11 00                        	adc	dword ptr [eax], eax
     1be: 03 00                        	add	eax, dword ptr [eax]
     1c0: d2 00                        	rol	byte ptr [eax], cl
     1c2: 00 00                        	add	byte ptr [eax], al
     1c4: 90                           	nop
     1c5: 06                           	push	es
     1c6: 00 08                        	add	byte ptr [eax], cl
     1c8: 1d 00 00 00 12               	sbb	eax, 301989888
     1cd: 00 01                        	add	byte ptr [ecx], al
     1cf: 00 d9                        	add	cl, bl
     1d1: 00 00                        	add	byte ptr [eax], al
     1d3: 00 2d 01 00 08 01            	add	byte ptr [17301505], ch
     1d9: 03 00                        	add	eax, dword ptr [eax]
     1db: 00 12                        	add	byte ptr [edx], dl
     1dd: 00 01                        	add	byte ptr [ecx], al
     1df: 00 e3                        	add	bl, ah
     1e1: 00 00                        	add	byte ptr [eax], al
     1e3: 00 83 04 00 08 1b            	add	byte ptr [ebx + 453509124], al
     1e9: 00 00                        	add	byte ptr [eax], al
     1eb: 00 12                        	add	byte ptr [edx], dl
     1ed: 00 01                        	add	byte ptr [ecx], al
     1ef: 00 ec                        	add	ah, ch
     1f1: 00 00                        	add	byte ptr [eax], al
     1f3: 00 30                        	add	byte ptr [eax], dh
     1f5: 05 00 08 00 00               	add	eax, 2048
     1fa: 00 00                        	add	byte ptr [eax], al
     1fc: 10 00                        	adc	byte ptr [eax], al
     1fe: 01 00                        	add	dword ptr [eax], eax
     200: f3 00 00                     	rep		add	byte ptr [eax], al
     203: 00 46 05                     	add	byte ptr [esi + 5], al
     206: 00 08                        	add	byte ptr [eax], cl
     208: 00 00                        	add	byte ptr [eax], al
     20a: 00 00                        	add	byte ptr [eax], al
     20c: 10 00                        	adc	byte ptr [eax], al
     20e: 01 00                        	add	dword ptr [eax], eax
     210: fa                           	cli
     211: 00 00                        	add	byte ptr [eax], al
     213: 00 84 10 00 08 80 00         	add	byte ptr [eax + edx + 8390656], al
     21a: 00 00                        	add	byte ptr [eax], al
     21c: 11 00                        	adc	dword ptr [eax], eax
     21e: 02 00                        	add	al, byte ptr [eax]
     220: 01 01                        	add	dword ptr [ecx], eax
     222: 00 00                        	add	byte ptr [eax], al
     224: 66 06                        	push	es
     226: 00 08                        	add	byte ptr [eax], cl
     228: 2a 00                        	sub	al, byte ptr [eax]
     22a: 00 00                        	add	byte ptr [eax], al
     22c: 12 00                        	adc	al, byte ptr [eax]
     22e: 01 00                        	add	dword ptr [eax], eax
     230: 08 01                        	or	byte ptr [ecx], al
     232: 00 00                        	add	byte ptr [eax], al
     234: 09 06                        	or	dword ptr [esi], eax
     236: 00 08                        	add	byte ptr [eax], cl
     238: 5d                           	pop	ebp
     239: 00 00                        	add	byte ptr [eax], al
     23b: 00 12                        	add	byte ptr [edx], dl
     23d: 00 01                        	add	byte ptr [ecx], al
     23f: 00 13                        	add	byte ptr [ebx], dl
     241: 01 00                        	add	dword ptr [eax], eax
     243: 00 ad 06 00 08 47            	add	byte ptr [ebp + 1191706630], ch
     249: 00 00                        	add	byte ptr [eax], al
     24b: 00 12                        	add	byte ptr [edx], dl
     24d: 00 01                        	add	byte ptr [ecx], al
     24f: 00 1a                        	add	byte ptr [edx], bl
     251: 01 00                        	add	dword ptr [eax], eax
     253: 00 da                        	add	dl, bl
     255: 05 00 08 2f 00               	add	eax, 3082240
     25a: 00 00                        	add	byte ptr [eax], al
     25c: 12 00                        	adc	al, byte ptr [eax]
     25e: 01 00                        	add	dword ptr [eax], eax
     260: 21 01                        	and	dword ptr [ecx], eax
     262: 00 00                        	add	byte ptr [eax], al
     264: f4                           	hlt
     265: 06                           	push	es
     266: 00 08                        	add	byte ptr [eax], cl
     268: 30 00                        	xor	byte ptr [eax], al
     26a: 00 00                        	add	byte ptr [eax], al
     26c: 12 00                        	adc	al, byte ptr [eax]
     26e: 01 00                        	add	dword ptr [eax], eax
     270: 28 01                        	sub	byte ptr [ecx], al
     272: 00 00                        	add	byte ptr [eax], al
     274: 70 05                        	jo	5 <.comment+0x27b>
     276: 00 08                        	add	byte ptr [eax], cl
     278: 6a 00                        	push	0
     27a: 00 00                        	add	byte ptr [eax], al
     27c: 12 00                        	adc	al, byte ptr [eax]
     27e: 01 00                        	add	dword ptr [eax], eax

Disassembly of section .shstrtab:

00000000 .shstrtab:
       0: 00 2e                        	add	byte ptr [esi], ch
       2: 74 65                        	je	101 <.comment+0x69>
       4: 78 74                        	js	116 <.comment+0x7a>
       6: 00 2e                        	add	byte ptr [esi], ch
       8: 72 6f                        	jb	111 <.comment+0x79>
       a: 64 61                        	popal
       c: 74 61                        	je	97 <.comment+0x6f>
       e: 00 2e                        	add	byte ptr [esi], ch
      10: 64 61                        	popal
      12: 74 61                        	je	97 <.comment+0x75>
      14: 00 2e                        	add	byte ptr [esi], ch
      16: 62 73 73                     	bound	esi, dword ptr [ebx + 115]
      19: 00 2e                        	add	byte ptr [esi], ch
      1b: 63 6f 6d                     	arpl	word ptr [edi + 109], bp
      1e: 6d                           	insd	dword ptr es:[edi], dx
      1f: 65 6e                        	outsb	dx, byte ptr gs:[esi]
      21: 74 00                        	je	0 <.comment+0x23>
      23: 2e 73 79                     	jae	121 <.comment+0x9f>
      26: 6d                           	insd	dword ptr es:[edi], dx
      27: 74 61                        	je	97 <.comment+0x8a>
      29: 62 00                        	bound	eax, dword ptr [eax]
      2b: 2e 73 68                     	jae	104 <.comment+0x96>
      2e: 73 74                        	jae	116 <.comment+0xa4>
      30: 72 74                        	jb	116 <.comment+0xa6>
      32: 61                           	popal
      33: 62 00                        	bound	eax, dword ptr [eax]
      35: 2e 73 74                     	jae	116 <.comment+0xac>
      38: 72 74                        	jb	116 <.comment+0xae>
      3a: 61                           	popal
      3b: 62 00                        	<unknown>

Disassembly of section .strtab:

00000000 .strtab:
       0: 00 65 6c                     	add	byte ptr [ebp + 108], ah
       3: 66 74 65                     	je	101 <.comment+0x6b>
       6: 73 74                        	jae	116 <.comment+0x7c>
       8: 2e 63 00                     	arpl	word ptr cs:[eax], ax
       b: 73 74                        	jae	116 <.comment+0x81>
       d: 64 69 6f 2e 63 00 73 74      	imul	ebp, dword ptr fs:[edi + 46], 1953693795
      15: 72 69                        	jb	105 <.comment+0x80>
      17: 6e                           	outsb	dx, byte ptr [esi]
      18: 67 2e 61                     	popal
      1b: 73 6d                        	jae	109 <.comment+0x8a>
      1d: 00 73 74                     	add	byte ptr [ebx + 116], dh
      20: 72 69                        	jb	105 <.comment+0x8b>
      22: 6e                           	outsb	dx, byte ptr [esi]
      23: 67 2e 63 00                  	arpl	word ptr cs:[bx + si], ax
      27: 73 74                        	jae	116 <.comment+0x9d>
      29: 72 74                        	jb	116 <.comment+0x9f>
      2b: 6f                           	outsd	dx, dword ptr [esi]
      2c: 6b 2e 62                     	imul	ebp, dword ptr [esi], 98
      2f: 75 66                        	jne	102 <.comment+0x97>
      31: 66 65 72 00                  	jb	0 <.comment+0x35>
      35: 63 74 79 70                  	arpl	word ptr [ecx + 2*edi + 112], si
      39: 65 2e 63 00                  	arpl	word ptr cs:[eax], ax
      3d: 5f                           	pop	edi
      3e: 73 74                        	jae	116 <.comment+0xb4>
      40: 61                           	popal
      41: 72 74                        	jb	116 <.comment+0xb7>
      43: 00 67 65                     	add	byte ptr [edi + 101], ah
      46: 74 53                        	je	83 <.comment+0x9b>
      48: 53                           	push	ebx
      49: 00 70 72                     	add	byte ptr [eax + 114], dh
      4c: 69 6e 74 66 00 70 75         	imul	ebp, dword ptr [esi + 116], 1970274406
      53: 74 73                        	je	115 <.comment+0xc8>
      55: 00 5f 69                     	add	byte ptr [edi + 105], bl
      58: 6e                           	outsb	dx, byte ptr [esi]
      59: 74 65                        	je	101 <.comment+0xc0>
      5b: 72 6e                        	jb	110 <.comment+0xcb>
      5d: 61                           	popal
      5e: 6c                           	insb	byte ptr es:[edi], dx
      5f: 5f                           	pop	edi
      60: 73 74                        	jae	116 <.comment+0xd6>
      62: 64 69 6e 00 63 6c 65 61      	imul	ebp, dword ptr fs:[esi], 1634036835
      6a: 72 65                        	jb	101 <.comment+0xd1>
      6c: 72 72                        	jb	114 <.comment+0xe0>
      6e: 00 66 63                     	add	byte ptr [esi + 99], ah
      71: 6c                           	insb	byte ptr es:[edi], dx
      72: 6f                           	outsd	dx, dword ptr [esi]
      73: 73 65                        	jae	101 <.comment+0xda>
      75: 00 66 65                     	add	byte ptr [esi + 101], ah
      78: 6f                           	outsd	dx, dword ptr [esi]
      79: 66 00 66 65                  	add	byte ptr [esi + 101], ah
      7d: 72 72                        	jb	114 <.comment+0xf1>
      7f: 6f                           	outsd	dx, dword ptr [esi]
      80: 72 00                        	jb	0 <.comment+0x82>
      82: 66 6f                        	outsw	dx, word ptr [esi]
      84: 70 65                        	jo	101 <.comment+0xeb>
      86: 6e                           	outsb	dx, byte ptr [esi]
      87: 00 66 70                     	add	byte ptr [esi + 112], ah
      8a: 72 69                        	jb	105 <.comment+0xf5>
      8c: 6e                           	outsb	dx, byte ptr [esi]
      8d: 74 66                        	je	102 <.comment+0xf5>
      8f: 00 66 70                     	add	byte ptr [esi + 112], ah
      92: 75 74                        	jne	116 <.comment+0x108>
      94: 63 00                        	arpl	word ptr [eax], ax
      96: 66 70 75                     	jo	117 <.comment+0x10e>
      99: 74 73                        	je	115 <.comment+0x10e>
      9b: 00 67 65                     	add	byte ptr [edi + 101], ah
      9e: 74 63                        	je	99 <.comment+0x103>
      a0: 00 67 65                     	add	byte ptr [edi + 101], ah
      a3: 74 63                        	je	99 <.comment+0x108>
      a5: 68 61 72 00 67               	push	1728082529
      aa: 65 74 73                     	je	115 <.comment+0x120>
      ad: 00 67 65                     	add	byte ptr [edi + 101], ah
      b0: 74 73                        	je	115 <.comment+0x125>
      b2: 5f                           	pop	edi
      b3: 73 00                        	jae	0 <.comment+0xb5>
      b5: 70 65                        	jo	101 <.comment+0x11c>
      b7: 72 72                        	jb	114 <.comment+0x12b>
      b9: 6f                           	outsd	dx, dword ptr [esi]
      ba: 72 00                        	jb	0 <.comment+0xbc>
      bc: 70 75                        	jo	117 <.comment+0x133>
      be: 74 63                        	je	99 <.comment+0x123>
      c0: 68 61 72 00 73               	push	1929409121
      c5: 70 72                        	jo	114 <.comment+0x139>
      c7: 69 6e 74 66 00 73 74         	imul	ebp, dword ptr [esi + 116], 1953693798
      ce: 64 69 6e 00 73 74 72 6c      	imul	ebp, dword ptr fs:[esi], 1819440243
      d6: 65 6e                        	outsb	dx, byte ptr gs:[esi]
      d8: 00 76 73                     	add	byte ptr [esi + 115], dh
      db: 6e                           	outsb	dx, byte ptr [esi]
      dc: 70 72                        	jo	114 <.comment+0x150>
      de: 69 6e 74 66 00 76 73         	imul	ebp, dword ptr [esi + 116], 1937113190
      e5: 70 72                        	jo	114 <.comment+0x159>
      e7: 69 6e 74 66 00 6d 65         	imul	ebp, dword ptr [esi + 116], 1701642342
      ee: 6d                           	insd	dword ptr es:[edi], dx
      ef: 73 65                        	jae	101 <.comment+0x156>
      f1: 74 00                        	je	0 <.comment+0xf3>
      f3: 6d                           	insd	dword ptr es:[edi], dx
      f4: 65 6d                        	insd	dword ptr es:[edi], dx
      f6: 63 70 79                     	arpl	word ptr [eax + 121], si
      f9: 00 5f 63                     	add	byte ptr [edi + 99], bl
      fc: 74 79                        	je	121 <.comment+0x177>
      fe: 70 65                        	jo	101 <.comment+0x165>
     100: 00 6d 65                     	add	byte ptr [ebp + 101], ch
     103: 6d                           	insd	dword ptr es:[edi], dx
     104: 63 6d 70                     	arpl	word ptr [ebp + 112], bp
     107: 00 73 74                     	add	byte ptr [ebx + 116], dh
     10a: 72 63                        	jb	99 <.comment+0x16f>
     10c: 61                           	popal
     10d: 73 65                        	jae	101 <.comment+0x174>
     10f: 63 6d 70                     	arpl	word ptr [ebp + 112], bp
     112: 00 73 74                     	add	byte ptr [ebx + 116], dh
     115: 72 63                        	jb	99 <.comment+0x17a>
     117: 61                           	popal
     118: 74 00                        	je	0 <.comment+0x11a>
     11a: 73 74                        	jae	116 <.comment+0x190>
     11c: 72 63                        	jb	99 <.comment+0x181>
     11e: 6d                           	insd	dword ptr es:[edi], dx
     11f: 70 00                        	jo	0 <.comment+0x121>
     121: 73 74                        	jae	116 <.comment+0x197>
     123: 72 63                        	jb	99 <.comment+0x188>
     125: 70 79                        	jo	121 <.comment+0x1a0>
     127: 00 73 74                     	add	byte ptr [ebx + 116], dh
     12a: 72 74                        	jb	116 <.comment+0x1a0>
     12c: 6f                           	outsd	dx, dword ptr [esi]
     12d: 6b 00                        	<unknown>
