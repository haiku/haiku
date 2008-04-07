/*
 * Copyright 2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#ifndef _USB_RAW_PRIVATE_H_
#define _USB_RAW_PRIVATE_H_

#include <lock.h>
#include <USB3.h>

typedef struct {
	usb_device			device;
	benaphore			lock;
	uint32				reference_count;

	char				name[32];
	void				*link;

	sem_id				notify;
	status_t			status;
	size_t				actual_length;
} raw_device;

#endif // _USB_RAW_PRIVATE_H_
