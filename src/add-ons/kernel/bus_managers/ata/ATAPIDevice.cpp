/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2008, Marcus Overhagen.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002-2003, Thomas Kurschel.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"

ATAPIDevice::ATAPIDevice(ATAChannel *channel, uint8 index)
	:	ATADevice(channel, index),
		fLastLun(1)
{
}


ATAPIDevice::~ATAPIDevice()
{
}


status_t
ATAPIDevice::ExecuteIO(ATARequest *request)
{
	request->SetStatus(SCSI_SEL_TIMEOUT);
	request->SetSense(SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
	return B_ERROR;
}
