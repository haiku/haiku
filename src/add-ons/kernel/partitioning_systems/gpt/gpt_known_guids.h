/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef GPT_KNOWN_GUIDS_H
#define GPT_KNOWN_GUIDS_H


#include <ByteOrder.h>

#include <disk_device_types.h>


struct guid;


struct static_guid {
	uint32	data1;
	uint16	data2;
	uint16	data3;
	uint64	data4;

	inline bool operator==(const guid& other) const;
	inline operator guid_t() const;
} _PACKED;


inline bool
static_guid::operator==(const guid_t& other) const
{
	return B_HOST_TO_LENDIAN_INT32(data1) == other.data1
		&& B_HOST_TO_LENDIAN_INT16(data2) == other.data2
		&& B_HOST_TO_LENDIAN_INT16(data3) == other.data3
		&& B_HOST_TO_BENDIAN_INT64(*(uint64*)&data4) == *(uint64*)other.data4;
			// the last 8 bytes are in big-endian order
}


inline
static_guid::operator guid_t() const
{
	guid_t guid;
	guid.data1 = B_HOST_TO_LENDIAN_INT32(data1);
	guid.data2 = B_HOST_TO_LENDIAN_INT16(data2);
	guid.data3 = B_HOST_TO_LENDIAN_INT16(data3);
	uint64 last = B_HOST_TO_BENDIAN_INT64(*(uint64*)&data4);
	memcpy(guid.data4, &last, sizeof(uint64));

	return guid;
}


const static struct type_map {
	static_guid	guid;
	const char*	type;
} kTypeMap[] = {
	{{0xC12A7328, 0xF81F, 0x11D2, 0xBA4B00A0C93EC93BLL}, "EFI System Data"},
	{{0x21686148, 0x6449, 0x6E6F, 0x744E656564454649LL}, "BIOS Boot Data"},
	{{0x024DEE41, 0x33E7, 0x11D3, 0x9D690008C781F39FLL}, "MBR Partition Nest"},
	{{0x42465331, 0x3BA3, 0x10F1, 0x802A4861696B7521LL}, BFS_NAME},
	{{0x0FC63DAF, 0x8483, 0x4772, 0x8E793D69D8477DE4LL}, "Linux File System"},
	{{0xA19D880F, 0x05FC, 0x4D3B, 0xA006743F0F84911ELL}, "Linux RAID"},
	{{0x0657FD6D, 0xA4AB, 0x43C4, 0x84E50933C84B4F4FLL}, "Linux Swap"},
	{{0xE6D6D379, 0xF507, 0x44C2, 0xA23C238F2A3DF928LL}, "Linux LVM"},
	{{0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C068B6B72699C7LL}, "Windows Data"},
	{{0x48465300, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "HFS+ File System"},
	{{0x55465300, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "UFS File System"},
	{{0x52414944, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple RAID"},
	{{0x52414944, 0x5F4F, 0x11AA, 0xAA1100306543ECACLL}, "Apple RAID, offline"}
};


#endif	// GPT_KNOWN_GUIDS_H
