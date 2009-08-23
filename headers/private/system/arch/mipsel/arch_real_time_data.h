/*
 * Copyright 2006-2009, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_ARCH_REAL_TIME_DATA_H
#define _SYSTEM_ARCH_REAL_TIME_DATA_H


#include <SupportDefs.h>


struct arch_real_time_data {
	bigtime_t	system_time_offset;
	uint32		system_time_conversion_factor;
};

#endif /* _SYSTEM_ARCH_REAL_TIME_DATA_H */

