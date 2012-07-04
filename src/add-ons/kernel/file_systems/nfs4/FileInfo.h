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

struct Filehandle {
			uint8		fSize;
			uint8		fFH[NFS4_FHSIZE];

	inline				Filehandle();
	inline				Filehandle(const Filehandle& fh);
	inline	Filehandle&	operator=(const Filehandle& fh);
};


class Filesystem;
class RequestBuilder;

// Complete information needed to identify a file in any situation.
// Unfortunately just a filehandle is not enough even when they are persistent
// since OPEN requires both parent filehandle and file name (just like LOOKUP).
struct FileInfo {
			uint64		fFileId;
			Filehandle	fHandle;

			Filehandle	fParent;
			const char*	fName;
			const char*	fPath;

	inline				FileInfo();
	inline				~FileInfo();
	inline				FileInfo(const FileInfo& fi);
	inline	FileInfo&	operator=(const FileInfo& fi);

			status_t	UpdateFileHandles(Filesystem* fs);

	static status_t		ParsePath(RequestBuilder& req, uint32& count,
							const char* _path);
};

struct FilesystemId {
			uint64		fMajor;
			uint64		fMinor;

	inline	bool		operator==(const FilesystemId& fsid) const;
	inline	bool		operator!=(const FilesystemId& fsid) const;
};


inline
Filehandle::Filehandle()
	:
	fSize(0)
{
}


inline
Filehandle::Filehandle(const Filehandle& fh)
	:
	fSize(fh.fSize)
{
	memcpy(fFH, fh.fFH, fSize);
}


inline Filehandle&
Filehandle::operator=(const Filehandle& fh)
{
	fSize = fh.fSize;
	memcpy(fFH, fh.fFH, fSize);
	return *this;
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
FilesystemId::operator==(const FilesystemId& fsid) const
{
	return fMajor == fsid.fMajor && fMinor == fsid.fMinor;
}


inline bool
FilesystemId::operator!=(const FilesystemId& fsid) const
{
	return !operator==(fsid);
}


#endif	// FILEHINFO_H

