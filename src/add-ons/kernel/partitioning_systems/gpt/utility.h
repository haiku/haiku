/*
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <SupportDefs.h>

#include "guid.h"


struct static_guid;

extern const guid_t kEmptyGUID;


void to_utf8(const uint16* from, size_t maxFromLength, char* to, size_t toSize);
const char* get_partition_type(const guid_t& guid);

#ifndef _BOOT_MODE
void to_ucs2(const char* from, size_t fromLength, uint16* to,
	size_t maxToLength);
bool get_guid_for_partition_type(const char* type, guid_t& guid);
#endif // !_BOOT_MODE


#endif	// UTILITY_H
