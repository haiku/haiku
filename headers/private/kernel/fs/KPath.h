/*
 * Copyright 2004-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_PATH_H
#define _K_PATH_H


#include <KernelExport.h>


namespace BPrivate {
namespace DiskDevice {

class KPath {
	public:
		KPath(size_t bufferSize = B_PATH_NAME_LENGTH);
		KPath(const char* path, bool normalize = false,
			size_t bufferSize = B_PATH_NAME_LENGTH);
		KPath(const KPath& other);
		~KPath();

		status_t SetTo(const char *path, bool normalize = false,
			size_t bufferSize = B_PATH_NAME_LENGTH,
			bool traverseLeafLink = false);
		void Adopt(KPath& other);

		status_t InitCheck() const;

		status_t SetPath(const char *path, bool normalize = false,
			bool traverseLeafLink = false);
		const char *Path() const;
		size_t Length() const { return fPathLength; }

		size_t BufferSize() const { return fBufferSize; }
		char *LockBuffer();
		void UnlockBuffer();
		char* DetachBuffer();

		const char *Leaf() const;
		status_t ReplaceLeaf(const char *newLeaf);
		bool RemoveLeaf();
			// returns false, if nothing could be removed anymore

		status_t Append(const char *toAppend, bool isComponent = true);

		status_t Normalize(bool traverseLeafLink);

		KPath& operator=(const KPath& other);
		KPath& operator=(const char* path);

		bool operator==(const KPath& other) const;
		bool operator==(const char* path) const;
		bool operator!=(const KPath& other) const;
		bool operator!=(const char* path) const;

	private:
		void _ChopTrailingSlashes();

		char*	fBuffer;
		size_t	fBufferSize;
		size_t	fPathLength;
		bool	fLocked;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPath;

#endif	/* _K_PATH_H */
