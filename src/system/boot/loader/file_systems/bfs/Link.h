/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef LINK_H
#define LINK_H


#include <boot/vfs.h>

#include "File.h"


namespace BFS {

class Link : public File {
	public:
		Link(Volume &volume, block_run run);
		Link(Volume &volume, off_t id);
		Link(const Stream &stream);

		status_t InitCheck();

		virtual status_t ReadLink(char *buffer, size_t bufferSize);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual int32 Type() const;
};

}	// namespace BFS

#endif	/* FILE_H */
