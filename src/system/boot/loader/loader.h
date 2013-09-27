/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef LOADER_H
#define LOADER_H


#include <boot/vfs.h>


extern bool is_bootable(Directory* volume);
extern status_t load_kernel(stage2_args* args, BootVolume& volume);
extern status_t load_modules(stage2_args* args, BootVolume& volume);

#endif	/* LOADER_H */
