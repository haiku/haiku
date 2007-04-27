/*
 * Copyright 2004-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_PATH_H
#define _K_PATH_H


#include "fssh_defs.h"
#include "fssh_kernel_export.h"


namespace FSShell {

class KPath {
	public:
		KPath(fssh_size_t bufferSize = FSSH_B_PATH_NAME_LENGTH);
		KPath(const char* path, bool normalize = false,
			fssh_size_t bufferSize = FSSH_B_PATH_NAME_LENGTH);
		KPath(const KPath& other);
		~KPath();

		fssh_status_t SetTo(const char *path, bool normalize = false,
			fssh_size_t bufferSize = FSSH_B_PATH_NAME_LENGTH);

		fssh_status_t InitCheck() const;

		fssh_status_t SetPath(const char *path, bool normalize = false);
		const char *Path() const;
		fssh_size_t Length() const { return fPathLength; }

		fssh_size_t BufferSize() const { return fBufferSize; }
		char *LockBuffer();
		void UnlockBuffer();

		const char *Leaf() const;
		fssh_status_t ReplaceLeaf(const char *newLeaf);

		fssh_status_t Append(const char *toAppend, bool isComponent = true);

		KPath& operator=(const KPath& other);
		KPath& operator=(const char* path);

		bool operator==(const KPath& other) const;
		bool operator==(const char* path) const;
		bool operator!=(const KPath& other) const;
		bool operator!=(const char* path) const;

	private:
		void _ChopTrailingSlashes();

		char*		fBuffer;
		fssh_size_t	fBufferSize;
		fssh_size_t	fPathLength;
		bool		fLocked;
};

} // namespace FSShell

using FSShell::KPath;

#endif	/* _K_PATH_H */
