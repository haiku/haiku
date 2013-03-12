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

#include <SupportDefs.h>


#define NFS4_FHSIZE	128

struct FileHandle {
			uint8		fSize;
			uint8		fData[NFS4_FHSIZE];

	inline				FileHandle();
	inline				FileHandle(const FileHandle& fh);
	inline	FileHandle&	operator=(const FileHandle& fh);


	inline	bool		operator!=(const FileHandle& handle) const;
	inline	bool		operator>(const FileHandle& handle) const;
	inline	bool		operator<(const FileHandle& handle) const;
};


class FileSystem;
class RequestBuilder;

// Complete information needed to identify a file in any situation.
// Unfortunately just a FileHandle is not enough even when they are persistent
// since OPEN requires both parent FileHandle and file name (just like LOOKUP).
struct FileInfo {
			uint64		fFileId;
			FileHandle	fHandle;

			FileHandle	fParent;
			const char*	fName;
			const char*	fPath;

			FileHandle	fAttrDir;

	inline				FileInfo();
	inline				~FileInfo();
	inline				FileInfo(const FileInfo& fi);
	inline	FileInfo&	operator=(const FileInfo& fi);

			status_t	UpdateFileHandles(FileSystem* fs);

	static	status_t	ParsePath(RequestBuilder& req, uint32& count,
							const char* _path);

			status_t	CreateName(const char* dirPath, const char* name);
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
FileHandle::operator!=(const FileHandle& handle) const
{
	if (fSize != handle.fSize)
		return true;
	return memcmp(fData, handle.fData, fSize) != 0;
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


inline
FileInfo::FileInfo()
	:
	fFileId(0),
	fName(NULL),
	fPath(NULL)
{
}


inline
FileInfo::~FileInfo()
{
	free(const_cast<char*>(fName));
	free(const_cast<char*>(fPath));
}


inline
FileInfo::FileInfo(const FileInfo& fi)
	:
	fFileId(fi.fFileId),
	fHandle(fi.fHandle),
	fParent(fi.fParent),
	fName(strdup(fi.fName)),
	fPath(strdup(fi.fPath))
{
}


inline FileInfo&
FileInfo::operator=(const FileInfo& fi)
{
	fFileId = fi.fFileId;
	fHandle = fi.fHandle;
	fParent = fi.fParent;

	free(const_cast<char*>(fName));
	fName = strdup(fi.fName);

	free(const_cast<char*>(fPath));
	fPath = strdup(fi.fPath);

	return *this;
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

