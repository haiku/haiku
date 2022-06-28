/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_SYS_LINK_ELF_H_
#define	_BSD_SYS_LINK_ELF_H_


#include <features.h>


#ifdef _DEFAULT_SOURCE


#include <os/kernel/elf.h>


struct dl_phdr_info {
	Elf_Addr dlpi_addr;
	const char *dlpi_name;
	const Elf_Phdr *dlpi_phdr;
	Elf_Half dlpi_phnum;
};


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*__dl_iterate_hdr_callback)(struct dl_phdr_info *, size_t, void *);
extern int dl_iterate_phdr(__dl_iterate_hdr_callback, void *);

#ifdef __cplusplus
}
#endif


#endif


#endif /* _BSD_SYS_LINK_ELF_H_ */
