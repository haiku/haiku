/*
 * Copyright 2018 Haiku Inc. All Rights Reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2019, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>


struct arch_real_time_data {
	bigtime_t	system_time_offset;
	uint64		system_time_conversion_factor;
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */

