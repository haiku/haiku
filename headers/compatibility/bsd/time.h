/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_TIME_H_
#define _BSD_TIME_H_


#include_next <time.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

time_t	timelocal(struct tm *tm);
time_t	timegm(struct tm *tm);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_TIME_H_ */
