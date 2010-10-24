/*
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
		int		fHandle;
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

class ExecDevice : public ConsoleNode {
	public:
		ExecDevice(struct IORequest *ioRequest);
		ExecDevice(size_t requestSize);
		ExecDevice();
		virtual ~ExecDevice();

		status_t	AllocRequest(size_t requestSize);

		status_t	Open(const char *name, unsigned long unit = 0,
			unsigned long flags = 0);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual off_t Size() const;

		struct IORequest	*Request() const { return fIORequest; };
		struct IOStdReq		*StdRequest() const { return fIOStdReq; };

		// *IO() call helpers
		status_t	Do(void);
		status_t	Clear(void);

	protected:
		struct IORequest	*fIORequest;
		struct IOStdReq	*fIOStdReq;
};


#endif	/* __cplusplus */

#endif	/* HANDLE_H */
