/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>

struct arm64_real_time_data {
	vint64	system_time_offset;
};

struct arch_real_time_data {
	struct arm64_real_time_data	data[2];
	vint32						system_time_conversion_factor;
	vint32						version;
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */
