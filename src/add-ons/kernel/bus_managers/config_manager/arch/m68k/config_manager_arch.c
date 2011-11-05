/*
 * Copyright 2007 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * arch-specific config manager
 *
 * Authors (in chronological order):
 *              François Revol (revol@free.fr)
 */

#include <OS.h>
#include <config_manager.h>

int config_manager_scan_hardcoded(struct device_info **info, int32 *count)
{
	//XXX
	return atari_hardcoded(info, count);
	
	count = 0;
	return B_OK;
}

