/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_ELF_H
#define KERNEL_BOOT_ELF_H


#include <elf_priv.h>


struct preloaded_image {
	struct preloaded_image *next;
	char		*name;
	elf_region	text_region;
	elf_region	data_region;
	addr		dynamic_section;	// pointer to the dynamic section
	Elf32_Ehdr	elf_header;
};

#endif	/* KERNEL_BOOT_ELF_H */
