/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>

struct arch_real_time_data {
	bigtime_t	system_time_offset;
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */
