/*
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
 * Copyright 2013 Fredrik Holmqvist, fredrik.holmqvist@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include "console.h"
#include "efi_platform.h"


extern void (*__ctor_list)(void);
extern void (*__ctor_end)(void);


const EFI_SYSTEM_TABLE		*kSystemTable;
const EFI_BOOT_SERVICES		*kBootServices;
const EFI_RUNTIME_SERVICES	*kRuntimeServices;


static void
call_ctors(void)
{
	void (**f)(void);

	for (f = &__ctor_list; f < &__ctor_end; f++)
		(**f)();
}


/**
 * efi_main - The entry point for the EFI application
 * @image: firmware-allocated handle that identifies the image
 * @systemTable: EFI system table
 */
extern "C" EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systemTable)
{
	kSystemTable = systemTable;
	kBootServices = systemTable->BootServices;
	kRuntimeServices = systemTable->RuntimeServices;

	call_ctors();

	console_init();

	printf("Hello from EFI Loader for Haiku!\nPress any key to continue...\n");
	console_wait_for_key();

	return EFI_SUCCESS;
}
