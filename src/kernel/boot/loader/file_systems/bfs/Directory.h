/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include <boot/vfs.h>

#include "Volume.h"


namespace BFS {

class Directory : public ::Directory {
	public:
		Directory(BFS::Volume &volume, block_run run);
		virtual ~Directory();

		status_t InitCheck();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node *Lookup(const char *name, bool traverseLinks);

		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);

	private:
		Volume		&fVolume;
		bfs_inode	fNode;
};

}	// namespace BFS

#endif	/* DIRECTORY_H */
