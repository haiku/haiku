/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "amiga_ffs.h"

#include <boot/vfs.h>


namespace FFS {

class Volume;

class Directory : public ::Directory {
	public:
		Directory();
		Directory(Volume &volume, RootBlock &root);
		Directory(Volume &volume, int32 block);
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
		Volume			&fVolume;
		DirectoryBlock	fNode;

		typedef ::Directory _inherited;
};

}	// namespace FFS

#endif	/* DIRECTORY_H */
