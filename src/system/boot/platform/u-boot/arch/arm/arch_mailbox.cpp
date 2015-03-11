/*
 * Copyright 2012-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_mailbox.h"

#include <board_config.h>


ArchMailbox *gMailbox = NULL;


extern "C" status_t
arch_mailbox_init()
{
	#if defined(BOARD_CPU_BCM2835) || defined(BOARD_CPU_BCM2836)
	extern ArchMailbox *arch_get_mailbox_arm_bcm2835(addr_t base);
	gMailbox = arch_get_mailbox_arm_bcm2835(DEVICE_BASE + ARM_CTRL_0_MAILBOX_BASE);
	#endif
	return B_OK;
}
