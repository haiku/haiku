/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef FILEHANDLE_H
#define FILEHANDLE_H


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


// Complete information needed to identify a file in any situation.
// Unfortunately just a filehandle is not enough even when they are persistent
// since OPEN requires both parent filehandle and file name (just like LOOKUP).
struct FileInfo {
			Filehandle	fFH;

			Filehandle	fParent;
			const char*	fName;

			// ... full path may be needed if filehandles are volatile

	inline				FileInfo();
	inline				~FileInfo();
	inline				FileInfo(const FileInfo& fi);
	inline	FileInfo&	operator=(const FileInfo& fi);
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
	fName(NULL)
{
}


inline
FileInfo::~FileInfo()
{
	free(const_cast<char*>(fName));
}


inline
FileInfo::FileInfo(const FileInfo& fi)
	:
	fFH(fi.fFH),
	fParent(fi.fParent),
	fName(strdup(fi.fName))
{
}


inline FileInfo&
FileInfo::operator=(const FileInfo& fi)
{
	fFH = fi.fFH;
	fParent = fi.fParent;

	free(const_cast<char*>(fName));
	fName = strdup(fi.fName);
	return *this;
}


#endif	// FILEHANDLE_H

