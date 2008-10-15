/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "fatfs.h"
#include "Stream.h"

#include <boot/vfs.h>


namespace FATFS {

class Volume;

class Directory : public ::Directory {
	public:
		Directory();
		Directory(Volume &volume, uint32 cluster, const char *name);
		virtual ~Directory();

		status_t InitCheck();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node *Lookup(const char *name, bool traverseLinks);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		virtual status_t GetName(char *name, size_t size) const;
		virtual ino_t Inode() const;

	private:
		status_t GetNextEntry(void *cookie, 
				uint8 mask = FAT_VOLUME, uint8 match = 0);
		Volume			&fVolume;
		Stream			fStream;

		typedef ::Directory _inherited;
};

}	// namespace FATFS

#endif	/* DIRECTORY_H */
