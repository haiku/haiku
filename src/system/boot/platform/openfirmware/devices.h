/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef OPENFIRMWARE_DEVICES_H
#define OPENFIRMWARE_DEVICES_H

#include <SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

status_t platform_get_next_device(int *_cookie, int root, const char *type,
	char *path, size_t pathSize);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	/* OPENFIRMWARE_DEVICES_H */
