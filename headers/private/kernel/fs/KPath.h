/*
 * Copyright 2004-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_PATH_H
#define _K_PATH_H


#include <KernelExport.h>


namespace BPrivate {
namespace DiskDevice {


class KPath {
public:
	enum {
		DEFAULT = 0,
		NORMALIZE = 0x01,
		TRAVERSE_LEAF_LINK = 0x02,
		LAZY_ALLOC = 0x04
	};
public:
								KPath(size_t bufferSize = B_PATH_NAME_LENGTH);
								KPath(const char* path, int32 flags = DEFAULT,
									size_t bufferSize = B_PATH_NAME_LENGTH);
								KPath(const KPath& other);
								~KPath();

			status_t			SetTo(const char* path, int32 flags = DEFAULT,
									size_t bufferSize = B_PATH_NAME_LENGTH);
			void				Adopt(KPath& other);

			status_t			InitCheck() const;

			status_t			SetPath(const char* path,
									int32 flags = DEFAULT);
			const char*			Path() const;
			size_t				Length() const
									{ return fPathLength; }

			size_t				BufferSize() const
									{ return fBufferSize; }
			char*				LockBuffer(bool force = false);
			void				UnlockBuffer();
			char*				DetachBuffer();

			const char*			Leaf() const;
			status_t			ReplaceLeaf(const char* newLeaf);
			bool				RemoveLeaf();
				// returns false, if nothing could be removed anymore

			status_t			Append(const char* toAppend,
									bool isComponent = true);

			status_t			Normalize(bool traverseLeafLink);

			KPath&				operator=(const KPath& other);
			KPath&				operator=(const char* path);

			bool				operator==(const KPath& other) const;
			bool				operator==(const char* path) const;
			bool				operator!=(const KPath& other) const;
			bool				operator!=(const char* path) const;

private:
			status_t			_AllocateBuffer();
			status_t			_Normalize(const char* path,
									bool traverseLeafLink);
			void				_ChopTrailingSlashes();

private:
			char*				fBuffer;
			size_t				fBufferSize;
			size_t				fPathLength;
			bool				fLocked;
			bool				fLazy;
			bool				fFailed;
			bool				fIsNull;
};


} // namespace DiskDevice
} // namespace BPrivate


using BPrivate::DiskDevice::KPath;


#endif	/* _K_PATH_H */
