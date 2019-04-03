/*
 * Copyright 2005-2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _KERNEL_OPEN_FIRMWARE_DEVICES_H
#define _KERNEL_OPEN_FIRMWARE_DEVICES_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t of_get_next_device(intptr_t *_cookie, intptr_t root, const char *type,
	char *path, size_t pathSize);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	/* _KERNEL_OPEN_FIRMWARE_DEVICES_H */
