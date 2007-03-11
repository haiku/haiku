// endianess.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef ENDIANESS_H
#define ENDIANESS_H

#include <endian.h>
#include <SupportDefs.h>

/*!
	\file endianess.h
	\break Endianess conversion support functions.

	The functions le2h() and h2le() convert various integer types from
	little endian to host format and vice versa respectively.
*/

// swap_value
static inline
uint8
swap_value(uint8 v)
{
	return v;
}

// swap_value
static inline
int8
swap_value(int8 v)
{
	return v;
}

// swap_value
static inline
uint16
swap_value(uint16 v)
{
	return ((v & 0xff) << 8) | (v >> 8);
}

// swap_value
static inline
int16
swap_value(int16 v)
{
	return (int16)swap_value((uint16)v);
}

// swap_value
static inline
uint32
swap_value(uint32 v)
{
	return ((v & 0xff) << 24) | ((v & 0xff00) << 8) | ((v & 0xff0000) >> 8)
		   | (v >> 24);
}

// swap_value
static inline
int32
swap_value(int32 v)
{
	return (int32)swap_value((uint32)v);
}

// swap_value
static inline
uint64
swap_value(uint64 v)
{
	return (uint64(swap_value(uint32(v & 0xffffffffULL))) << 32)
		   | uint64(swap_value(uint32((v & 0xffffffff00000000ULL) >> 32)));
}

// swap_value
static inline
int64
swap_value(int64 v)
{
	return (int64)swap_value((uint64)v);
}

// le2h
template<typename T>
static inline
T
le2h(const T &v)
{
#if LITTLE_ENDIAN
	return v;
#else
	return swap_value(v);
#endif
}

// h2le
template<typename T>
static inline
T
h2le(const T &v)
{
#if LITTLE_ENDIAN
	return v;
#else
	return swap_value(v);
#endif
}

#endif	// ENDIANESS_H
