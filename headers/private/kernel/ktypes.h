/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KTYPES_H
#define _KERNEL_KTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SupportDefs.h>

typedef int32  region_id;
typedef int32  aspace_id;

#ifndef _IMAGE_H
	typedef int32  image_id;
#endif


typedef unsigned long			addr;

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_KTYPES_H */
