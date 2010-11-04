/*
 * Copyright 2009, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

#include "multiboot.h"

#include <KernelExport.h>
#include <string.h>
#include <boot/stage2_args.h>

extern struct multiboot_info *gMultiBootInfo;

// load_driver_settings.h
extern status_t add_safe_mode_settings(char *buffer);

void
dump_multiboot_info(void)
{
	if (!gMultiBootInfo)
		return;
	dprintf("Found MultiBoot Info at %p\n", gMultiBootInfo);
	dprintf("	flags: 0x%lx\n", gMultiBootInfo->flags);

	if (gMultiBootInfo->flags & MULTIBOOT_INFO_BOOTDEV)
		dprintf("	boot_device: 0x%lx\n", gMultiBootInfo->boot_device);

	if (gMultiBootInfo->flags & MULTIBOOT_INFO_CMDLINE
		&& gMultiBootInfo->cmdline)
		dprintf("	cmdline: '%s'\n", (char *)gMultiBootInfo->cmdline);

	if (gMultiBootInfo->boot_loader_name)
		dprintf("	boot_loader_name: '%s'\n",
			(char *)gMultiBootInfo->boot_loader_name);
}


status_t
parse_multiboot_commandline(stage2_args *args)
{
	static const char *sArgs[] = { NULL, NULL };

	if (!gMultiBootInfo || !(gMultiBootInfo->flags & MULTIBOOT_INFO_CMDLINE)
		|| !gMultiBootInfo->cmdline)
		return B_ENTRY_NOT_FOUND;

	const char *cmdline = (const char *)gMultiBootInfo->cmdline;

	// skip kernel (bootloader) name
	cmdline = strchr(cmdline, ' ');
	if (!cmdline)
		return B_ENTRY_NOT_FOUND;
	cmdline++;
	if (*cmdline == '\0')
		return B_ENTRY_NOT_FOUND;

	sArgs[0] = cmdline;
	args->arguments = sArgs;

	return B_OK;
}
