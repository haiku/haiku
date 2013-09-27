/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include <boot/vfs.h>

#include "Volume.h"
#include "Stream.h"
#include "BPlusTree.h"


namespace BFS {

class Directory : public ::Directory {
	public:
		Directory(Volume &volume, block_run run);
		Directory(Volume &volume, off_t id);
		Directory(const Stream &stream);
		virtual ~Directory();

		status_t InitCheck();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node* LookupDontTraverse(const char* name);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		virtual status_t GetName(char *name, size_t size) const;
		virtual ino_t Inode() const;

	private:
		Stream		fStream;
		BPlusTree	fTree;

		typedef ::Directory _inherited;
};

}	// namespace BFS

#endif	/* DIRECTORY_H */
