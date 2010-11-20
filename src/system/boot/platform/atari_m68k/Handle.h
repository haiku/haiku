/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HANDLE_H
#define HANDLE_H


#include <boot/vfs.h>


#ifdef __cplusplus

class Handle : public ConsoleNode {
	public:
		Handle(int handle);
		Handle();
		virtual ~Handle();

		void SetHandle(int handle);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

	protected:
		int16		fHandle;
};

/* character devices */
class CharHandle : public Handle {
	public:
		CharHandle(int handle);
		CharHandle();
		virtual ~CharHandle();

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

	protected:
};

/* block devices */
/* cf. devices.cpp */

#endif	/* __cplusplus */

#endif	/* HANDLE_H */
