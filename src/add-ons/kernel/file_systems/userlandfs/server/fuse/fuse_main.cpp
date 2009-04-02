/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <signal.h>
#include <stdio.h>

#include "fuse_api.h"
#include "fuse_config.h"
#include "FUSEFileSystem.h"


int
fuse_main_real(int argc, char* argv[], const struct fuse_operations* op,
	size_t opSize, void* userData)
{
printf("fuse_main_real(%d, %p, %p, %ld, %p)\n", argc, argv, op, opSize,
userData);

	// parse args
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	fuse_config config;
	memset(&config, 0, sizeof(config));
	config.entry_timeout = 1.0;
	config.attr_timeout = 1.0;
	config.negative_timeout = 0.0;
	config.intr_signal = SIGUSR1;

	bool success = fuse_parse_config_args(&args, &config);
	fuse_opt_free_args(&args);

	if (!success)
		return 1;

	// run the main loop
	status_t error = FUSEFileSystem::GetInstance()->FinishInitClientFS(&config,
		op, opSize, userData);


	return error == B_OK ? 0 : 1;
}


int
fuse_version(void)
{
	return FUSE_VERSION;
}
