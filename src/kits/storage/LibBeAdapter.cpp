// LibBeAdapter.cpp

#include <dirent.h>
#include <errno.h>
#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <fs_query.h>
#include <Path.h>

#include <syscalls.h>

enum {
	FD_TYPE_UNKNOWN,
	FD_TYPE_DIR,
	FD_TYPE_ATTR_DIR,
	FD_TYPE_QUERY,
};

static const int kFDTableSlotCount = 1024;
static uint8 sFDTable[kFDTableSlotCount];

static struct InitDirFDTable {
	InitDirFDTable()
	{
		memset(sFDTable, FD_TYPE_UNKNOWN, sizeof(sFDTable));
	}
} sInitDirFDTable;

// _kern_entry_ref_to_path
extern "C"
status_t 
_kern_entry_ref_to_path(dev_t device, ino_t inode, const char *leaf,
	char *userPath, size_t pathLength)
{
	// check buffer
	if (!userPath)
		return B_BAD_VALUE;
	// construct an entry_ref
	if (!leaf)
		leaf = ".";
	entry_ref ref(device, inode, leaf);
	// get the path
	BPath path;
	status_t error = path.SetTo(&ref);
	if (error != B_OK)
		return error;
	// copy the path into the buffer
	if (strlcpy(userPath, path.Path(), pathLength) >= pathLength)
		return B_BUFFER_OVERFLOW;
	return B_OK;
}


// Syscall mapping between R5 and us

// private libroot.so functions

extern "C" status_t _kclosedir_(int fd);

extern "C" status_t _kclose_attr_dir_(int fd);

extern "C" status_t _kclose_query_(int fd);

extern "C" ssize_t _kreadlink_(int fd, const char *path, char *buffer,
							   size_t bufferSize);

extern "C" status_t _krstat_(int fd, const char *path, struct stat *st,
							 int16 unknown);

extern "C" status_t _kwstat_(int fd, const char *path, const struct stat *st,
							 uint32 mask, uint16 unknown);

extern "C" status_t _kwfsstat_(dev_t device, const struct fs_info *info,
							   long mask);

extern "C" status_t _klock_node_(int fd);

extern "C" status_t _kunlock_node_(int fd);

extern "C" status_t _kstart_watching_vnode_(dev_t device, ino_t node,
											uint32 flags, port_id port,
											int32 handlerToken);

extern "C" status_t _kstop_watching_vnode_(dev_t device, ino_t node,
										   port_id port, int32 handlerToken);


extern "C" status_t _kstop_notifying_(port_id port, int32 handlerToken);


status_t
_kern_write_fs_info(dev_t device, const struct fs_info *info, int mask)
{
	return _kwfsstat_(device, info, mask);
}


status_t
_kern_stop_notifying(port_id port, uint32 token)
{
	return _kstop_notifying_(port, token);
}


status_t
_kern_start_watching(dev_t device, ino_t node, uint32 flags,
	port_id port, uint32 token)
{
	return _kstart_watching_vnode_(device, node, flags, port, token);
}

			
status_t
_kern_stop_watching(dev_t device, ino_t node, uint32 flags,
	port_id port, uint32 token)
{
	(void)flags;
	return _kstop_watching_vnode_(device, node, port, token);
}

// init_dir
static
status_t
init_dir(BDirectory &dir, int fd)
{
	// get a node_ref for the dir
	struct stat st;
	if (fstat(fd, &st) < 0)
		return errno;
	node_ref ref;
	ref.device = st.st_dev;
	ref.node = st.st_ino;
	// init the dir
	return dir.SetTo(&ref);
}

// open_dir_hack
static
int
open_dir_hack(const char *path)
{
	DIR *dirHandle = opendir(path);
	if (!dirHandle)
		return errno;
	int dirFD = dirHandle->fd;
	if (dirFD < 0 || dirFD >= kFDTableSlotCount) {
		closedir(dirHandle);
		return B_ERROR;
	}
	free(dirHandle);
	sFDTable[dirFD] = FD_TYPE_DIR;
	return dirFD;
}

// get_path
static
int
get_path(int fd, const char *relPath, BPath &path)
{
	if (fd < 0 || relPath && relPath[0] == '/')
		return path.SetTo(relPath);
	BDirectory dir;
	status_t error = init_dir(dir, fd);
	if (error != B_OK)
		return error;
	return path.SetTo(&dir, relPath);
}

// _kern_open
int
_kern_open(int fd, const char *path, int omode)
{
	status_t error;
	BPath fullPath;
	if (!path)
		path = ".";
	if (fd >= 0) {
		// construct the full path
		error = get_path(fd, path, fullPath);
		if (error != B_OK)
			return error;
		path = fullPath.Path();
	}
	// open the file
	int result = open(path, omode);
	if (result < 0)
		return errno;
	return result;
}


// _kern_open_entry_ref
extern "C"
int
_kern_open_entry_ref(dev_t device, ino_t inode, const char *name, int omode)
{
	BPath fullPath;
	node_ref ref;
	ref.device = device;
	ref.node = inode;
	// init the dir and get the path
	BDirectory dir;
	status_t error = dir.SetTo(&ref);
	if (error != B_OK)
		return error;
	error = fullPath.SetTo(&dir, name);
	if (error != B_OK)
		return error;
	// open the file
	int fd = open(fullPath.Path(), omode);
	if (fd < 0)
		return errno;
	return fd;
}

// _kern_open_dir
extern "C"
int
_kern_open_dir(int fd, const char *path)
{
	status_t error;
	BPath fullPath;
	if (fd >= 0) {
		if (!path)
			return _kern_dup(fd);
		// construct the full path
		error = get_path(fd, path, fullPath);
		if (error != B_OK)
			return error;
		path = fullPath.Path();
	}
	// open the dir
	return open_dir_hack(path);
}

// _kern_open_dir_entry_ref
extern "C"
int
_kern_open_dir_entry_ref(dev_t device, ino_t inode, const char *path)
{
	BPath fullPath;
	node_ref ref;
	ref.device = device;
	ref.node = inode;
	// init the dir and get the path
	BDirectory dir;
	status_t error = dir.SetTo(&ref);
	if (error != B_OK)
		return error;
	error = fullPath.SetTo(&dir, path);
	if (error != B_OK)
		return error;
	path = fullPath.Path();
	// open the dir
	return open_dir_hack(path);
}

// _kern_open_parent_dir
extern "C"
int
_kern_open_parent_dir(int fd, char *name, size_t pathLength)
{
	if (fd < 0)
		return B_BAD_VALUE;
	// get the dir
	BDirectory dir;
	status_t error = init_dir(dir, fd);
	if (error != B_OK)
		return error;
	// get an entry
	BEntry entry;
	error = entry.SetTo(&dir, NULL);
	if (error != B_OK)
		return error;
	// copy back the path
	if (name) {
		char tmpName[B_FILE_NAME_LENGTH];
		error = entry.GetName(tmpName);
		if (error != B_OK)
			return error;
		if (strlcpy(name, tmpName, pathLength) >= pathLength)
			return B_BUFFER_OVERFLOW;
	}
	// get the parent dir path
	BPath path;
	error = path.SetTo(&dir, "..");
	if (error != B_OK)
		return error;
	// open the dir
	return open_dir_hack(path.Path());
}

// _kern_open_query
extern "C"
int
_kern_open_query(dev_t device, const char *query, uint32 flags, port_id port,
	int32 token)
{
	// check params
	if (!query || device < 0)
		return B_BAD_VALUE;
	if (flags & B_LIVE_QUERY && port < 0)
		return B_BAD_VALUE;
	// open query
	DIR *dirHandle;
	if (flags & B_LIVE_QUERY)
		dirHandle = fs_open_live_query(device, query, flags, port, token);
	else
		dirHandle = fs_open_query(device, query, flags);
	if (!dirHandle)
		return errno;
	// get FD and return result
	int dirFD = dirHandle->fd;
	if (dirFD < 0 || dirFD >= kFDTableSlotCount) {
		fs_close_query(dirHandle);
		return B_ERROR;
	}
	free(dirHandle);
	sFDTable[dirFD] = FD_TYPE_QUERY;
	return dirFD;
}

// _kern_create_dir
extern "C"
status_t
_kern_create_dir(int fd, const char *path, int perms)
{
	if (!path)
		return B_BAD_VALUE;
	status_t error;
	BPath fullPath;
	if (fd >= 0) {
		// construct the full path
		error = get_path(fd, path, fullPath);
		if (error != B_OK)
			return error;
		path = fullPath.Path();
	}
	// create the dir
	return (mkdir(path, perms) < 0 ? errno : B_OK);
}

// _kern_create_symlink
extern "C"
status_t
_kern_create_symlink(int fd, const char *path, const char *toPath, int mode)
{
	(void)mode;
	if (!path)
		return B_BAD_VALUE;
	status_t error;
	BPath fullPath;
	if (fd >= 0) {
		// construct the full path
		error = get_path(fd, path, fullPath);
		if (error != B_OK)
			return error;
		path = fullPath.Path();
	}
	// create the symlink
	return (symlink(toPath, path) < 0 ? errno : B_OK);
}

// _kern_close
extern "C"
status_t
_kern_close(int fd)
{
	if (fd >= 0 && fd < kFDTableSlotCount && sFDTable[fd] != FD_TYPE_UNKNOWN) {
		status_t error;
		switch (sFDTable[fd]) {
			case FD_TYPE_DIR:
				error = _kclosedir_(fd);
				break;
			case FD_TYPE_ATTR_DIR:
				error = _kclose_attr_dir_(fd);
				break;
			case FD_TYPE_QUERY:
				error = _kclose_query_(fd);
				break;
			default:
				error = B_BAD_VALUE;
				break;
		}
		sFDTable[fd] = FD_TYPE_UNKNOWN;
		if (error != B_OK)
			return error;
	} else if (close(fd) < 0)
		return errno;
	return B_OK;
}

// _kern_dup
extern "C"
int
_kern_dup(int fd)
{
	int clonedFD = dup(fd);
	if (clonedFD < 0)
		return errno;
	if (fd >= 0 && fd < kFDTableSlotCount && sFDTable[fd] != FD_TYPE_UNKNOWN
		&& clonedFD >= 0 && clonedFD < kFDTableSlotCount) {
		sFDTable[clonedFD] = sFDTable[fd];
	}
	return clonedFD;
}

// _kern_read
extern "C"
ssize_t
_kern_read(int fd, off_t pos, void *buffer, size_t bufferSize)
{
	if (!buffer)
		return B_BAD_VALUE;
	ssize_t result;
	if (pos == -1)
		result = read(fd, buffer, bufferSize);
	else
		result = read_pos(fd, pos, buffer, bufferSize);
	return (result < 0 ? errno : result);
}

// _kern_write
extern "C"
ssize_t
_kern_write(int fd, off_t pos, const void *buffer, size_t bufferSize)
{
	if (!buffer)
		return B_BAD_VALUE;
	ssize_t result;
	if (pos == -1)
		result = write(fd, buffer, bufferSize);
	else
		result = write_pos(fd, pos, buffer, bufferSize);
	return (result < 0 ? errno : result);
}

// _kern_seek
extern "C"
off_t
_kern_seek(int fd, off_t pos, int seekType)
{
	off_t result = lseek(fd, pos, seekType);
	return (result < 0 ? errno : result);
}

// _kern_read_dir
extern "C"
ssize_t
_kern_read_dir(int fd, struct dirent *buffer, size_t bufferSize,
	uint32 maxCount)
{
	if (fd < 0 || !buffer)
		return B_BAD_VALUE;
	if (fd >= kFDTableSlotCount || sFDTable[fd] == FD_TYPE_UNKNOWN)
		return B_BAD_VALUE;
	if (maxCount < 1)
		return 0;
	char tmpBuffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	DIR *dirDir = (DIR*)tmpBuffer;
	dirDir->fd = fd;
	dirent *entry = NULL;
	switch (sFDTable[fd]) {
		case FD_TYPE_DIR:
			entry = readdir(dirDir);
			break;
		case FD_TYPE_ATTR_DIR:
			entry = fs_read_attr_dir(dirDir);
			break;
		case FD_TYPE_QUERY:
			entry = fs_read_query(dirDir);
			break;
	}
	if (!entry)
		return 0;
	// Don't trust entry->d_reclen.
	// Unlike stated in BeBook::BEntryList, the value is not the length
	// of the whole structure, but only of the name. Some FSs count
	// the terminating '\0', others don't.
	// So we calculate the size ourselves (including the '\0'):
	size_t entryLen = entry->d_name + strlen(entry->d_name) + 1
					  - (char*)entry;
	if (bufferSize >= entryLen) {
		memcpy(buffer, entry, entryLen);
		return 1;
	} else	// buffer too small
		return B_BUFFER_OVERFLOW;
}

// _kern_rewind_dir
extern "C"
status_t
_kern_rewind_dir(int fd)
{
	if (fd < 0 || fd >= kFDTableSlotCount || sFDTable[fd] == FD_TYPE_UNKNOWN)
		return B_BAD_VALUE;
	char tmpBuffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	DIR *dirDir = (DIR*)tmpBuffer;
	dirDir->fd = fd;
	switch (sFDTable[fd]) {
		case FD_TYPE_DIR:
			rewinddir(dirDir);
			break;
		case FD_TYPE_ATTR_DIR:
			fs_rewind_attr_dir(dirDir);
			break;
		case FD_TYPE_QUERY:
			return B_BAD_VALUE;
			break;
	}
	return B_OK;
}

// _kern_read_link
extern "C"
ssize_t
_kern_read_link(int fd, const char *path, char *buffer, size_t bufferSize)
{
	ssize_t result = _kreadlink_(fd, path, buffer, bufferSize);
	if (result >= 0)
		buffer[result] = '\0';

	return result;
}

// _kern_unlink
extern "C"
status_t
_kern_unlink(int fd, const char *relPath)
{
	BPath path;
	status_t error = get_path(fd, relPath, path);
	if (error != B_OK)
		return error;
	if (unlink(path.Path()) < 0) {
		error = errno;
		if (error == B_IS_A_DIRECTORY) {
			if (rmdir(path.Path()) < 0)
				return errno;
		} else
			return error;
	}
	return B_OK;
}

// _kern_rename
extern "C"
status_t
_kern_rename(int oldDir, const char *oldRelPath, int newDir,
	const char *newRelPath)
{
	// get old path
	BPath oldPath;
	status_t error = get_path(oldDir, oldRelPath, oldPath);
	if (error != B_OK)
		return error;
	// get new path
	BPath newPath;
	error = get_path(newDir, newRelPath, newPath);
	if (error != B_OK)
		return error;
	// rename
	if (rename(oldPath.Path(), newPath.Path()) < 0)
		return errno;
	return B_OK;
}


// _kern_read_stat
extern "C"
status_t
_kern_read_stat(int fd, const char *path, bool traverseLink, struct stat *st,
	size_t statSize)
{
	if (traverseLink)
		return B_ERROR; // unsupported
	return _krstat_(fd, path, st, 0);
}

// _kern_write_stat
extern "C"
status_t
_kern_write_stat(int fd, const char *path, bool traverseLink,
	const struct stat *st, size_t statSize, int statMask)
{
	if (traverseLink)
		return B_ERROR; // unsupported
	return _kwstat_(fd, path, st, statMask, 0);
}

// _kern_lock_node
extern "C"
status_t
_kern_lock_node(int fd)
{
	return _klock_node_(fd);
}

// _kern_unlock_node
extern "C"
status_t
_kern_unlock_node(int fd)
{
	return _kunlock_node_(fd);
}

// _kern_fsync
extern "C"
status_t
_kern_fsync(int fd)
{
	return (fsync(fd) < 0) ? errno : B_OK ;
}

// _kern_open_attr_dir
extern "C"
int
_kern_open_attr_dir(int fd, const char *path)
{
	if (fd < 0 && !path)
		return B_BAD_VALUE;
	if (fd >= 0 && path)
		return B_ERROR; // unsupported
	DIR *dirHandle = (fd >= 0 ? fs_fopen_attr_dir(fd) : fs_open_attr_dir(path));
	if (!dirHandle)
		return errno;
	int dirFD = dirHandle->fd;
	if (dirFD < 0 || dirFD >= kFDTableSlotCount) {
		fs_close_attr_dir(dirHandle);
		return B_ERROR;
	}
	free(dirHandle);
	sFDTable[dirFD] = FD_TYPE_ATTR_DIR;
	return dirFD;
}

// _kern_remove_attr
extern "C"
status_t
_kern_remove_attr(int fd, const char *name)
{
	return fs_remove_attr(fd, name) == -1 ? errno : B_OK ;
}

// _kern_rename_attr
extern "C"
status_t
_kern_rename_attr(int fromFile, const char *fromName, int toFile,
	const char *toName)
{
	status_t error = (fromName && toName ? B_OK : B_BAD_VALUE);
	// Figure out how much data there is
	attr_info info;
	if (error == B_OK) {
		if (fs_stat_attr(fromFile, fromName, &info) < 0)
			error = B_BAD_VALUE;	// This is what R5::BNode returns...		
	}
	// Alloc a buffer
	char *data = NULL;
	if (error == B_OK) {
		// alloc at least one byte
		data = new(nothrow) char[info.size >= 1 ? info.size : 1];
		if (data == NULL)
			error = B_NO_MEMORY;		
	}
	// Read in the data
	if (error == B_OK) {
		ssize_t size = fs_read_attr(fromFile, fromName, info.type, 0, data,
			info.size);
		if (size != info.size) {
			if (size < 0)
				error = errno;
			else
				error = B_ERROR;
		}		
	}
	// Write it to the new attribute
	if (error == B_OK) {
		ssize_t size = 0;
		if (info.size > 0)
			size = fs_write_attr(toFile, toName, info.type, 0, data, info.size);
		if (size != info.size) {
			if (size < 0)
				error = errno;
			else
				error = B_ERROR;
		}	
	}
	// free the buffer
	if (data)
		delete[] data;
	// Remove the old attribute
	if (error == B_OK)
		error = _kern_remove_attr(fromFile, fromName);
	return error;
}

