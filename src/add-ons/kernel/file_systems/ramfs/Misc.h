/*
 * Copyright 2003, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MISC_H
#define MISC_H

#include <SupportDefs.h>
#include <util/StringHash.h>

// min and max
// We don't want to include <algobase.h> otherwise we also get <iostream.h>
// and other undesired things.
template<typename C>
static inline C min(const C &a, const C &b) { return (a < b ? a : b); }
template<typename C>
static inline C max(const C &a, const C &b) { return (a > b ? a : b); }

// node_child_hash
static inline
uint32
node_child_hash(uint64 id, const char *name)
{
	return uint32(id & 0xffffffff) ^ hash_hash_string(name);
}

#endif	// MISC_H
