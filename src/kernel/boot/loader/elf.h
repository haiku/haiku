/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef LOADER_ELF_H
#define LOADER_ELF_H


#include <boot/elf.h>
#include <boot/vfs.h>


extern status_t elf_load_image(Node *node, const char *path);
extern status_t elf_load_image(int fd, preloaded_image *image);

#endif	/* LOADER_ELF_H */
