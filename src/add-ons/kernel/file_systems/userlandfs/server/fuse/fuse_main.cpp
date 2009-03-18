/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#define FUSE_USE_VERSION FUSE_VERSION

#include <fuse.h>


int
fuse_main_real(int argc, char* argv[], const struct fuse_operations* op,
	size_t op_size, void *user_data)
{
	// TODO: Implement!
	return 0;
}


int
fuse_is_lib_option(const char *opt)
{
	// TODO: Implement!
	return 0;
}


int
fuse_version(void)
{
	return FUSE_VERSION;
}
