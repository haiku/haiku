/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_UCRED_H_
#define _OBSD_COMPAT_SYS_UCRED_H_


#include <sys/priv.h>


#define suser(...) priv_check(NULL, PRIV_DRIVER)


#endif	/* _OBSD_COMPAT_SYS_UCRED_H_ */
