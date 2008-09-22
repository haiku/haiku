/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef LOADER_H
#define LOADER_H


#include <boot/vfs.h>


extern bool is_bootable(Directory *volume);
extern void to_boot_path(char *path, size_t pathSize);
extern status_t load_kernel(stage2_args *args, Directory *volume);
extern status_t load_modules(stage2_args *args, Directory *volume);

#endif	/* LOADER_H */
