/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOAD_DRIVER_SETTINGS_H
#define LOAD_DRIVER_SETTINGS_H


#include <boot/vfs.h>


extern status_t load_driver_settings(stage2_args *args, Directory *volume);

#endif	/* LOAD_DRIVER_SETTINGS_H */
