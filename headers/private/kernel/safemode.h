/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_SAFEMODE_H
#define _KERNEL_SAFEMODE_H

#include <driver_settings.h>

#include <safemode_defs.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t get_safemode_option(const char *parameter, char *buffer, size_t *_bufferSize);
bool get_safemode_boolean(const char *parameter, bool defaultValue);
status_t _user_get_safemode_option(const char *parameter, char *buffer, size_t *_bufferSize);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_SAFEMODE_H */
