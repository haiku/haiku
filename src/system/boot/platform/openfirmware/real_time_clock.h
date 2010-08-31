/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef OPENFIRMWARE_REAL_TIME_CLOCK_H
#define OPENFIRMWARE_REAL_TIME_CLOCK_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t init_real_time_clock(void);
bigtime_t real_time_clock_usecs(void);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	/* OPENFIRMWARE_REAL_TIME_CLOCK_H */
