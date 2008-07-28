/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// included by vfs.cpp

//#include "io_requests.h"


// #pragma mark - public API


extern "C" fssh_status_t
fssh_do_fd_io(int fd, fssh_io_request* request)
{
	fssh_panic("fssh_do_fd_io() not yet implemented");
	return FSSH_B_BAD_VALUE;
}


extern "C" fssh_status_t
fssh_do_iterative_fd_io(int fd, fssh_io_request* request,
	fssh_iterative_io_get_vecs getVecs,
	fssh_iterative_io_finished finished, void* cookie)
{
	fssh_panic("fssh_do_iterative_fd_io() not yet implemented");
	return FSSH_B_BAD_VALUE;
}
