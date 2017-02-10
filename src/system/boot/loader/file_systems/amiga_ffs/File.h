/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef FILE_H
#define FILE_H


#include <boot/vfs.h>

#include "Volume.h"


namespace FFS {

class File : public Node {
	public:
		File(Volume &volume, int32 block);
		virtual ~File();

		status_t InitCheck();

		virtual status_t Open(void **_cookie, int mode);
		virtual status_t Close(void *cookie);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

	private:
		Volume		&fVolume;
		FileBlock	fNode;
};

}	// namespace FFS

#endif	/* FILE_H */
