/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_ALTCODEPATCH_H
#define _KERNEL_ARCH_x86_ALTCODEPATCH_H


#include <arch/x86/arch_kernel.h>


#define ASM_NOP1	.byte 0x90
#define ASM_NOP2	.byte 0x66, 0x90
#define ASM_NOP3	.byte 0x0f, 0x1f, 0x00
#define ASM_NOP4	.byte 0x0f, 0x1f, 0x40, 0x00
#define ASM_NOP5	.byte 0x0f, 0x1f, 0x44, 0x00, 0x00
#define ASM_NOP6	.byte 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00
#define ASM_NOP7	.byte 0x0f, 0x1f, 0x80, 0x00, 0x00, 0x00, 0x00
#define ASM_NOP8	.byte 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00
#define ASM_NOP9	.byte 0x66, 0x0f, 0x1f, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00


#define ALTCODEPATCH_TAG_STAC		1
#define ALTCODEPATCH_TAG_CLAC		2
#define ALTCODEPATCH_TAG_XSAVE		3
#define ALTCODEPATCH_TAG_XRSTOR		4


#ifdef _ASSEMBLER

#define CODEPATCH_START	990:
#define CODEPATCH_END(tag)	991:	\
	.pushsection	.altcodepatch, "a"	;	\
	.long			(990b - KERNEL_LOAD_BASE)	;	\
	.short			(991b - 990b)		;	\
	.short			tag				;	\
	.popsection

#define ASM_STAC	CODEPATCH_START \
					ASM_NOP3	;	 \
					CODEPATCH_END(ALTCODEPATCH_TAG_STAC)

#define ASM_CLAC	CODEPATCH_START \
					ASM_NOP3	;	\
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
					STRINGIFY(ASM_NOP3)	"\n" \
					CODEPATCH_END(ALTCODEPATCH_TAG_STAC)

#define ASM_CLAC	CODEPATCH_START \
					STRINGIFY(ASM_NOP3)	"\n"\
					CODEPATCH_END(ALTCODEPATCH_TAG_CLAC)


void arch_altcodepatch_replace(uint16 tag, void* newcodepatch, size_t length);


#endif

#endif	/* _KERNEL_ARCH_x86_ALTCODEPATCH_H */
