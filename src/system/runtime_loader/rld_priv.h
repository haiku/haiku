/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef RUN_TIME_LINKER_H
#define RUN_TIME_LINKER_H


#include <user_runtime.h>
#include <runtime_loader.h>

int runtime_loader(void *arg);
int open_executable(char *name, image_type type, const char *rpath);
status_t test_executable(const char *path, uid_t user, gid_t group, char *starter);

void terminate_program(void);
image_id load_program(char const *path, void **entry);
status_t unload_library(image_id imageID, bool addOn);
image_id load_library(char const *path, uint32 flags, bool addOn);
status_t get_nth_symbol(image_id imageID, int32 num, char *nameBuffer, int32 *_nameLength,
	int32 *_type, void **_location);
status_t get_symbol(image_id imageID, char const *symbolName, int32 symbolType,
	void **_location);
int resolve_symbol(image_t *image, struct Elf32_Sym *sym, addr_t *sym_addr);


status_t elf_verify_header(void *header, int32 length);
void rldelf_init(void);
void rldexport_init(void);

// RLD heap
void rldheap_init(void);
void *rldalloc(size_t);
void rldfree(void *p);

// arch dependent prototypes
status_t arch_relocate_image(image_t *image);
void arch_call_init(image_t *image);
void arch_call_term(image_t *image);

extern struct uspace_program_args *gProgramArgs;
extern struct rld_export gRuntimeLoader;

#endif	/* RUN_TIME_LINKER_H */
