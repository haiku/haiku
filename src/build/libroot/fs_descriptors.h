/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef FS_DESCRIPTORS_H
#define FS_DESCRIPTORS_H

#include <dirent.h>

#include <string>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include "NodeRef.h"

using std::string;

struct stat;

namespace BPrivate {

// Descriptor
struct Descriptor {
	int fd;

	virtual ~Descriptor();

	virtual status_t Close() = 0;
	virtual status_t Dup(Descriptor *&clone) = 0;
	virtual status_t GetStat(bool traverseLink, struct stat *st) = 0;

	virtual bool IsSystemFD() const;
	virtual status_t GetPath(string& path) const;

	virtual status_t GetNodeRef(NodeRef &ref);
};

// FileDescriptor
struct FileDescriptor : Descriptor {
	FileDescriptor(int fd);
	virtual ~FileDescriptor();

	virtual status_t Close();
	virtual status_t Dup(Descriptor *&clone);
	virtual status_t GetStat(bool traverseLink, struct stat *st);

	virtual bool IsSystemFD() const;
};

// DirectoryDescriptor
struct DirectoryDescriptor : Descriptor {
	DIR *dir;
	NodeRef ref;

	DirectoryDescriptor(DIR *dir, const NodeRef &ref);
	virtual ~DirectoryDescriptor();

	virtual status_t Close();
	virtual status_t Dup(Descriptor *&clone);
	virtual status_t GetStat(bool traverseLink, struct stat *st);

	virtual status_t GetNodeRef(NodeRef &ref);
};

// SymlinkDescriptor
struct SymlinkDescriptor : Descriptor {
	string path;

	SymlinkDescriptor(const char *path);

	virtual status_t Close();
	virtual status_t Dup(Descriptor *&clone);
	virtual status_t GetStat(bool traverseLink, struct stat *st);

	virtual status_t GetPath(string& path) const;
};

// AttributeDescriptor
struct AttributeDescriptor : Descriptor {
								AttributeDescriptor(int fileFD,
									const char* attribute, uint32 type,
									int openMode);
	virtual						~AttributeDescriptor();

			status_t			Init();
			status_t			Write(off_t offset, const void* buffer,
									size_t bufferSize);

			int					FileFD() const		{ return fFileFD; }
			const char*			Attribute() const	{ return fAttribute; }
			uint32				Type() const		{ return fType; }
			int					OpenMode() const	{ return fOpenMode; }

	virtual	status_t			Close();
	virtual	status_t			Dup(Descriptor*& clone);
	virtual	status_t			GetStat(bool traverseLink, struct stat* st);

private:
			int					fFileFD;
			char				fAttribute[B_ATTR_NAME_LENGTH];
			uint32				fType;
			int					fOpenMode;
			uint8*				fData;
			size_t				fDataSize;
};

// AttrDirDescriptor
struct AttrDirDescriptor : DirectoryDescriptor {

	AttrDirDescriptor(DIR *dir, const NodeRef &ref);
	virtual ~AttrDirDescriptor();

	virtual status_t Close();
	virtual status_t Dup(Descriptor *&clone);
	virtual status_t GetStat(bool traverseLink, struct stat *st);

	virtual status_t GetNodeRef(NodeRef &ref);
};


Descriptor*	get_descriptor(int fd);
int			add_descriptor(Descriptor *descriptor);
status_t	delete_descriptor(int fd);
bool		is_unknown_or_system_descriptor(int fd);

}	// namespace BPrivate

#endif	// FS_DESCRIPTORS_H
