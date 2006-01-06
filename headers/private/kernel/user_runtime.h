/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Manuel J. Petit. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_USER_RUNTIME_H_
#define KERNEL_USER_RUNTIME_H_


#include <image.h>
#include <OS.h>


#define MAGIC_APP_NAME	"_APP_"

struct uspace_program_args {
	char program_name[B_OS_NAME_LENGTH];
	char program_path[B_PATH_NAME_LENGTH];
	int  argc;
	int  envc;
	char **argv;
	char **envp;
};

struct rld_export {
	// runtime linker API export
	image_id (*load_add_on)(char const *path, uint32 flags);
	status_t (*unload_add_on)(image_id imageID);
	status_t (*get_image_symbol)(image_id imageID, char const *symbolName,
		int32 symbolType, void **_location);
	status_t (*get_nth_image_symbol)(image_id imageID, int32 num, char *symbolName,
		int32 *nameLength, int32 *symbolType, void **_location);
	status_t (*test_executable)(const char *path, uid_t user, gid_t group,
		char *starter);

	const struct uspace_program_args *program_args;
};

extern struct rld_export *__gRuntimeLoader;

#endif	/* KERNEL_USER_RUNTIME_H_ */
