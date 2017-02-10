/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef FILE_H
#define FILE_H


#include <boot/vfs.h>

#include "Stream.h"
#include "Volume.h"


namespace FATFS {

class File : public Node {
	public:
		File(Volume &volume, off_t dirEntryOffset, uint32 cluster, off_t size,
			const char *name);
		virtual ~File();

		status_t InitCheck();

		off_t DirEntryOffset() const	{ return fDirEntryOffset; }

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual status_t GetFileMap(struct file_map_run *runs, int32 *count);
		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

	private:
		Volume		&fVolume;
		//FileBlock	fNode;
		Stream		fStream;
		off_t		fDirEntryOffset;
};

}	// namespace FATFS

#endif	/* FILE_H */
