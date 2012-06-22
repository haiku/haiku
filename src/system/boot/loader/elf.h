/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef LOADER_ELF_H
#define LOADER_ELF_H


#include <boot/elf.h>
#include <boot/vfs.h>


extern void elf_init();
extern status_t elf_load_image(Directory *directory, const char *path);
extern status_t elf_load_image(int fd, preloaded_image **_image);

extern status_t elf_relocate_image(preloaded_image *image);

#endif	/* LOADER_ELF_H */
