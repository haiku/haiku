/*
 * Copyright 2006-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_STDLIB_H_
#define _BSD_STDLIB_H_


#include_next <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

int			daemon(int noChangeDir, int noClose);
const char	*getprogname(void);
void		setprogname(const char *programName);

#ifdef __cplusplus
}
#endif

#endif	/* _BSD_STDLIB_H_ */
