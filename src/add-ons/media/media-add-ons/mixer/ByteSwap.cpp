/*
 * Copyright 2003-2007 Marcus Overhagen
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ByteSwap.h"

#include <ByteOrder.h>
#include <MediaDefs.h>

#include "MixerDebug.h"


static void swap_float(void *buffer, size_t bytecount);
static void swap_int32(void *buffer, size_t bytecount);
static void swap_int16(void *buffer, size_t bytecount);
static void do_nothing(void *buffer, size_t bytecount);


ByteSwap::ByteSwap(uint32 format)
{
	switch (format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			fFunc = &swap_float;
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			fFunc = &swap_int32;
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			fFunc = &swap_int16;
			break;
		default:
			fFunc = &do_nothing;
			break;
	}
}


ByteSwap::~ByteSwap()
{
}


void
do_nothing(void *buffer, size_t bytecount)
{
}


#if __i386__
// #pragma mark - optimized for IA32 platform


void
swap_float(void *buffer, size_t bytecount)
{
	// XXX Should be optimized
	swap_data(B_FLOAT_TYPE, buffer, bytecount, B_SWAP_ALWAYS);
}


void
swap_int32(void *buffer, size_t bytecount)
{
	// XXX Should be optimized
	swap_data(B_INT32_TYPE, buffer, bytecount, B_SWAP_ALWAYS);
}


void
swap_int16(void *buffer, size_t bytecount)
{
	// GCC FAQ: To write an asm which modifies an input operand but does
	//          not output anything usable, specify that operand as an
	//          output operand outputting to an unused dummy variable.
	uint32 dummy1;
	uint32 dummy2;
	 // GCC is way too smart and will remove the complete asm statement
	 // if we do not specify it as __volatile__. Don't remove that!
	__asm__ __volatile__ (
	"pushl	%%ebx	\n\t"
	"movl	%%eax, %%ebx	\n\t"
	"movl	%%edx, %%eax	\n\t"
	"andl   $0xFFFFFFE0,%%eax	\n\t"
	"pushl	%%eax	\n\t"
	".L_swap_loop: \n\t"
		"rolw	$8,-32(%%ebx,%%eax)	\n\t"
		"rolw	$8,-30(%%ebx,%%eax)	\n\t"
		"rolw	$8,-28(%%ebx,%%eax)	\n\t"
		"rolw	$8,-26(%%ebx,%%eax)	\n\t"
		"rolw	$8,-24(%%ebx,%%eax)	\n\t"
		"rolw	$8,-22(%%ebx,%%eax)	\n\t"
		"rolw	$8,-20(%%ebx,%%eax)	\n\t"
		"rolw	$8,-18(%%ebx,%%eax)	\n\t"
		"rolw	$8,-16(%%ebx,%%eax)	\n\t"
		"rolw	$8,-14(%%ebx,%%eax)	\n\t"
		"rolw	$8,-12(%%ebx,%%eax)	\n\t"
		"rolw	$8,-10(%%ebx,%%eax)	\n\t"
		"rolw	$8,-8(%%ebx,%%eax)	\n\t"
		"rolw	$8,-6(%%ebx,%%eax)	\n\t"
		"rolw	$8,-4(%%ebx,%%eax)	\n\t"
		"rolw	$8,-2(%%ebx,%%eax)	\n\t"
		"subl	$32,%%eax	\n\t"
		"jnz	.L_swap_loop	\n\t"
	"popl	%%eax	\n\t"
	"addl	%%eax,%%ebx	\n\t"
	"andl	$0x1F,%%edx	\n\t"
	"jz		.L_swap_end	\n\t"
	"addl	%%ebx, %%edx	\n\t"
	".L_swap_loop_2: \n\t"
		"rolw	$8,(%%ebx)	\n\t"
		"addl	$2,%%ebx	\n\t"
		"cmpl	%%edx,%%ebx	\n\t"
		"jne	.L_swap_loop_2 \n\t"
	".L_swap_end:	\n\t"
	"popl	%%ebx \n\t"
	: "=a" (dummy1), "=d" (dummy2)
	: "d" (bytecount), "a" (buffer)
	: "cc", "memory");
}


#else	// !__i386__
// #pragma mark - generic versions


void
swap_float(void *buffer, size_t bytecount)
{
	swap_data(B_FLOAT_TYPE, buffer, bytecount, B_SWAP_ALWAYS);
}


void
swap_int32(void *buffer, size_t bytecount)
{
	swap_data(B_INT32_TYPE, buffer, bytecount, B_SWAP_ALWAYS);
}


void
swap_int16(void *buffer, size_t bytecount)
{
	swap_data(B_INT16_TYPE, buffer, bytecount, B_SWAP_ALWAYS);
}


#endif
