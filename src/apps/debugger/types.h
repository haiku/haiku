/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPES_H
#define TYPES_H

#include <SupportDefs.h>


// target address type
typedef uint32 dwarf_addr_t;

// DWARF 32 or 64 offset/size types
typedef uint32 dwarf_off_t;
typedef uint32 dwarf_size_t;


#define DWARF_ADDRESS_MAX	0xffffffff


#endif	// TYPES_H
