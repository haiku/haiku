// Copyright 2015 Bruno Bierbaumer
// Released under the terms of the MIT License

#pragma once

#include <efi/types.h>

#define EFI_APPLE_SET_OS_GUID \
	{0xc5c5da95, 0x7d5c, 0x45e6, {0xb2, 0xf1, 0x3f, 0xd5, 0x2b, 0xb1, 0x00, 0x77}}
extern efi_guid AppleSetOSProtocol;

typedef struct efi_apple_set_os_protocol {
	uint64_t Revision;

	efi_status (*SetOSVersion) (char* version) EFIAPI;
	efi_status (*SetOSVendor) (char* vendor) EFIAPI;
} efi_apple_set_os_protocol;
