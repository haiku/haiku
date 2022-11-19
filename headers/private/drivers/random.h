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

	status_t (*queue_randomness)(uint64 value);
} random_for_controller_interface;


#define RANDOM_FOR_CONTROLLER_MODULE_NAME "bus_managers/random/controller/driver_v1"


#endif /* _RANDOM_H */
