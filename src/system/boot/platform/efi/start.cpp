/*
 * Copyright 2014-2016 Haiku, Inc. All rights reserved.
 * Copyright 2013 Fredrik Holmqvist, fredrik.holmqvist@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

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


extern "C" uint32
platform_boot_options()
{
	return 0;
}


extern "C" void
platform_start_kernel(void)
{
	panic("platform_start_kernel not implemented");
}


extern "C" void
platform_exit(void)
{
	return;
}


/**
 * efi_main - The entry point for the EFI application
 * @image: firmware-allocated handle that identifies the image
 * @systemTable: EFI system table
 */
extern "C" EFI_STATUS
efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systemTable)
{
	stage2_args args;

	kSystemTable = systemTable;
	kBootServices = systemTable->BootServices;
	kRuntimeServices = systemTable->RuntimeServices;

	memset(&args, 0, sizeof(stage2_args));

	call_ctors();

	console_init();

	main(&args);

	return EFI_SUCCESS;
}
