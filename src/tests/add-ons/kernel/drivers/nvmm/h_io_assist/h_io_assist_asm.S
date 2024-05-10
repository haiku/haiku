/*
 * Copyright (c) 2019-2021 Maxime Villard, m00nbsd.net
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
	.text
	.code64

#define TEST_END	\
	movq	$0xFFFFFFFFFFFFFFFF,%rcx;	\
	rdmsr	;

/*
 * IN
 */

	.align	64
test1_begin:
	movq	$0x1000,%rbx

	inb	$123
	movb	%al,(%rbx)
	incq	%rbx

	movq	$123,%rdx
	inb	%dx
	movb	%al,(%rbx)

	TEST_END
test1_end:

	.align	64
test2_begin:
	movq	$0x1000,%rbx

	inw	$123
	movw	%ax,(%rbx)
	addq	$2,%rbx

	movq	$123,%rdx
	inw	%dx
	movw	%ax,(%rbx)

	TEST_END
test2_end:

	.align	64
test3_begin:
	movq	$0x1000,%rbx

	inl	$123
	movl	%eax,(%rbx)
	addq	$4,%rbx

	movq	$123,%rdx
	inl	%dx
	movl	%eax,(%rbx)

	TEST_END
test3_end:

	.align	64
test4_begin:
	movq	$0x1000,%rdi
	movq	$5,%rcx

	movq	$123,%rdx
	rep
	insb

	TEST_END
test4_end:

	.align	64
test5_begin:
	movq	$0x1000,%rdi
	movq	$14,%rcx

	movq	$123,%rdx
	rep
	insw

	TEST_END
test5_end:

	.align	64
test6_begin:
	movq	$0x1000,%rdi
	movq	$7,%rcx

	movq	$123,%rdx
	rep
	insl

	TEST_END
test6_end:

/*
 * OUT
 */

	.align	64
test7_begin:
	movq	$0x1000,%rbx

	movb	(%rbx),%al
	outb	$123
	incq	%rbx

	movb	(%rbx),%al
	movq	$123,%rdx
	outb	%dx

	TEST_END
test7_end:

	.align	64
test8_begin:
	movq	$0x1000,%rbx

	movw	(%rbx),%ax
	outw	$123
	addq	$2,%rbx

	movw	(%rbx),%ax
	movq	$123,%rdx
	outw	%dx

	TEST_END
test8_end:

	.align	64
test9_begin:
	movq	$0x1000,%rbx

	movl	(%rbx),%eax
	outl	$123
	addq	$4,%rbx

	movl	(%rbx),%eax
	movq	$123,%rdx
	outl	%dx

	TEST_END
test9_end:

	.align	64
test10_begin:
	movq	$0x1000,%rsi
	movq	$5,%rcx

	movq	$123,%rdx
	rep
	outsb

	TEST_END
test10_end:

	.align	64
test11_begin:
	movq	$0x1000,%rsi
	movq	$8,%rcx

	movq	$123,%rdx
	rep
	outsw

	TEST_END
test11_end:

	.align	64
test12_begin:
	movq	$0x1000,%rsi
	movq	$7,%rcx

	movq	$123,%rdx
	rep
	outsl

	TEST_END
test12_end:
