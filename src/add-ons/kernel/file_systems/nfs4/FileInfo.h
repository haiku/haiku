/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef FILEINFO_H
#define FILEINFO_H


#include <stdlib.h>
#include <string.h>

#include <lock.h>
#include <SupportDefs.h>
#include <util/KernelReferenceable.h>


#define NFS4_FHSIZE	128

struct FileHandle {
			uint8		fSize;
			uint8		fData[NFS4_FHSIZE];

	inline				FileHandle();
	inline				FileHandle(const FileHandle& fh);
	inline	FileHandle&	operator=(const FileHandle& fh);


	inline	bool		operator==(const FileHandle& handle) const;
	inline	bool		operator!=(const FileHandle& handle) const;
	inline	bool		operator>(const FileHandle& handle) const;
	inline	bool		operator<(const FileHandle& handle) const;
};

class InodeNames;

struct InodeName : public SinglyLinkedListLinkImpl<InodeName> {
				InodeName(InodeNames* parent, const char* name);
				~InodeName();

	InodeNames*	fParent;
	const char*	fName;
};

struct InodeNames : public KernelReferenceable {
								InodeNames();
								~InodeNames();

	status_t					AddName(InodeNames* parent, const char* name);
	bool						RemoveName(InodeNames* parent,
									const char* name);

	mutex						fLock;
	SinglyLinkedList<InodeName>	fNames;
	FileHandle					fHandle;
};

class FileSystem;
class RequestBuilder;

// Complete information needed to identify a file in any situation.
// Unfortunately just a FileHandle is not enough even when they are persistent
// since OPEN requires both parent FileHandle and file name (just like LOOKUP).
struct FileInfo {
			uint64		fFileId;
			FileHandle	fHandle;

			InodeNames*	fNames;

			FileHandle	fAttrDir;

						FileInfo();
						~FileInfo();
						FileInfo(const FileInfo& fi);
			FileInfo&	operator=(const FileInfo& fi);

			status_t	UpdateFileHandles(FileSystem* fs);
};

struct FileSystemId {
			uint64		fMajor;
			uint64		fMinor;

	inline	bool		operator==(const FileSystemId& fsid) const;
	inline	bool		operator!=(const FileSystemId& fsid) const;
};


inline
FileHandle::FileHandle()
	:
	fSize(0)
{
}


inline
FileHandle::FileHandle(const FileHandle& fh)
	:
	fSize(fh.fSize)
{
	memcpy(fData, fh.fData, fSize);
}


inline FileHandle&
FileHandle::operator=(const FileHandle& fh)
{
	fSize = fh.fSize;
	memcpy(fData, fh.fData, fSize);
	return *this;
}


inline bool
FileHandle::operator==(const FileHandle& handle) const
{
	if (fSize != handle.fSize)
		return false;
	return memcmp(fData, handle.fData, fSize) == 0;
}


inline bool
FileHandle::operator!=(const FileHandle& handle) const
{
	return !operator==(handle);
}


inline bool
FileHandle::operator>(const FileHandle& handle) const
{
	if (fSize > handle.fSize)
		return true;
	return memcmp(fData, handle.fData, fSize) > 0;
}


inline bool
FileHandle::operator<(const FileHandle& handle) const
{
	if (fSize < handle.fSize)
		return true;
	return memcmp(fData, handle.fData, fSize) < 0;
}


inline bool
FileSystemId::operator==(const FileSystemId& fsid) const
{
	return fMajor == fsid.fMajor && fMinor == fsid.fMinor;
}


inline bool
FileSystemId::operator!=(const FileSystemId& fsid) const
{
	return !operator==(fsid);
}


#endif	// FILEHINFO_H

