/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SIGNAL_H_
#define _BSD_SIGNAL_H_


#include_next <signal.h>


#ifdef __cplusplus
extern "C" {
#endif

int sigsetmask(int mask);
int sigblock(int mask);

#ifdef __cplusplus
}
#endif

#endif	/* _BSD_SIGNAL_H_ */
