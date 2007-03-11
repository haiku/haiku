// hashes.h
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

#ifndef HASHES_H
#define HASHES_H

#include <SupportDefs.h>

/*
	\file hashes.h
	\brief Hash functions.

	key_offset_for_name() computes the maximal possible dir entry offset for
	given name, i.e. the offset that has the maximal generation number.
	Doing a rightmost close search for the respective key will produce
	the rightmost entry with the same hash value as the supplied name
	(if any). Walking backwards will find all other entries with that hash.
*/

typedef uint32 (*hash_function_t)(const signed char *, int);

uint32 keyed_hash(const signed char *msg, int len);
uint32 yura_hash(const signed char *msg, int len);
uint32 r5_hash(const signed char *msg, int len);

uint32 key_offset_for_name(hash_function_t hash, const char *name, int len);

#endif	// HASHES_H
