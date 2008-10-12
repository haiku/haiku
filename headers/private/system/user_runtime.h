/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
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

#define MAX_PROCESS_ARGS_SIZE	(128 * 1024)
	// maximal total size needed for process arguments and environment strings


struct user_space_program_args {
	char	program_name[B_OS_NAME_LENGTH];
	char	program_path[B_PATH_NAME_LENGTH];
	port_id	error_port;
	uint32	error_token;
	int		arg_count;
	int		env_count;
	char	**args;
	char	**env;
};

struct rld_export {
	// runtime linker API export
	image_id (*load_add_on)(char const *path, uint32 flags);
	status_t (*unload_add_on)(image_id imageID);
	status_t (*get_image_symbol)(image_id imageID, char const *symbolName,
		int32 symbolType, void **_location);
	status_t (*get_nth_image_symbol)(image_id imageID, int32 num, char *symbolName,
		int32 *nameLength, int32 *symbolType, void **_location);
	status_t (*test_executable)(const char *path, char *interpreter);
	status_t (*get_next_image_dependency)(image_id id, uint32 *cookie,
		const char **_name);

	status_t (*reinit_after_fork)();

	void (*call_atexit_hooks_for_range)(addr_t start, addr_t size);

	void (*call_termination_hooks)();

	const struct user_space_program_args *program_args;
};

extern struct rld_export *__gRuntimeLoader;

#endif	/* KERNEL_USER_RUNTIME_H_ */
