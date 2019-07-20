/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_STDLIB_H_
#define _BSD_STDLIB_H_


#include_next <stdlib.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#ifdef __cplusplus
extern "C" {
#endif

int			daemon(int noChangeDir, int noClose);
const char	*getprogname(void);
void		setprogname(const char *programName);

int			mkstemps(char *templat, int slen);

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_STDLIB_H_ */
