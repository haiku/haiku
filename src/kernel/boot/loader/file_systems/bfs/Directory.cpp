/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Directory.h"
#include "Volume.h"


namespace BFS {

Directory::Directory(Volume &volume, block_run run)
	:
	fStream(volume, run)
{
}


Directory::~Directory()
{
}


status_t 
Directory::InitCheck()
{
	return B_ERROR;
}


status_t 
Directory::Open(void **_cookie, int mode)
{
	return B_ERROR;
}


status_t 
Directory::Close(void *cookie)
{
	return B_ERROR;
}


Node *
Directory::Lookup(const char *name, bool traverseLinks)
{
	return NULL;
}


status_t 
Directory::GetNextNode(void *cookie, Node **_node)
{
	return B_ERROR;
}


status_t 
Directory::Rewind(void *cookie)
{
	return B_ERROR;
}

}	// namespace BFS
