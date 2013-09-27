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

struct dir_entry;
class Volume;

class Directory : public ::Directory {
	public:
		Directory();
		Directory(Volume &volume, off_t dirEntryOffset, uint32 cluster,
			const char *name);
		virtual ~Directory();

		status_t InitCheck();

		off_t DirEntryOffset() const	{ return fDirEntryOffset; }

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual Node* LookupDontTraverse(const char* name);

		virtual status_t GetNextEntry(void *cookie, char *nameBuffer, size_t bufferSize);
		virtual status_t GetNextNode(void *cookie, Node **_node);
		virtual status_t Rewind(void *cookie);
		virtual bool IsEmpty();

		virtual status_t GetName(char *name, size_t size) const;
		virtual ino_t Inode() const;

		virtual status_t CreateFile(const char *name, mode_t permissions,
			Node **_node);

		static status_t UpdateDirEntry(Volume& volume, off_t dirEntryOffset,
			uint32 firstCluster, uint32 size);

	private:
		status_t GetNextEntry(void *cookie,
				uint8 mask = FAT_VOLUME, uint8 match = 0);
		status_t _AddEntry(dir_entry& entry, off_t& _entryOffset);
		off_t _GetStreamSize();

		Volume			&fVolume;
		Stream			fStream;
		off_t			fDirEntryOffset;

		typedef ::Directory _inherited;
};

}	// namespace FATFS

#endif	/* DIRECTORY_H */
