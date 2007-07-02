/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUID_H
#define GUID_H


#include <SupportDefs.h>


typedef struct guid {
	uint32	data1;
	uint16	data2;
	uint16	data3;
	uint8	data4[8];

	inline bool operator==(const guid &other) const;
	inline bool operator!=(const guid &other) const;
} _PACKED guid_t;


inline bool
guid_t::operator==(const guid_t &other) const
{
	return data1 == other.data1
		&& data2 == other.data2
		&& data3 == other.data3
		&& *(uint64 *)data4 == *(uint64 *)other.data4;
}


inline bool
guid_t::operator!=(const guid_t &other) const
{
	return data1 != other.data1
		|| data2 != other.data2
		|| data3 != other.data3
		|| *(uint64 *)data4 != *(uint64 *)other.data4;
}

#endif	/* GUID_H */

