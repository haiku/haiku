/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHECKSUM_DEVICE_H
#define CHECKSUM_DEVICE_H


#include <Drivers.h>

#include "CheckSum.h"


enum {
	CHECKSUM_DEVICE_IOCTL_GET_CHECK_SUM	= B_DEVICE_OP_CODES_END + 0x666,
	CHECKSUM_DEVICE_IOCTL_SET_CHECK_SUM
};


struct checksum_device_ioctl_check_sum {
	uint64		blockIndex;
	CheckSum	checkSum;
};


#endif	// CHECKSUM_DEVICE_H
