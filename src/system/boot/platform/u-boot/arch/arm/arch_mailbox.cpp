/*
 * Copyright 2012-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_mailbox.h"

#include "fdt_support.h"

ArchMailbox *gMailbox = NULL;
extern void* gFDT;

extern "C" status_t
arch_mailbox_init()
{
	extern ArchMailbox *arch_get_mailbox_arm_bcm2835(addr_t base);
	phys_addr_t mboxBase = fdt_get_device_reg_byname(gFDT, "/axi/mbox");
	if (!mboxBase) {
		return B_ERROR;
	}
	gMailbox = arch_get_mailbox_arm_bcm2835(mboxBase);
	return B_OK;
}
