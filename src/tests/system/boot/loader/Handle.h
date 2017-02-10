/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef HANDLE_H
#define HANDLE_H


#include <boot/vfs.h>


#ifdef __cplusplus

class Handle : public Node {
	public:
		Handle(int handle, bool takeOwnership = true);
		Handle(const char *path);
		Handle();
		virtual ~Handle();

		status_t InitCheck();

		void SetTo(int handle, bool takeOwnership = true);

		virtual ssize_t ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize);
		virtual ssize_t WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize);

		virtual status_t GetName(char *nameBuffer, size_t bufferSize) const;
		virtual off_t Size() const;

	protected:
		int		fHandle;
		bool	fOwnHandle;
		char	*fPath;
};

#endif	/* __cplusplus */

#endif	/* HANDLE_H */
