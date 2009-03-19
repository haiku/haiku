/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#define FUSE_USE_VERSION FUSE_VERSION

#include <stdio.h>

#include "fuse_api.h"
#include "FUSEFileSystem.h"


int
fuse_main_real(int argc, char* argv[], const struct fuse_operations* op,
	size_t opSize, void* userData)
{
printf("fuse_main_real(%d, %p, %p, %ld, %p)\n", argc, argv, op, opSize,
userData);

	// run the main loop
	status_t error = FUSEFileSystem::GetInstance()->FinishInitClientFS(op,
		opSize, userData);

	return error == B_OK ? 0 : 1;
}


int
fuse_is_lib_option(const char* opt)
{
printf("fuse_is_lib_option(\"%s\")\n", opt);
	// TODO: Implement!
	return 0;
}


int
fuse_version(void)
{
	return FUSE_VERSION;
}
