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
	mode_t	umask;	// (mode_t)-1 means not set
	bool	disable_user_addons;
};

#endif	/* KERNEL_USER_RUNTIME_H_ */
