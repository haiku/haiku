/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_UNISTD_H_
#define _BSD_UNISTD_H_


#include_next <unistd.h>


#ifdef __cplusplus
extern "C" {
#endif

int issetugid(void);

#ifdef __cplusplus
}
#endif

#endif	/* _BSD_UNISTD_H_ */
