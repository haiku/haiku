/*
 * Copyright (c) 2018-2026 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

	.globl	test1_begin, test1_end
	.globl	test2_begin, test2_end
	.globl	test3_begin, test3_end
	.globl	test4_begin, test4_end
	.globl	test5_begin, test5_end
	.globl	test6_begin, test6_end
	.globl	test7_begin, test7_end
	.globl	test8_begin, test8_end
	.globl	test9_begin, test9_end
	.globl	test10_begin, test10_end
	.globl	test11_begin, test11_end
	.globl	test12_begin, test12_end
	.globl	test13_begin, test13_end
	.globl	test14_begin, test14_end
	.globl	test_64bit_15_begin, test_64bit_15_end
	.globl	test_64bit_16_begin, test_64bit_16_end
	.globl	test_64bit_17_begin, test_64bit_17_end
	.globl	test_64bit_18_begin, test_64bit_18_end
	.globl	test_64bit_19_begin, test_64bit_19_end
	.globl	test_64bit_20_begin, test_64bit_20_end
	.globl	test_64bit_21_begin, test_64bit_21_end
	.text
	.code64

#define TEST_END	\
	movq	$0xFFFFFFFFFFFFFFFF,%rcx;	\
	rdmsr	;

	.align	64
test1_begin:
	movq	$0x1000,%rax
	movq	$0x1000,%rbp

	movq	$0x1000,(%rax)

	movq	$1,%r11
	movq	$0x2000,(%rax,%r11,8)

	movq	(%rbp),%r8
	movq	8(%rbp),%rbx
	addq	%rbx,%r8
	movq	%r8,(%rbp)
	movb	$4,(%rbp)

	TEST_END
test1_end:

	.align	64
test2_begin:
	movq	$0x1000,%rax

	movq	$0x1000,(%rax)
	movq	$0x00FF,%rbx
	orb	%bl,(%rax)
	movq	$0x0400,%rcx
	orw	%cx,(%rax)

	movq	$0x0200,%rcx
	orq	(%rax),%rcx
	movq	%rcx,(%rax)

	TEST_END
test2_end:

	.align	64
test3_begin:
	movq	$0x1000,%rax

	movq	$0x1FFF,(%rax)
	movq	$0x1FF0,%rbx
	andq	%rbx,(%rax)
	movq	$0x10C1,%rcx
	andb	%cl,(%rax)

	TEST_END
test3_end:

	.align	64
test4_begin:
	movq	$0x1000,%rax

	movq	$0x1FFF,(%rax)
	movq	$0x1FF0,%rbx
	xorq	%rbx,(%rax)
	movq	$0x10C0,%rcx
	xorw	%cx,(%rax)

	TEST_END
test4_end:

	.align	64
test5_begin:
	movq	$0xFFFFFFFF00001000,%rax

	movq	$0x1FFF,(%eax)
	movb	$0,(%eax,%ebx,1)

	TEST_END
test5_end:

	.align	64
test6_begin:
	movq	$0xFFA0,%rax
	movabs	%rax,0x1000

	movabs	0x1000,%al
	orb	$0x0B,%al
	movabs	%al,0x1000

	TEST_END
test6_end:

	.align	64
test7_begin:
	movq	$0x56,%rax

	movq	$1,%rcx
	movq	$0x1000,%rdi
	rep	stosb

	movq	$0x1234,%rax
	stosw

	TEST_END
test7_end:

	.align	64
test8_begin:
	movq	$0x1008,%rsi
	movq	$0x12345678,(%rsi)

	movq	$0x1000,%rdi

	lodsw
	movw	%ax,(%rdi)
	addq	$2,%rdi

	lodsb
	movb	%al,(%rdi)
	addq	$1,%rdi

	lodsb
	movb	%al,(%rdi)
	addq	$2,%rdi

	TEST_END
test8_end:

	.align	64
test9_begin:
	movq	$0x1000,%rax

	movq	$0x12345678,8(%rax)

	movq	$0x1008,%rsi
	movq	$0x1000,%rdi

	movq	$4,%rcx
	rep movsb

	movq	$2,%rcx
	rep movsw

	TEST_END
test9_end:

	.align	64
test10_begin:
	movq	$0x1000,%rax
	movq	$0x12345678,(%rax)

	movq	$0xFFFFFFFFFFFFFFFF,%rbx
	movzbl	(%rax),%ebx
	movq	%rbx,(%rax)

	TEST_END
test10_end:

	.align	64
test11_begin:
	movq	$0x1000,%rax
	movq	$0x12345678,(%rax)

	movq	$0xFFFFFFFFFFFFFFFF,%rbx
	movzwq	(%rax),%rbx
	movq	%rbx,(%rax)

	TEST_END
test11_end:

	.align	64
test12_begin:
	movq	$0x1000,%rax
	movq	$0xFFFFFFFFF2345678,(%rax)

	cmpb	$0x78,(%rax)
	jne	.L12_failure
	cmpb	$0x77,(%rax)
	jl	.L12_failure
	cmpb	$0x79,(%rax)
	jg	.L12_failure

	cmpw	$0x5678,(%rax)
	jne	.L12_failure
	cmpw	$0x5677,(%rax)
	jl	.L12_failure
	cmpw	$0x5679,(%rax)
	jg	.L12_failure

	cmpl	$0xF2345678,(%rax)
	jne	.L12_failure
	cmpl	$0xF2345677,(%rax)
	jl	.L12_failure
	cmpl	$0xF2345679,(%rax)
	jg	.L12_failure

	cmpq	$0xFFFFFFFFF2345678,(%rax)
	jne	.L12_failure
	cmpq	$0xFFFFFFFFF2345677,(%rax)
	jl	.L12_failure
	cmpq	$0xFFFFFFFFF2345679,(%rax)
	jg	.L12_failure

.L12_success:
	movq	$1,(%rax)
	TEST_END
.L12_failure:
	movq	$0,(%rax)
	TEST_END
test12_end:

	.align	64
test13_begin:
	movq	$0x1000,%rax
	movq	$0x000000001000A0FF,(%rax)

	movq	$0xFFFF,%rcx
	subb	%cl,(%rax)

	movq	$0xA000,%rcx
	subw	%cx,(%rax)

	movq	$0x0000000F1000A0FF,%rcx
	subq	(%rax),%rcx

	movq	%rcx,(%rax)

	TEST_END
test13_end:

	.align	64
test14_begin:
	movq	$0x1000,%rax
	movq	$0xA0FF,(%rax)

	testb	$0x0F,(%rax)
	jz	.L14_failure

	testw	$0x0F00,(%rax)
	jnz	.L14_failure

	testl	$0xA000,(%rax)
	jz	.L14_failure

.L14_success:
	movq	$1,(%rax)
	TEST_END
.L14_failure:
	movq	$0,(%rax)
	TEST_END
test14_end:

	.align	64
test_64bit_15_begin:
	movq	$0x1000,%rax
	movq	$0x120000,%rbx
	movq	$0x003400,%rcx
	movq	$0x000056,%rdx

	xchgq	%rbx,(%rax)
	xchgw	(%rax),%cx
	xchgb	%dl,(%rax)

	TEST_END
test_64bit_15_end:

	.align	64
test_64bit_16_begin:
	movq	$0x1000,%rax
	movq	$0x000000,%rbx
	movq	$0x000000,%rcx
	movq	$0x000000,%rdx

	movq	$0x123456,(%rax)
	xchgq	%rbx,(%eax)
	movq	$0,(%rax)
	xchgq	%rbx,(%eax)

	TEST_END
test_64bit_16_end:

	.align	64
test_64bit_17_begin:
	movq	$0x1000,%rax
	movq	$0x4,%rcx

	/*
	 * REP is a no-op on MOV, so this should only write 0x4 once.
	 */
	.byte	0xF3 /* REP prefix */
	movq	%rcx,(%rax)

	TEST_END
test_64bit_17_end:

	.align	64
test_64bit_18_begin:
	movq	$0x1000,%rax
	movq	$-1,(%rax)

	cmpb	$-1,(%rax)
	jne	.L18_failure
	cmpw	$-1,(%rax)
	jne	.L18_failure
	cmpl	$-1,(%rax)
	jne	.L18_failure

.L18_success:
	movq	$1,(%rax)
	TEST_END
.L18_failure:
	movq	$0,(%rax)
	TEST_END
test_64bit_18_end:

	.align	64
test_64bit_19_begin:
	movq	$0x1000,%rax

	/* Test each operand size, and verify that BT does not modify memory. */
	movw	$0x8001,8(%rax)
	movl	$0x80000001,16(%rax)
	movabsq	$0x8000000000000001,%rbx
	movq	%rbx,24(%rax)

	btw	$0,8(%rax)
	jnc	.L19_failure
	btw	$1,8(%rax)
	jc	.L19_failure
	btw	$255,8(%rax)
	jnc	.L19_failure

	btl	$0,16(%rax)
	jnc	.L19_failure
	btl	$1,16(%rax)
	jc	.L19_failure
	btl	$255,16(%rax)
	jnc	.L19_failure

	btq	$0,24(%rax)
	jnc	.L19_failure
	btq	$1,24(%rax)
	jc	.L19_failure
	btq	$255,24(%rax)
	jnc	.L19_failure

	movzwq	8(%rax),%rcx
	cmpq	$0x8001,%rcx
	jne	.L19_failure
	movl	16(%rax),%ecx
	cmpl	$0x80000001,%ecx
	jne	.L19_failure
	movq	24(%rax),%rcx
	cmpq	%rbx,%rcx
	jne	.L19_failure

.L19_success:
	movq	$1,(%rax)
	TEST_END
.L19_failure:
	movq	$0,(%rax)
	TEST_END
test_64bit_19_end:

	.align	64
test_64bit_20_begin:
	movq	$0x5a,%rax
	movabsq	$0xdeadbeef00001000,%rdi
	movabsq	$0xfeedface00000001,%rcx

	/* addr32 rep stosb must update EDI/ECX and zero-extend both. */
	.byte	0x67
	rep stosb

	cmpq	$0x1001,%rdi
	jne	.L20_failure

.L20_success:
	movq	%rcx,0x1008
	TEST_END
.L20_failure:
	movq	$-1,0x1008
	TEST_END
test_64bit_20_end:

	.align	64
test_64bit_21_begin:
	movq	$0x1010,%rbx
	movb	$0x5a,(%rbx)
	movabsq	$0xdeadbeef00001010,%rsi
	movabsq	$0xfeedface00001000,%rdi

	/* addr32 movsb must update ESI/EDI and zero-extend both. */
	.byte	0x67
	movsb

	cmpq	$0x1011,%rsi
	jne	.L21_failure
	cmpq	$0x1001,%rdi
	jne	.L21_failure
	cmpb	$0x5a,0x1000
	jne	.L21_failure

.L21_success:
	movq	$1,0x1008
	TEST_END
.L21_failure:
	movq	$0,0x1008
	TEST_END
test_64bit_21_end:

/* -------------------------------------------------------------------------- */

	.globl	test_32bit_1_begin, test_32bit_1_end

#define TEST32_END	\
	rdmsr

	.code32

	.align	64
test_32bit_1_begin:
	movl	$0x1000,%esi
	movl	$0x1008,%edi

	movsl

	TEST32_END
test_32bit_1_end:

/* -------------------------------------------------------------------------- */

	.globl	test_16bit_1_begin, test_16bit_1_end
	.globl	test_16bit_2_begin, test_16bit_2_end
	.globl	test_16bit_3_begin, test_16bit_3_end
	.globl	test_16bit_4_begin, test_16bit_4_end
	.globl	test_16bit_5_begin, test_16bit_5_end
	.globl	test_16bit_6_begin, test_16bit_6_end
	.globl	test_16bit_7_begin, test_16bit_7_end

#define TEST16_END	\
	rdmsr

	.code16

	.align	64
test_16bit_1_begin:
	movw	$0x10f1,%bx
	movw	$0x123,%dx

	movb	%dl,(%bx)

	TEST16_END
test_16bit_1_end:

	.align	64
test_16bit_2_begin:
	movw	$0x10f1,%bx
	movw	$2,%di
	movw	$0x123,%dx

	movw	%dx,(%bx,%di)

	TEST16_END
test_16bit_2_end:

	.align	64
test_16bit_3_begin:
	movw	$0x10f1,%bp
	movw	$2,%si
	movw	$0x678,%dx

	movw	%dx,-2(%bp,%si)

	TEST16_END
test_16bit_3_end:

	.align	64
test_16bit_4_begin:
	movw	$0x10f0,%bp
	movw	$2,%si
	movw	$2+4+4,%di
	movw	$0xFFFF,%dx
	movl	$0x0001,%eax
	movl	$0x0010,%ebx
	movl	$0x1000,%ecx

	movw	%dx,4(%bp,%si)		/* 16bit opr 16bit adr */
	andl	%eax,4(%bp,%si)		/* 32bit opr 16bit adr */
	orw	%bx,4(%ebp,%esi)	/* 16bit opr 32bit adr */
	orl	%ecx,-4(%bp,%di)	/* 32bit opr 16bit adr, negative */

	TEST16_END
test_16bit_4_end:

	.align	64
test_16bit_5_begin:
	movb	$0x12,0x1234

	TEST16_END
test_16bit_5_end:

	.align	64
test_16bit_6_begin:
	movw	$0x1234,%bp
	movw	$4,%di
	movw	$0x1200,%bx
	movw	$0x0034,%cx

	xchgw	%bx,(%bp)
	xchgb	-4(%bp,%di),%cl

	TEST16_END
test_16bit_6_end:

	.align	64
test_16bit_7_begin:
	movw	$0x1240,%bx

	/* Test the default and overridden operand sizes in 16-bit mode. */
	movw	$0x8001,(%bx)
	movl	$0x80000001,4(%bx)

	btw	$0,(%bx)
	jnc	.L16_7_failure
	btw	$1,(%bx)
	jc	.L16_7_failure
	btw	$255,(%bx)
	jnc	.L16_7_failure

	btl	$0,4(%bx)
	jnc	.L16_7_failure
	btl	$1,4(%bx)
	jc	.L16_7_failure
	btl	$255,4(%bx)
	jnc	.L16_7_failure

	movw	(%bx),%cx
	cmpw	$0x8001,%cx
	jne	.L16_7_failure
	movl	4(%bx),%ecx
	cmpl	$0x80000001,%ecx
	jne	.L16_7_failure

.L16_7_success:
	movw	$1,0x1234
	TEST16_END
.L16_7_failure:
	movw	$0,0x1234
	TEST16_END
test_16bit_7_end:

