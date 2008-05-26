/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LEGACY_DRIVERS_H
#define LEGACY_DRIVERS_H


#include <Drivers.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t legacy_driver_add(const char* path);
status_t legacy_driver_publish(const char* path, device_hooks* hooks);
status_t legacy_driver_rescan(const char* driverName);
status_t legacy_driver_probe(const char* path);
status_t legacy_driver_init(void);

#ifdef __cplusplus
}
#endif

#endif	// LEGACY_DRIVERS_H
