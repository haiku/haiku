//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file File.h
	BFile interface declaration.
*/
#ifndef _FILE_H
#define _FILE_H


#include <DataIO.h>
#include <Node.h>


/*!
	\class BFile
	\brief BFile is a wrapper class for common operations on files providing
	access to the file's content data and its attributes.

	A BFile represents a file in some file system. It implements the
	BPositionIO interface and thus the methods to read from and write to the
	file, and is derived of BNode to provide access to the file's attributes.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>

	\version 0.0.0
*/
class BFile : public BNode, public BPositionIO {
public:
	BFile();
	BFile(const BFile &file);
	BFile(const entry_ref *ref, uint32 openMode);
	BFile(const BEntry *entry, uint32 openMode);
	BFile(const char *path, uint32 openMode);
	BFile(const BDirectory *dir, const char *path, uint32 openMode);
	virtual ~BFile();

	status_t SetTo(const entry_ref *ref, uint32 openMode);
	status_t SetTo(const BEntry *entry, uint32 openMode);
	status_t SetTo(const char *path, uint32 openMode);
	status_t SetTo(const BDirectory *dir, const char *path, uint32 openMode);

	bool IsReadable() const;
	bool IsWritable() const;

	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t ReadAt(off_t location, void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t location, const void *buffer, size_t size);

	virtual off_t Seek(off_t offset, uint32 seekMode);
	virtual off_t Position() const;

	virtual status_t SetSize(off_t size);
	virtual	status_t GetSize(off_t* size) const;

	BFile &operator=(const BFile &file);

private:
	virtual void _PhiloFile1();
	virtual void _PhiloFile2();
	virtual void _PhiloFile3();
	virtual void _PhiloFile4();
	virtual void _PhiloFile5();
	virtual void _PhiloFile6();

	uint32 _reservedData[8];

private:
	int get_fd() const;
	virtual void close_fd();

private:
	//! The file's open mode.
	uint32 fMode;
};


#endif	// _FILE_H
