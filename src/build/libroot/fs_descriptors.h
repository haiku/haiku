#ifndef FS_DESCRIPTORS_H
#define FS_DESCRIPTORS_H

#include <dirent.h>

#include <string>

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
	
	virtual status_t GetNodeRef(NodeRef &ref);
};

// FileDescriptor
struct FileDescriptor : Descriptor {
	FileDescriptor(int fd);
	virtual ~FileDescriptor();
	
	virtual status_t Close();
	virtual status_t Dup(Descriptor *&clone);
	virtual status_t GetStat(bool traverseLink, struct stat *st);
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

}	// namespace BPrivate

#endif	// FS_DESCRIPTORS_H
