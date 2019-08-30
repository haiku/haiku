/*
 * Copyright 2003, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MISC_H
#define MISC_H

#include <SupportDefs.h>

#include "String.h"

// min and max
// We don't want to include <algobase.h> otherwise we also get <iostream.h>
// and other undesired things.
template<typename C>
static inline C min(const C &a, const C &b) { return (a < b ? a : b); }
template<typename C>
static inline C max(const C &a, const C &b) { return (a > b ? a : b); }

// find last (most significant) set bit
static inline
int
fls(uint32 value)
{
	if (!value)
		return -1;
	int index = 0;
#define HAND_OPTIMIZED_FLS	1
#if !HAND_OPTIMIZED_FLS
// This is the algorithm in its pure form.
	const uint32 masks[] = {
		0xffff0000,
		0xff00ff00,
		0xf0f0f0f0,
		0xcccccccc,
		0xaaaaaaaa,
	};
	int range = 16;
	for (int i = 0; i < 5; i++) {
		if (value & masks[i]) {
			index += range;
			value &= masks[i];
		}
		range /= 2;
	}
#else	// HAND_OPTIMIZED_FLS
// This is how the compiler should optimize it for us: Unroll the loop and
// inline the masks.
	// 0: 0xffff0000
	if (value & 0xffff0000) {
		index += 16;
		value &= 0xffff0000;
	}
	// 1: 0xff00ff00
	if (value & 0xff00ff00) {
		index += 8;
		value &= 0xff00ff00;
	}
	// 2: 0xf0f0f0f0
	if (value & 0xf0f0f0f0) {
		index += 4;
		value &= 0xf0f0f0f0;
	}
	// 3: 0xcccccccc
	if (value & 0xcccccccc) {
		index += 2;
		value &= 0xcccccccc;
	}
	// 4: 0xaaaaaaaa
	if (value & 0xaaaaaaaa)
		index++;
#endif	// HAND_OPTIMIZED_FLS
	return index;
}

// node_child_hash
static inline
uint32
node_child_hash(uint64 id, const char *name)
{
	return uint32(id & 0xffffffff) ^ string_hash(name);
}

#endif	// MISC_H
