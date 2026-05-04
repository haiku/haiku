/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_ALTCODEPATCH_H
#define _KERNEL_ARCH_x86_ALTCODEPATCH_H


#include <arch/x86/arch_kernel.h>


#define X86_NOP1	0x90
#define X86_NOP2	0x66, 0x90
#define X86_NOP3	0x0f, 0x1f, 0x00
#define X86_NOP4	0x0f, 0x1f, 0x40, 0x00
#define X86_NOP5	0x0f, 0x1f, 0x44, 0x00, 0x00
#define X86_NOP6	0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00
#define X86_NOP7	0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00
#define X86_NOP8	0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00
#define X86_NOP9	0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00


#define ALTCODEPATCH_TAG_STAC		1
#define ALTCODEPATCH_TAG_CLAC		2
#define ALTCODEPATCH_TAG_XSAVE		3
#define ALTCODEPATCH_TAG_XRSTOR		4
#define ALTCODEPATCH_TAG_CLEAR_FPU	5


#ifdef _ASSEMBLER

#define CODEPATCH_START	990:
#define CODEPATCH_END(tag)	991:	\
	.pushsection	.altcodepatch, "a"	;	\
	.long			(990b - KERNEL_LOAD_BASE)	;	\
	.short			(991b - 990b)		;	\
	.short			tag				;	\
	.popsection

#define ASM_STAC	CODEPATCH_START \
					.byte X86_NOP3	;	 \
					CODEPATCH_END(ALTCODEPATCH_TAG_STAC)

#define ASM_CLAC	CODEPATCH_START \
					.byte X86_NOP3	;	\
					CODEPATCH_END(ALTCODEPATCH_TAG_CLAC)

#else

#define _STRINGIFY(x...)	#x
#define STRINGIFY(x...)		_STRINGIFY(x)

#define CODEPATCH_START	"990:	\n"
#define CODEPATCH_END(tag)	"991:	\n"	\
	".pushsection	.altcodepatch, \"a\"	\n" \
	".long			(990b - " STRINGIFY(KERNEL_LOAD_BASE) ")	\n" \
	".short			(991b - 990b)		\n" \
	".short			" STRINGIFY(tag)	"	\n" \
	".popsection"

#define ASM_STAC	CODEPATCH_START \
					STRINGIFY(.byte X86_NOP3)	"\n" \
					CODEPATCH_END(ALTCODEPATCH_TAG_STAC)

#define ASM_CLAC	CODEPATCH_START \
					STRINGIFY(.byte X86_NOP3)	"\n"\
					CODEPATCH_END(ALTCODEPATCH_TAG_CLAC)


void arch_altcodepatch_replace(uint16 tag, void* newcodepatch, size_t length);


#endif

#endif	/* _KERNEL_ARCH_x86_ALTCODEPATCH_H */
