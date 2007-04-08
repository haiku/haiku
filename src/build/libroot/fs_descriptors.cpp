
#ifdef BUILDING_FS_SHELL
#	include "compat.h"
#	define B_OK			0
#	define B_FILE_ERROR	EBADF
#else
#	include <BeOSBuildCompatibility.h>
#endif

#include "fs_descriptors.h"

#include <map>

#include <stdio.h>
#include <unistd.h>

#include <fs_attr.h>

#include "fs_impl.h"

using std::map;

static const int kVirtualDescriptorStart = 10000;

typedef map<int, BPrivate::Descriptor*> DescriptorMap;
static DescriptorMap sDescriptors;

namespace BPrivate {


// #pragma mark - Descriptor


// constructor
Descriptor::~Descriptor()
{
}

// IsSystemFD
bool
Descriptor::IsSystemFD() const
{
	return false;
}

// GetPath
status_t
Descriptor::GetPath(string& path) const
{
	return get_path(fd, NULL, path);
}

// GetNodeRef
status_t
Descriptor::GetNodeRef(NodeRef &ref)
{
	struct stat st;
	status_t error = GetStat(false, &st);
	if (error != B_OK)
		return error;

	ref = NodeRef(st);

	return B_OK;
}


// #pragma mark - FileDescriptor


// constructor
FileDescriptor::FileDescriptor(int fd)
{
	this->fd = fd;
}

// destructor
FileDescriptor::~FileDescriptor()
{
	Close();
}

// Close
status_t
FileDescriptor::Close()
{
	if (fd >= 0) {
		int oldFD = fd;
		fd = -1;
		if (close(oldFD) < 0)
			return errno;
	}

	return B_OK;			
}

// Dup
status_t
FileDescriptor::Dup(Descriptor *&clone)
{
	int dupFD = dup(fd);
	if (dupFD < 0)
		return errno;
	
	clone = new FileDescriptor(dupFD);
	return B_OK;
}

// GetStat
status_t
FileDescriptor::GetStat(bool traverseLink, struct stat *st)
{
	if (fstat(fd, st) < 0)
		return errno;
	return B_OK;
}

// IsSystemFD
bool
FileDescriptor::IsSystemFD() const
{
	return true;
}


// #pragma mark - DirectoryDescriptor


// constructor
DirectoryDescriptor::DirectoryDescriptor(DIR *dir, const NodeRef &ref)
{
	this->dir = dir;
	this->ref = ref;
}

// destructor
DirectoryDescriptor::~DirectoryDescriptor()
{
	Close();
}

// Close
status_t
DirectoryDescriptor::Close()
{
	if (dir) {
		DIR *oldDir = dir;
		dir = NULL;
		if (closedir(oldDir) < 0)
			return errno;
	}

	return B_OK;			
}

// Dup
status_t
DirectoryDescriptor::Dup(Descriptor *&clone)
{
	string path;
	status_t error = get_path(fd, NULL, path);
	if (error != B_OK)
		return error;

	DIR *dupDir = opendir(path.c_str());
	if (!dupDir)
		return errno;
	
	clone = new DirectoryDescriptor(dupDir, ref);
	return B_OK;
}

// GetStat
status_t
DirectoryDescriptor::GetStat(bool traverseLink, struct stat *st)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, NULL, realPath);
	if (error != B_OK)
		return error;

	// stat
	int result;
	result = stat(realPath.c_str(), st);

	if (result < 0)
		return errno;

	return B_OK;
}

// GetNodeRef
status_t
DirectoryDescriptor::GetNodeRef(NodeRef &ref)
{
	ref = this->ref;

	return B_OK;
}


// #pragma mark - SymlinkDescriptor


// constructor
SymlinkDescriptor::SymlinkDescriptor(const char *path)
{
	this->path = path;
}

// Close
status_t
SymlinkDescriptor::Close()
{
	return B_OK;			
}

// Dup
status_t
SymlinkDescriptor::Dup(Descriptor *&clone)
{
	clone = new SymlinkDescriptor(path.c_str());
	return B_OK;
}

// GetStat
status_t
SymlinkDescriptor::GetStat(bool traverseLink, struct stat *st)
{
	// stat
	int result;
	if (traverseLink)
		result = stat(path.c_str(), st);
	else
		result = lstat(path.c_str(), st);

	if (result < 0)
		return errno;

	return B_OK;
}

// GetPath
status_t
SymlinkDescriptor::GetPath(string& path) const
{
	path = this->path;
	return B_OK;
}


// #pragma mark - AttrDirDescriptor


// constructor
AttrDirDescriptor::AttrDirDescriptor(DIR *dir, const NodeRef &ref)
	: DirectoryDescriptor(dir, ref)
{
}

// destructor
AttrDirDescriptor::~AttrDirDescriptor()
{
	Close();
}

// Close
status_t
AttrDirDescriptor::Close()
{
	if (dir) {
		DIR *oldDir = dir;
		dir = NULL;
		if (fs_close_attr_dir(oldDir) < 0)
			return errno;
	}

	return B_OK;			
}

// Dup
status_t
AttrDirDescriptor::Dup(Descriptor *&clone)
{
	// we don't allow dup()int attr dir descriptors
	return B_FILE_ERROR;
}

// GetStat
status_t
AttrDirDescriptor::GetStat(bool traverseLink, struct stat *st)
{
	// we don't allow stat()int attr dir descriptors
	return B_FILE_ERROR;
}

// GetNodeRef
status_t
AttrDirDescriptor::GetNodeRef(NodeRef &ref)
{
	ref = this->ref;

	return B_OK;
}


// get_descriptor
Descriptor *
get_descriptor(int fd)
{
	DescriptorMap::iterator it = sDescriptors.find(fd);
	if (it == sDescriptors.end())
		return NULL;
	return it->second;
}

// add_descriptor
int
add_descriptor(Descriptor *descriptor)
{
	int fd = -1;
	if (FileDescriptor *file = dynamic_cast<FileDescriptor*>(descriptor)) {
		fd = file->fd;
	} else {
		// find a free slot
		for (fd = kVirtualDescriptorStart;
			sDescriptors.find(fd) != sDescriptors.end();
			fd++) {
		}
	}

	sDescriptors[fd] = descriptor;
	descriptor->fd = fd;

	return fd;
}

// delete_descriptor
status_t
delete_descriptor(int fd)
{
	DescriptorMap::iterator it = sDescriptors.find(fd);
	if (it == sDescriptors.end())
		return B_FILE_ERROR;

	status_t error = it->second->Close();
	delete it->second;
	sDescriptors.erase(it);
	
	return error;
}

} // namespace BPrivate
