//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file OffsetFile.h
	OffsetFile interface declaration.
*/

#ifndef _OFFSET_FILE_H
#define _OFFSET_FILE_H

#include <DataIO.h>
#include <File.h>

namespace BPrivate {
namespace Storage {

/*!
	\class OffsetFile
	\brief Provides access to a file skipping a certain amount of bytes at
	the beginning.
	
	This class implements the BPositionIO interface to provide access to the
	data of a file past a certain offset. This is very handy e.g. for dealing
	with resources, as they always reside at the end of a file, but may start
	at arbitrary offsets.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/
class OffsetFile : public BPositionIO {
public:
	OffsetFile();
	OffsetFile(BFile *file, off_t offset);
	virtual ~OffsetFile();

	status_t SetTo(BFile *file, off_t offset);
	void Unset();
	status_t InitCheck() const;

	BFile *File() const;

	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	ssize_t WriteAt(off_t pos, const void *buffer,
								size_t size);
	off_t Seek(off_t position, uint32 seekMode);
	off_t Position() const;

	status_t SetSize(off_t size);
	status_t GetSize(off_t *size) const;

	off_t Offset() const;

private:
	BFile*				fFile;
	off_t				fOffset;
	off_t				fCurrentPosition;
};

};	// namespace Storage
};	// namespace BPrivate

#endif	// _OFFSET_FILE_H


