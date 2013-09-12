/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@berlios.de
 */
#ifndef _RANDOM_H
#define _RANDOM_H


#include <device_manager.h>


typedef struct {
	driver_module_info info;
} random_for_controller_interface;


#define RANDOM_FOR_CONTROLLER_MODULE_NAME "bus_managers/random/controller/driver_v1"

// Bus manager interface used by Random controller drivers.
typedef struct random_module_info {
	driver_module_info info;

	status_t (*read)(void* cookie, void *_buffer, size_t *_numBytes);
	status_t (*write)(void* cookie, const void *_buffer, size_t *_numBytes);
} random_module_info;


#endif /* _RANDOM_H */
