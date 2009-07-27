/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_REAL_TIME_DATA_H
#define _KERNEL_ARCH_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>

#warning ARM: fix system_time()

struct arch_real_time_data {
	vint32						system_time_conversion_factor;
	vint32						version;
};

#endif	/* _KERNEL_ARCH_REAL_TIME_DATA_H */
