/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>


struct arch_real_time_data {
	bigtime_t	system_time_offset;
	uint32		system_time_conversion_factor;
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */
