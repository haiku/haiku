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
	// Core EFI partition types
	{{0xC12A7328, 0xF81F, 0x11D2, 0xBA4B00A0C93EC93BLL}, "EFI system data"},
	{{0x21686148, 0x6449, 0x6E6F, 0x744E656564454649LL}, "BIOS boot data"},
	{{0x024DEE41, 0x33E7, 0x11D3, 0x9D690008C781F39FLL}, "MBR partition nest"},
	// Haiku partition
	{{0x42465331, 0x3BA3, 0x10F1, 0x802A4861696B7521LL}, BFS_NAME},
	// Linux partitions
	{{0x0FC63DAF, 0x8483, 0x4772, 0x8E793D69D8477DE4LL}, "Linux data"},
	{{0xE6D6D379, 0xF507, 0x44C2, 0xA23C238F2A3DF928LL}, "Linux data (LVM)"},
	{{0xA19D880F, 0x05FC, 0x4D3B, 0xA006743F0F84911ELL}, "Linux data (RAID)"},
	{{0x0657FD6D, 0xA4AB, 0x43C4, 0x84E50933C84B4F4FLL}, "Linux swap"},
	// Windows partitions
	{{0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C068B6B72699C7LL}, "Windows data"},
	// Apple partitions
	{{0x426F6F74, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple boot"},
	{{0x48465300, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple data (HFS+)"},
	{{0x55465300, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple data (UFS)"},
	{{0x52414944, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple RAID"},
	{{0x52414944, 0x5F4F, 0x11AA, 0xAA1100306543ECACLL}, "Apple RAID, offline"},
	// ChromeOS partitions
	{{0xFE3A2A5D, 0x4F32, 0x41A7, 0xB725ACCC3285A309LL}, "ChromeOS kernel"},
	{{0x3CB8E202, 0x3B7E, 0x47DD, 0x8A3C7FF2A13CFCECLL}, "ChromeOS rootfs"},
	// BSD partitions
	{{0x83BD6B9D, 0x7F41, 0x11DC, 0xBE0B001560B84F0FLL}, "FreeBSD boot"},
	{{0x516E7CB4, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL}, "FreeBSD data"},
	{{0x516E7CB6, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL}, "FreeBSD data (UFS)"},
	{{0x516E7CBA, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL}, "FreeBSD data (ZFS)"},
	{{0x516E7CB5, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL}, "FreeBSD swap"},
	{{0x85D5E45E, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD boot"},
	{{0x85D5E45A, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD data"},
	{{0x85D5E45B, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD swap"},
	{{0x49F48D5A, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD data (ffs)"},
	{{0x49F48D82, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD data (lfs)"},
	{{0x49F48D32, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD swap"},
	{{0x824CC7A0, 0x36A8, 0x11E3, 0x890A952519AD3F61LL}, "OpenBSD data"},
	// Other
	{{0xCEF5A9AD, 0x73BC, 0x4601, 0x89F3CDEEEEE321A1LL}, "QNX data"},
	{{0x6A898CC3, 0x1DD2, 0x11B2, 0x99A6080020736631LL}, "Solaris data (ZFS)"}
};


#endif	// GPT_KNOWN_GUIDS_H
