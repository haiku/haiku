/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef FILE_H
#define FILE_H


#include <boot/vfs.h>

#include "Volume.h"
#include "Stream.h"


namespace BFS {

class File : public Node {
	public:
		File(Volume &volume, block_run run);
		File(Volume &volume, off_t id);
		File(const Stream &stream);
		virtual ~File();

		status_t InitCheck();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual int32 Type() const;
		virtual off_t Size() const;
		virtual ino_t Inode() const;

	protected:
		Stream		fStream;
};

}	// namespace BFS

#endif	/* FILE_H */
