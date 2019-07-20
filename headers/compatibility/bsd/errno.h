/*
 * Copyright 2008-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_ERRNO_H_
#define _BSD_ERRNO_H_


#include_next <errno.h>
#include <features.h>


#ifdef _DEFAULT_SOURCE


#define EDOOFUS	EINVAL


#endif


#endif	/* _BSD_ERRNO_H_ */
