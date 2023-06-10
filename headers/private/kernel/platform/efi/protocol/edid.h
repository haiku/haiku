/*
 * Copyright 2023 Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Distributed under terms of the MIT license.
 */

#pragma once


#include <efi/types.h>


#define EFI_EDID_DISCOVERED_PROTOCOL_GUID \
	{0x1c0c34f6, 0xd380, 0x41fa, {0xa0, 0x49, 0x8a, 0xd0, 0x6c, 0x1a, 0x66, 0xaa}}

#define EFI_EDID_ACTIVE_PROTOCOL_GUID \
	{0xbd8c1056, 0x9f36, 0x44ec, {0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86}}

#define EFI_EDID_OVERRIDE_PROTOCOL_GUID \
	{0x48ecb431, 0xfb72, 0x45c0, {0xa9, 0x22, 0xf4, 0x58, 0xfe, 0x04, 0x0b, 0xd5}}


struct efi_edid_protocol {
	uint32_t SizeOfEdid;
	uint8_t* Edid;
};


struct efi_edid_override_protocol {
	efi_status (*GetEdid) (struct efi_edid_override_protocol* self, efi_handle* child,
		uint32_t* attributes, size_t* edidSize, uint8_t** edid) EFIAPI;
};
