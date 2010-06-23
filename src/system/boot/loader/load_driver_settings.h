/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOAD_DRIVER_SETTINGS_H
#define LOAD_DRIVER_SETTINGS_H


#include <boot/vfs.h>


extern status_t add_safe_mode_settings(char *buffer);
extern status_t add_stage2_driver_settings(stage2_args *args);
extern status_t load_driver_settings(stage2_args *args, Directory *volume);

extern void apply_boot_settings();


#endif	/* LOAD_DRIVER_SETTINGS_H */
