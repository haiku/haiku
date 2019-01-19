/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2019, Rob Gill, rrobgill@protonmail.com
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
	// Microsoft partitions
	{{0xEBD0A0A2, 0xB9E5, 0x4433, 0x87C068B6B72699C7LL}, "Windows data"},
	{{0xE3C9E316, 0x0B5C, 0x4DB8, 0x817DF92DF00215AELL}, "Microsoft Reserved"},
	{{0xDE94BBA4, 0x06D1, 0x4D40, 0xA16ABFD50179D6ACLL}, "Microsoft Recovery"},
	{{0x5808C8AA, 0x7E8F, 0x42E0, 0x85D2E1E90434CFB3LL},
		"Microsoft LDM metadata"},
	{{0xAF9B60A0, 0x1431, 0x4F62, 0xBC683311714A69ADLL},
		"Microsoft LDM data"},
	// Apple partitions
	{{0x426F6F74, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple boot"},
	{{0x48465300, 0x0000, 0x11AA, 0xAA1100306543ECACLL}, "Apple data (HFS+)"},
	{{0x4C616265, 0x6C00, 0x11AA, 0xAA1100306543ECACLL}, "Apple data"},
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
	{{0x516E7CB8, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL},
		"FreeBSD data (vinum)"},
	{{0x516E7CB5, 0x6ECF, 0x11D6, 0x8FF800022D09712BLL}, "FreeBSD swap"},
	{{0x85D5E45E, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD boot"},
	{{0x85D5E45A, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD data"},
	{{0x85D5E45B, 0x237C, 0x11E1, 0xB4B3E89A8F7FC3A7LL}, "MidnightBSD swap"},
	{{0x2DB519C4, 0xB10F, 0x11DC, 0xB99B0019D1879648LL},
		"NetBSD data (ccd)"},
	{{0x2DB519EC, 0xB10F, 0x11DC, 0xB99B0019D1879648LL},
		"NetBSD data (encrypted)"},
	{{0x49F48D5A, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD data (ffs)"},
	{{0x49F48D82, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD data (lfs)"},
	{{0x49F48D32, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD swap"},
	{{0x49F48DAA, 0xB10E, 0x11DC, 0xB99B0019D1879648LL}, "NetBSD RAID"},
	{{0x824CC7A0, 0x36A8, 0x11E3, 0x890A952519AD3F61LL}, "OpenBSD data"},
	{{0x9D087404, 0x1CA5, 0x11DC, 0x881701301BB8A9F5LL},
		"DragonFlyBSD data {l32}"},
	{{0x3D48CE54, 0x1D16, 0x11DC, 0x869601301BB8A9F5LL},
		"DragonFlyBSD data {l64)"},
	{{0xBD215AB2, 0x1D16, 0x11DC, 0x869601301BB8A9F5LL},
		"DragonFlyBSD data (legacy)"},
	{{0xDBD5211B, 0x1CA5, 0x11DC, 0x881701301BB8A9F5LL},
		"DragonFlyBSD data (ccd)"},
	{{0x61DC63AC, 0x6E38, 0x11DC, 0x851301301BB8A9F5LL},
		"DragonFlyBSD data (hammer)"},
	{{0x5CBB9AD1, 0x862D, 0x11DC, 0xA94D01301BB8A9F5LL},
		"DragonFlyBSD data (hammer2)"},
	{{0x9D58FDBD, 0x1CA5, 0x11DC, 0x881701301BB8A9F5LL},
		"DragonFlyBSD swap"},
	{{0x9D94CE7C, 0x1CA5, 0x11DC, 0x881701301BB8A9F5LL},
		"DragonFlyBSD data (ufs)"},
	{{0x9DD4478F, 0x1CA5, 0x11DC, 0x881701301BB8A9F5LL},
		"DragonFlyBSD data (vinum)"},
	// Freedesktop
	{{0x44479540, 0xF297, 0x41B2, 0x9AF7D131D5F0458ALL},
		"Freedesktop x86 root"},
	{{0x4F68BCE3, 0xE8CD, 0x4DB1, 0x96E7FBCAF984B709LL},
		"Freedesktop x86_64 root"},
	{{0x69DAD710, 0x2CE4, 0x4E3C, 0xB16C21A1D49ABED3LL},
		"Freedesktop Arm root"},
	{{0xB921B045, 0x1DF0, 0x41C3, 0xAF444C6F280D3FAELL},
		"Freedesktop Arm64 root"},
	{{0x993D8D3D, 0xF80E, 0x4225, 0x855A9DAF8ED7EA97LL},
		"Freedesktop IA-64 root"},
	{{0xD13C5D3B, 0xB5D1, 0x422A, 0xB29F9454FDC89D76LL},
		"Freedesktop x86 root verity"},
	{{0x2C7357ED, 0xEBD2, 0x46D9, 0xAEC123D437EC2BF5LL},
		"Freedesktop x86-64 root verity"},
	{{0x7386CDF2, 0x203C, 0x47A9, 0xA498F2ECCE45A2D6LL},
		"Freedesktop Arm root verity"},
	{{0xDF3300CE, 0xD69F, 0x4C92, 0x978C9BFB0F38D820LL},
		"Freedesktop Arm64 root verity"},
	{{0x86ED10D5, 0xB607, 0x45BB, 0x8957D350F23D0571LL},
		"Freedesktop IA-64 root verity"},
	{{0x933AC7E1, 0x2EB4, 0x4F13, 0xB8440E14E2AEF915LL},
		"Freedesktop home"},
	{{0x3B8F8425, 0x20E0, 0x4F3B, 0x907F1A25A76F98E8LL},
		"Freedesktop server data"},
	// VMware
	{{0xAA31E02A, 0x400F, 0x11DB, 0x9590000C2911D1B8LL}, "VMware (vmfs)"},
	{{0x9D275380, 0x40AD, 0x11DB, 0xBF97000C2911D1B8LL}, "VMware (diag)"},
	{{0x9198EFFC, 0x31C0, 0x11DB, 0x8F78000C2911D1B8LL}, "VMware (resereved)"},
	{{0x381CFCCC, 0x7288, 0x11E0, 0x92EE000C2911D0B2LL}, "VMware (vsan)"},
	// Other
	{{0x9E1A2D38, 0xC612, 0x4316, 0xAA268B49521E5A8BLL}, "IBM prep-boot"},
	{{0xBC13C2FF, 0x59E6, 0x4262, 0xA352B275FD6F7172LL}, "Systemd bootloader"},
	{{0xCEF5A9AD, 0x73BC, 0x4601, 0x89F3CDEEEEE321A1LL}, "QNX data"},
	{{0x6A898CC3, 0x1DD2, 0x11B2, 0x99A6080020736631LL}, "Solaris data (ZFS)"}
};


#endif	// GPT_KNOWN_GUIDS_H
