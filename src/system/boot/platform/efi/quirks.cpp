/*
 * Copyright 2015, Bruno Bierbaumer. All rights reserved.
 * Copyright 2019-2020, Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License
 */


#include <efi/protocol/apple-setos.h>
#include <KernelExport.h>

#include "efi_platform.h"


#define APPLE_FAKE_OS_VENDOR "Apple Inc."
#define APPLE_FAKE_OS_VERSION "Mac OS X 10.9"


// Apple Hardware configures hardware differently depending on
// the operating system being booted. Examples include disabling
// and powering down the internal GPU on some device models.
static void
quirks_fake_apple(void)
{
	efi_guid appleSetOSProtocolGUID = EFI_APPLE_SET_OS_GUID;
	efi_apple_set_os_protocol* set_os = NULL;

	efi_status status = kSystemTable->BootServices->LocateProtocol(
		&appleSetOSProtocolGUID, NULL, (void**)&set_os);

	// If not relevant, we will exit here (the protocol doesn't exist)
	if (status != EFI_SUCCESS || set_os == NULL) {
		return;
	}

	dprintf("Located Apple set_os protocol, applying EFI Apple Quirks...\n");

	if (set_os->Revision != 0) {
		status = set_os->SetOSVersion((char*)APPLE_FAKE_OS_VERSION);
		if (status != EFI_SUCCESS) {
			dprintf("%s: unable to set os version!\n", __func__);
			return;
		}
	}

	status = set_os->SetOSVendor((char*)APPLE_FAKE_OS_VENDOR);
	if (status != EFI_SUCCESS) {
		dprintf("%s: unable to set os version!\n", __func__);
		return;
	}

	return;
}


void
quirks_init(void)
{
	quirks_fake_apple();
}
