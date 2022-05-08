/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include <asm_defs.h>


	// the following code is based on ARM ARM
	// section B2.2.4, Cache maintenance functionality

FUNCTION(clean_dcache_all):
	dmb
	stmfd	sp!, {r4-r11, lr}

	// Query the number of cache levels to be cleaned
	// this is encoded in LoC which is bits 26..24 of CLIDR
	mrc		p15, 1, r0, c0, c0, 1
	mov		r3, r0, lsr #23
	ands	r3, r3, #7 << 1
	beq		cleaning_done

	// Start cleaning at cache level 0
	mov		r10, #0
clean_levels_loop:
	// Get type of current cache level
	add		r2, r10, r10, lsr #1
	mov		r1, r0, lsr r2
	and		r1, r1, #7

	// Skip this level if cache type is less than 2 i.e. no cache or only i-cache
	cmp		r1, #2
	blt		clean_next_level

	// Select current level in Cache Size selection register
	mcr		p15, 2, r10, c0, c0, 0
	isb

	// Read Cache Size ID for the current level
	mrc		p15, 1, r1, c0, c0, 0

	// Decode LineSize from bits[2:0]
	and		r2, r1, #7
	add		r2, r2, #4

	// Decode Associativity from bits[12:3]
	movw	r4, #0x3ff
	ands	r4, r4, r1, lsr #3
	clz		r5, r4

	// Decode NumSets from bits[27:13]
	movw	r7, #0x7fff
	ands	r7, r7, r1, lsr #13

clean_way_loop:
	mov		r9, r7
clean_index_loop:
	orr		r11, r10, r4, lsl r5
	orr		r11, r11, r9, lsl r2

	// Clean and Invalidate by set/way
	mcr		p15, 0, r11, c7, c14, 2

	subs	r9, r9, #1
	bge		clean_index_loop

	subs	r4, r4, #1
	bge		clean_way_loop
clean_next_level:
	add		r10, r10, #2
	cmp		r3, r10
	bgt		clean_levels_loop
cleaning_done:
	dsb
	isb
	ldmfd	sp!, {r4-r11, lr}
	bx		lr
FUNCTION_END(clean_dcache_all)


FUNCTION(invalidate_icache_all):
	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0
	dsb
	isb
	bx		lr
FUNCTION_END(invalidate_icache_all)
