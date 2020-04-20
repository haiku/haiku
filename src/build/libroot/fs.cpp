/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <BeOSBuildCompatibility.h>

#include "fs_impl.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <map>
#include <string>

#include <fs_attr.h>
#include <NodeMonitor.h>	// for B_STAT_* flags
#include <syscalls.h>

#include "fs_descriptors.h"
#include "NodeRef.h"
#include "remapped_functions.h"

#if defined(HAIKU_HOST_PLATFORM_FREEBSD)
#	include "fs_freebsd.h"
#endif


using namespace std;
using namespace BPrivate;


#if defined(HAIKU_HOST_PLATFORM_FREEBSD)
#	define haiku_host_platform_read		haiku_freebsd_read
#	define haiku_host_platform_write	haiku_freebsd_write
#	define haiku_host_platform_readv	haiku_freebsd_readv
#	define haiku_host_platform_writev	haiku_freebsd_writev
#	define HAIKU_HOST_STAT_ATIM(x)		((x).st_atimespec)
#	define HAIKU_HOST_STAT_MTIM(x)		((x).st_mtimespec)
#elif defined(HAIKU_HOST_PLATFORM_DARWIN)
#	define haiku_host_platform_read		read
#	define haiku_host_platform_write	write
#	define haiku_host_platform_readv	readv
#	define haiku_host_platform_writev	writev
#	define HAIKU_HOST_STAT_ATIM(x)		((x).st_atimespec)
#	define HAIKU_HOST_STAT_MTIM(x)		((x).st_mtimespec)
#else
#	define haiku_host_platform_read		read
#	define haiku_host_platform_write	write
#	define haiku_host_platform_readv	readv
#	define haiku_host_platform_writev	writev
#	define HAIKU_HOST_STAT_ATIM(x)		((x).st_atim)
#	define HAIKU_HOST_STAT_MTIM(x)		((x).st_mtim)
#endif

#define RETURN_AND_SET_ERRNO(err)			\
	do {									\
		__typeof(err) __result = (err);		\
		if (__result < 0) {					\
			errno = __result;				\
			return -1;						\
		}									\
		return __result;					\
	} while (0)


#if defined(_HAIKU_BUILD_NO_FUTIMENS) || defined(_HAIKU_BUILD_NO_FUTIMENS)

template<typename File>
static int
utimes_helper(File& file, const struct timespec times[2])
{
	if (times == NULL)
		return file.SetTimes(NULL);

	timeval timeBuffer[2];
	timeBuffer[0].tv_sec = times[0].tv_sec;
	timeBuffer[0].tv_usec = times[0].tv_nsec / 1000;
	timeBuffer[1].tv_sec = times[1].tv_sec;
	timeBuffer[1].tv_usec = times[1].tv_nsec / 1000;

	if (times[0].tv_nsec == UTIME_OMIT || times[1].tv_nsec == UTIME_OMIT) {
		struct stat st;
		if (file.GetStat(st) != 0)
			return -1;

		if (times[0].tv_nsec == UTIME_OMIT && times[1].tv_nsec == UTIME_OMIT)
			return 0;

		if (times[0].tv_nsec == UTIME_OMIT) {
			timeBuffer[0].tv_sec = st.st_atimespec.tv_sec;
			timeBuffer[0].tv_usec = st.st_atimespec.tv_nsec / 1000;
		}

		if (times[1].tv_nsec == UTIME_OMIT) {
			timeBuffer[1].tv_sec = st.st_mtimespec.tv_sec;
			timeBuffer[1].tv_usec = st.st_mtimespec.tv_nsec / 1000;
		}
	}

	if (times[0].tv_nsec == UTIME_NOW || times[1].tv_nsec == UTIME_NOW) {
		timeval now;
		gettimeofday(&now, NULL);

		if (times[0].tv_nsec == UTIME_NOW)
			timeBuffer[0] = now;

		if (times[1].tv_nsec == UTIME_NOW)
			timeBuffer[1] = now;
	}

	return file.SetTimes(timeBuffer);	
}

#endif	// _HAIKU_BUILD_NO_FUTIMENS || _HAIKU_BUILD_NO_FUTIMENS


#ifdef _HAIKU_BUILD_NO_FUTIMENS

struct FDFile {
	FDFile(int fd)
		:
		fFD(fd)
	{
	}

	int GetStat(struct stat& _st)
	{
		return fstat(fFD, &_st);
	}

	int SetTimes(const timeval times[2])
	{
		return futimes(fFD, times);
	}

private:
	int fFD;
};


int
futimens(int fd, const struct timespec times[2])
{
	FDFile file(fd);
	return utimes_helper(file, times);
}

#endif	// _HAIKU_BUILD_NO_FUTIMENS

#ifdef _HAIKU_BUILD_NO_UTIMENSAT

struct FDPathFile {
	FDPathFile(int fd, const char* path, int flag)
		:
		fFD(fd),
		fPath(path),
		fFlag(flag)
	{
	}

	int GetStat(struct stat& _st)
	{
		return fstatat(fFD, fPath, &_st, fFlag);
	}

	int SetTimes(const timeval times[2])
	{
		// TODO: fFlag (AT_SYMLINK_NOFOLLOW) is not supported here!
		return futimesat(fFD, fPath, times);
	}

private:
	int			fFD;
	const char*	fPath;
	int			fFlag;
};


int
utimensat(int fd, const char* path, const struct timespec times[2], int flag)
{
	FDPathFile file(fd, path, flag);
	return utimes_helper(file, times);
}

#endif	// _HAIKU_BUILD_NO_UTIMENSAT


static status_t get_path(dev_t device, ino_t node, const char *name,
	string &path);


// find_dir_entry
static status_t
find_dir_entry(DIR *dir, const char *path, NodeRef ref, string &name,
	bool skipDot)
{
	// find the entry
	bool found = false;
	while (dirent *entry = readdir(dir)) {
		if ((strcmp(entry->d_name, ".") == 0 && skipDot)
			|| strcmp(entry->d_name, "..") == 0) {
			// skip "." and ".."
		} else /*if (entry->d_ino == ref.node)*/ {
				// Note: Linux doesn't seem to translate dirent::d_ino of
				// mount points. Thus we always have to lstat().
			// We also need to compare the device, which is generally not
			// included in the dirent structure. Hence we lstat().
			string entryPath(path);
			entryPath += '/';
			entryPath += entry->d_name;
			struct stat st;
			if (lstat(entryPath.c_str(), &st) == 0) {
				if (NodeRef(st) == ref) {
					name = entry->d_name;
					found = true;
					break;
				}
			}
		}
	}

	if (!found)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}

// find_dir_entry
static status_t
find_dir_entry(const char *path, NodeRef ref, string &name, bool skipDot)
{
	// open dir
	DIR *dir = opendir(path);
	if (!dir)
		return errno;

	status_t error = find_dir_entry(dir, path, ref, name, skipDot);

	// close dir
	closedir(dir);

	return error;
}


static bool
guess_normalized_dir_path(string path, NodeRef ref, string& _normalizedPath)
{
	// We assume the CWD is normalized and hope that the directory is an
	// ancestor of it. We just chop off path components until we find a match or
	// hit root.
	char cwd[B_PATH_NAME_LENGTH];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		return false;

	while (cwd[0] == '/') {
		struct stat st;
		if (stat(cwd, &st) == 0) {
			if (st.st_dev == ref.device && st.st_ino == ref.node) {
				_normalizedPath = cwd;
				return true;
			}
		}

		*strrchr(cwd, '/') = '\0';
	}

	// TODO: If path is absolute, we could also try to work with that, though
	// the other way around -- trying prefixes until we hit a "." or ".."
	// component.

	return false;
}


// normalize_dir_path
static status_t
normalize_dir_path(string path, NodeRef ref, string &normalizedPath)
{
	// get parent path
	path += "/..";

	// stat the parent dir
	struct stat st;
	if (lstat(path.c_str(), &st) < 0)
		return errno;

	// root dir?
	NodeRef parentRef(st);
	if (parentRef == ref) {
		normalizedPath = "/";
		return 0;
	}

	// find the entry
	string name;
	status_t error = find_dir_entry(path.c_str(), ref, name, true)				;
	if (error != B_OK) {
		if (error != B_ENTRY_NOT_FOUND) {
			// We couldn't open the directory. This might be because we don't
			// have read permission. We're OK with not fully normalizing the
			// path and try to guess the path in this case. Note: We don't check
			// error for B_PERMISSION_DENIED, since opendir() may clobber the
			// actual kernel error code with something not helpful.
			if (guess_normalized_dir_path(path, ref, normalizedPath))
				return B_OK;
		}

		return error;
	}

	// recurse to get the parent dir path, if found
	error = normalize_dir_path(path, parentRef, normalizedPath);
	if (error != 0)
		return error;

	// construct the normalizedPath
	if (normalizedPath.length() > 1) // don't append "/", if parent is root
		normalizedPath += '/';
	normalizedPath += name;

	return 0;
}

// normalize_dir_path
static status_t
normalize_dir_path(const char *path, string &normalizedPath)
{
	// stat() the dir
	struct stat st;
	if (stat(path, &st) < 0)
		return errno;

	return normalize_dir_path(path, NodeRef(st), normalizedPath);
}

// normalize_entry_path
static status_t
normalize_entry_path(const char *path, string &normalizedPath)
{
	const char *dirPath = NULL;
	const char *leafName = NULL;

	string dirPathString;
	if (const char *lastSlash = strrchr(path, '/')) {
		// found a slash: decompose into dir path and leaf name
		leafName = lastSlash + 1;
		if (leafName[0] == '\0') {
			// slash is at the end: the whole path is a dir name
			leafName = NULL;
		} else {
			dirPathString = string(path, leafName - path);
			dirPath = dirPathString.c_str();
		}

	} else {
		// path contains no slash, so it is a path relative to the current dir
		dirPath = ".";
		leafName = path;
	}

	// catch special case: no leaf, or leaf is a directory
	if (!leafName || strcmp(leafName, ".") == 0 || strcmp(leafName, "..") == 0)
		return normalize_dir_path(path, normalizedPath);

	// normalize the dir path
	status_t error = normalize_dir_path(dirPath, normalizedPath);
	if (error != B_OK)
		return error;

	// append the leaf name
	if (normalizedPath.length() > 1) // don't append "/", if parent is root
		normalizedPath += '/';
	normalizedPath += leafName;

	return B_OK;
}


// #pragma mark -

typedef map<NodeRef, string> DirPathMap;
static DirPathMap sDirPathMap;

// get_path
static status_t
get_path(const NodeRef *ref, const char *name, string &path)
{
	if (!ref && !name)
		return B_BAD_VALUE;

	// no ref or absolute path
	if (!ref || (name && name[0] == '/')) {
		path = name;
		return B_OK;
	}

	// get the dir path
	if (ref) {
		DirPathMap::iterator it = sDirPathMap.find(*ref);
		if (it == sDirPathMap.end())
			return B_ENTRY_NOT_FOUND;

		path = it->second;

		// stat the path to check, if it is still valid
		struct stat st;
		if (lstat(path.c_str(), &st) < 0) {
			sDirPathMap.erase(it);
			return errno;
		}

		// compare the NodeRef
		if (NodeRef(st) != *ref) {
			sDirPathMap.erase(it);
			return B_ENTRY_NOT_FOUND;
		}

		// still a directory?
		if (!S_ISDIR(st.st_mode)) {
			sDirPathMap.erase(it);
			return B_NOT_A_DIRECTORY;
		}
	}

	// if there's a name, append it
	if (name) {
		path += '/';
		path += name;
	}

	return B_OK;
}

// get_path
status_t
BPrivate::get_path(int fd, const char *name, string &path)
{
	// get the node ref for the fd, if any, and the path part is not absolute
	if (fd >= 0 && !(name && name[0] == '/')) {
		// get descriptor
		Descriptor *descriptor = get_descriptor(fd);
		if (!descriptor)
			return B_FILE_ERROR;

		// Handle symlink descriptors here explicitly, so this function can be
		// used more flexibly.
		if (SymlinkDescriptor* symlinkDescriptor
				= dynamic_cast<SymlinkDescriptor*>(descriptor)) {
			path = symlinkDescriptor->path;
			if (name == NULL)
				return B_OK;
			path += '/';
			path += name;
			return B_OK;
		}

		// get node ref for the descriptor
		NodeRef ref;
		status_t error = descriptor->GetNodeRef(ref);
		if (error != B_OK)
			return error;

		return ::get_path(&ref, name, path);

	} else	// no descriptor or absolute path
		return ::get_path((NodeRef*)NULL, name, path);
}

// get_path
static status_t
get_path(dev_t device, ino_t directory, const char *name, string &path)
{
	NodeRef ref;
	ref.device = device;
	ref.node = directory;

	return get_path(&ref, name, path);
}

// add_dir_path
static void
add_dir_path(const char *path, const NodeRef &ref)
{
	// add the normalized path
	string normalizedPath;
	if (normalize_dir_path(path, normalizedPath) == B_OK)
		sDirPathMap[ref] = normalizedPath;
}


// #pragma mark -

// _kern_entry_ref_to_path
status_t
_kern_entry_ref_to_path(dev_t device, ino_t node, const char *leaf,
	char *userPath, size_t pathLength)
{
	// get the path
	string path;
	status_t error = get_path(device, node, leaf, path);
	if (error != B_OK)
		return error;

	// copy it back to the user buffer
	if (strlcpy(userPath, path.c_str(), pathLength) >= pathLength)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


// #pragma mark -

// _kern_create_dir
status_t
_kern_create_dir(int fd, const char *path, int perms)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	// mkdir
	if (mkdir(realPath.c_str(), perms) < 0)
		return errno;

	return B_OK;
}

// _kern_create_dir_entry_ref
status_t
_kern_create_dir_entry_ref(dev_t device, ino_t node, const char *name,
	int perms)
{
	// get a usable path
	string realPath;
	status_t error = get_path(device, node, name, realPath);
	if (error != B_OK)
		return error;

	// mkdir
	if (mkdir(realPath.c_str(), perms) < 0)
		return errno;

	return B_OK;
}

// open_dir
static int
open_dir(const char *path)
{
	// open the dir
	DIR *dir = opendir(path);
	if (!dir)
		return errno;

	// stat the entry
	struct stat st;
	if (stat(path, &st) < 0) {
		closedir(dir);
		return errno;
	}

	if (!S_ISDIR(st.st_mode)) {
		closedir(dir);
		return B_NOT_A_DIRECTORY;
	}

	// cache dir path
	NodeRef ref(st);
	add_dir_path(path, ref);

	// create descriptor
	DirectoryDescriptor *descriptor = new DirectoryDescriptor(dir, ref);
	return add_descriptor(descriptor);
}

// _kern_open_dir
int
_kern_open_dir(int fd, const char *path)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	return open_dir(realPath.c_str());
}

// _kern_open_dir_entry_ref
int
_kern_open_dir_entry_ref(dev_t device, ino_t node, const char *name)
{
	// get a usable path
	string realPath;
	status_t error = get_path(device, node, name, realPath);
	if (error != B_OK)
		return error;

	return open_dir(realPath.c_str());
}

// _kern_open_parent_dir
int
_kern_open_parent_dir(int fd, char *name, size_t nameLength)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, NULL, realPath);
	if (error != B_OK)
		return error;

	// stat the entry
	struct stat st;
	if (stat(realPath.c_str(), &st) < 0)
		return errno;

	if (!S_ISDIR(st.st_mode))
		return B_NOT_A_DIRECTORY;

	// get the entry name
	realPath += "/..";
	string entryName;
	error = find_dir_entry(realPath.c_str(), NodeRef(st),
		entryName, false);
	if (error != B_OK)
		return error;

	if (strlcpy(name, entryName.c_str(), nameLength) >= nameLength)
		return B_BUFFER_OVERFLOW;

	// open the parent directory

	return open_dir(realPath.c_str());
}

// _kern_read_dir
ssize_t
_kern_read_dir(int fd, struct dirent *buffer, size_t bufferSize,
	uint32 maxCount)
{
	if (maxCount <= 0)
		return B_BAD_VALUE;

	// get the descriptor
	DirectoryDescriptor *descriptor
		= dynamic_cast<DirectoryDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// get the next entry
	dirent *entry;
	errno = 0;
	if (dynamic_cast<AttrDirDescriptor*>(descriptor))
		entry = fs_read_attr_dir(descriptor->dir);
	else
		entry = readdir(descriptor->dir);
	if (!entry)
		return errno;

	// copy the entry
	int entryLen = &entry->d_name[strlen(entry->d_name) + 1] - (char*)entry;
	if (entryLen > (int)bufferSize)
		return B_BUFFER_OVERFLOW;

	memcpy(buffer, entry, entryLen);

	return 1;
}

// _kern_rewind_dir
status_t
_kern_rewind_dir(int fd)
{
	// get the descriptor
	DirectoryDescriptor *descriptor
		= dynamic_cast<DirectoryDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// rewind
	if (dynamic_cast<AttrDirDescriptor*>(descriptor))
		fs_rewind_attr_dir(descriptor->dir);
	else
		rewinddir(descriptor->dir);

	return B_OK;
}


// #pragma mark -

// open_file
static int
open_file(const char *path, int openMode, int perms)
{
	// stat the node
	bool exists = true;
	struct stat st;
	if (lstat(path, &st) < 0) {
		exists = false;
		if (!(openMode & O_CREAT))
			return errno;
	}

	Descriptor *descriptor;
	if (exists && S_ISLNK(st.st_mode) && (openMode & O_NOTRAVERSE) != 0) {
		// a symlink not to be followed: create a special descriptor
		// normalize path first
		string normalizedPath;
		status_t error = normalize_entry_path(path, normalizedPath);
		if (error != B_OK)
			return error;

		descriptor = new SymlinkDescriptor(normalizedPath.c_str());
	} else {
		// open the file
		openMode &= ~O_NOTRAVERSE;
		int newFD = open(path, openMode, perms);
		if (newFD < 0)
			return errno;

		descriptor = new FileDescriptor(newFD);
	}

	// cache path, if this is a directory
	if (exists && S_ISDIR(st.st_mode))
		add_dir_path(path, NodeRef(st));

	return add_descriptor(descriptor);
}

// _kern_open
int
_kern_open(int fd, const char *path, int openMode, int perms)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	return open_file(realPath.c_str(), openMode, perms);
}

// _kern_open_entry_ref
int
_kern_open_entry_ref(dev_t device, ino_t node, const char *name,
	int openMode, int perms)
{
	// get a usable path
	string realPath;
	status_t error = get_path(device, node, name, realPath);
	if (error != B_OK)
		return error;

	return open_file(realPath.c_str(), openMode, perms);
}

// _kern_seek
off_t
_kern_seek(int fd, off_t pos, int seekType)
{
	// get the descriptor
	FileDescriptor *descriptor
		= dynamic_cast<FileDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// seek
	off_t result = lseek(descriptor->fd, pos, seekType);
	if (result < 0)
		return errno;

	return result;
}

// _kern_read
ssize_t
_kern_read(int fd, off_t pos, void *buffer, size_t bufferSize)
{
	// get the descriptor
	FileDescriptor *descriptor
		= dynamic_cast<FileDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// seek
	if (pos != -1) {
		off_t result = lseek(descriptor->fd, pos, SEEK_SET);
		if (result < 0)
			return errno;
	}

	// read
	ssize_t bytesRead = haiku_host_platform_read(descriptor->fd, buffer,
		bufferSize);
	if (bytesRead < 0)
		return errno;

	return bytesRead;
}

// _kern_write
ssize_t
_kern_write(int fd, off_t pos, const void *buffer, size_t bufferSize)
{
	// get the descriptor
	FileDescriptor *descriptor
		= dynamic_cast<FileDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// seek
	if (pos != -1) {
		off_t result = lseek(descriptor->fd, pos, SEEK_SET);
		if (result < 0)
			return errno;
	}

	// read
	ssize_t bytesWritten = haiku_host_platform_write(descriptor->fd, buffer,
		bufferSize);
	if (bytesWritten < 0)
		return errno;

	return bytesWritten;
}

// _kern_close
status_t
_kern_close(int fd)
{
	return delete_descriptor(fd);
}

// _kern_dup
int
_kern_dup(int fd)
{
	// get the descriptor
	Descriptor *descriptor = get_descriptor(fd);
	if (!descriptor)
		return B_FILE_ERROR;

	// clone it
	Descriptor *clone;
	status_t error = descriptor->Dup(clone);
	if (error != B_OK)
		return error;

	return add_descriptor(clone);
}

// _kern_fsync
status_t
_kern_fsync(int fd)
{
	// get the descriptor
	FileDescriptor *descriptor
		= dynamic_cast<FileDescriptor*>(get_descriptor(fd));
	if (!descriptor)
		return B_FILE_ERROR;

	// sync
	if (fsync(descriptor->fd) < 0)
		return errno;

	return B_OK;
}

// _kern_read_stat
status_t
_kern_read_stat(int fd, const char *path, bool traverseLink,
	struct stat *st, size_t statSize)
{
	if (path) {
		// get a usable path
		string realPath;
		status_t error = get_path(fd, path, realPath);
		if (error != B_OK)
			return error;

		// stat
		int result;
		if (traverseLink)
			result = stat(realPath.c_str(), st);
		else
			result = lstat(realPath.c_str(), st);

		if (result < 0)
			return errno;
	} else {
		Descriptor *descriptor = get_descriptor(fd);
		if (!descriptor)
			return B_FILE_ERROR;

		return descriptor->GetStat(traverseLink, st);
	}

	return B_OK;
}

// _kern_write_stat
status_t
_kern_write_stat(int fd, const char *path, bool traverseLink,
	const struct stat *st, size_t statSize, int statMask)
{
	// get a usable path
	int realFD = -1;
	string realPath;
	status_t error;
	bool isSymlink = false;
	if (path) {
		error = get_path(fd, path, realPath);
		if (error != B_OK)
			return error;

		// stat it to see, if it is a symlink
		struct stat tmpStat;
		if (lstat(realPath.c_str(), &tmpStat) < 0)
			return errno;

		isSymlink = S_ISLNK(tmpStat.st_mode);

	} else {
		Descriptor *descriptor = get_descriptor(fd);
		if (!descriptor)
			return B_FILE_ERROR;

		if (FileDescriptor *fileFD
				= dynamic_cast<FileDescriptor*>(descriptor)) {
			realFD = fileFD->fd;

		} else if (dynamic_cast<DirectoryDescriptor*>(descriptor)) {
			error = get_path(fd, NULL, realPath);
			if (error != B_OK)
				return error;

		} else if (SymlinkDescriptor *linkFD
				= dynamic_cast<SymlinkDescriptor*>(descriptor)) {
			realPath = linkFD->path;
			isSymlink = true;

		} else
			return B_FILE_ERROR;
	}

	// We're screwed, if the node to manipulate is a symlink. All the
	// available functions traverse symlinks.
	if (isSymlink && !traverseLink)
		return B_ERROR;

	if (realFD >= 0) {
		if (statMask & B_STAT_MODE) {
			if (fchmod(realFD, st->st_mode) < 0)
				return errno;
		}

		if (statMask & B_STAT_UID) {
			if (fchown(realFD, st->st_uid, (gid_t)-1) < 0)
				return errno;
		}

		if (statMask & B_STAT_GID) {
			if (fchown(realFD, (uid_t)-1, st->st_gid) < 0)
				return errno;
		}

		if (statMask & B_STAT_SIZE) {
			if (ftruncate(realFD, st->st_size) < 0)
				return errno;
		}

		// The timestamps can only be set via utime(), but that requires a
		// path we don't have.
		if (statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME
				| B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME)) {
			return B_ERROR;
		}

		return 0;

	} else {
		if (statMask & B_STAT_MODE) {
			if (chmod(realPath.c_str(), st->st_mode) < 0)
				return errno;
		}

		if (statMask & B_STAT_UID) {
			if (chown(realPath.c_str(), st->st_uid, (gid_t)-1) < 0)
				return errno;
		}

		if (statMask & B_STAT_GID) {
			if (chown(realPath.c_str(), (uid_t)-1, st->st_gid) < 0)
				return errno;
		}

		if (statMask & B_STAT_SIZE) {
			if (truncate(realPath.c_str(), st->st_size) < 0)
				return errno;
		}

		if (statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME)) {
			// Grab the previous mod and access times so we only overwrite
			// the specified time and not both
			struct stat oldStat;
			if (~statMask & (B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME)) {
				if (stat(realPath.c_str(), &oldStat) < 0)
					return errno;
			}

			utimbuf buffer;
			buffer.actime = (statMask & B_STAT_ACCESS_TIME) ? st->st_atime : oldStat.st_atime;
			buffer.modtime = (statMask & B_STAT_MODIFICATION_TIME) ? st->st_mtime : oldStat.st_mtime;
			if (utime(realPath.c_str(), &buffer) < 0)
				return errno;
		}

		// not supported
		if (statMask & (B_STAT_CREATION_TIME | B_STAT_CHANGE_TIME))
			return B_ERROR;
	}

	return B_OK;
}


// #pragma mark -

// _kern_create_symlink
status_t
_kern_create_symlink(int fd, const char *path, const char *toPath, int mode)
{
	// Note: path must not be NULL, so this will always work.
	// get a usable path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	// symlink
	if (symlink(toPath, realPath.c_str()) < 0)
		return errno;

	return B_OK;
}

// _kern_read_link
status_t
_kern_read_link(int fd, const char *path, char *buffer, size_t *_bufferSize)
{
	// get the path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	ssize_t bytesRead = readlink(realPath.c_str(), buffer, *_bufferSize);
	if (bytesRead < 0)
		return errno;

	// On Haiku _kern_read_link will return the length of the link, *not*
	// the number of bytes written to the buffer parameter. To emulate that
	// here, we use readlink() to read the links contents, and then if it is
	// possible that the link contents didn't fit in buffer, we'll fall back
	// to lstat() to get the full length of the link.
	if (static_cast<size_t>(bytesRead) < *_bufferSize) {
		buffer[bytesRead] = '\0';
		*_bufferSize = bytesRead;
	} else {
		// The number of bytes copied by readlink() tells us that the entire
		// link might not have fit into buffer. Fall back to getting the full
		// length of the link using lstat.
		struct stat linkStat;
		if (lstat(realPath.c_str(), &linkStat) != 0)
			return errno;

		*_bufferSize = linkStat.st_size;
	}

	return B_OK;
}

// _kern_unlink
status_t
_kern_unlink(int fd, const char *path)
{
	// get a usable path
	string realPath;
	status_t error = get_path(fd, path, realPath);
	if (error != B_OK)
		return error;

	// unlink
	if (unlink(realPath.c_str()) < 0)
		return errno;

	return B_OK;
}

// _kern_rename
status_t
_kern_rename(int oldDir, const char *oldPath, int newDir, const char *newPath)
{
	// get usable paths
	string realOldPath;
	status_t error = get_path(oldDir, oldPath, realOldPath);
	if (error != B_OK)
		return error;

	string realNewPath;
	error = get_path(newDir, newPath, realNewPath);
	if (error != B_OK)
		return error;

	// rename
	if (rename(realOldPath.c_str(), realNewPath.c_str()) < 0)
		return errno;

	return B_OK;
}


// #pragma mark -

// _kern_lock_node
status_t
_kern_lock_node(int fd)
{
	return B_ERROR;
}

// _kern_unlock_node
status_t
_kern_unlock_node(int fd)
{
	return B_ERROR;
}


// #pragma mark -

// read_pos
ssize_t
read_pos(int fd, off_t pos, void *buffer, size_t bufferSize)
{
	// seek
	off_t result = lseek(fd, pos, SEEK_SET);
	if (result < 0)
		return errno;

	// read
	ssize_t bytesRead = haiku_host_platform_read(fd, buffer, bufferSize);
	if (bytesRead < 0) {
		errno = bytesRead;
		return -1;
	}

	return bytesRead;
}

// write_pos
ssize_t
write_pos(int fd, off_t pos, const void *buffer, size_t bufferSize)
{
	// If this is an attribute descriptor, let it do the job.
	AttributeDescriptor* descriptor
		= dynamic_cast<AttributeDescriptor*>(get_descriptor(fd));
	if (descriptor != NULL) {
		status_t error = descriptor->Write(pos, buffer, bufferSize);
		if (error != B_OK) {
			errno = error;
			return -1;
		}

		return bufferSize;
	}

	// seek
	off_t result = lseek(fd, pos, SEEK_SET);
	if (result < 0)
		return errno;

	// write
	ssize_t bytesWritten = haiku_host_platform_write(fd, buffer, bufferSize);
	if (bytesWritten < 0) {
		errno = bytesWritten;
		return -1;
	}

	return bytesWritten;
}

// readv_pos
ssize_t
readv_pos(int fd, off_t pos, const struct iovec *vec, size_t count)
{
	// seek
	off_t result = lseek(fd, pos, SEEK_SET);
	if (result < 0)
		return errno;

	// read
	ssize_t bytesRead = haiku_host_platform_readv(fd, vec, count);
	if (bytesRead < 0) {
		errno = bytesRead;
		return -1;
	}

	return bytesRead;
}

// writev_pos
ssize_t
writev_pos(int fd, off_t pos, const struct iovec *vec, size_t count)
{
	// seek
	off_t result = lseek(fd, pos, SEEK_SET);
	if (result < 0)
		return errno;

	// read
	ssize_t bytesWritten = haiku_host_platform_writev(fd, vec, count);
	if (bytesWritten < 0) {
		errno = bytesWritten;
		return -1;
	}

	return bytesWritten;
}


// #pragma mark -


int
_haiku_build_fchmod(int fd, mode_t mode)
{
	return _haiku_build_fchmodat(fd, NULL, mode, AT_SYMLINK_NOFOLLOW);
}


int
_haiku_build_fchmodat(int fd, const char* path, mode_t mode, int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return fchmodat(fd, path, mode, flag);

	struct stat st;
	st.st_mode = mode;

	RETURN_AND_SET_ERRNO(_kern_write_stat(fd, path,
		(flag & AT_SYMLINK_NOFOLLOW) == 0, &st, sizeof(st), B_STAT_MODE));
}


int
_haiku_build_fstat(int fd, struct stat* st)
{
	return _haiku_build_fstatat(fd, NULL, st, AT_SYMLINK_NOFOLLOW);
}


int
_haiku_build_fstatat(int fd, const char* path, struct stat* st, int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return fstatat(fd, path, st, flag);

	RETURN_AND_SET_ERRNO(_kern_read_stat(fd, path,
		(flag & AT_SYMLINK_NOFOLLOW) == 0, st, sizeof(*st)));
}


int
_haiku_build_mkdirat(int fd, const char* path, mode_t mode)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return mkdirat(fd, path, mode);

	RETURN_AND_SET_ERRNO(_kern_create_dir(fd, path, mode));
}


int
_haiku_build_mkfifoat(int fd, const char* path, mode_t mode)
{
	return mkfifoat(fd, path, mode);

	// TODO: Handle non-system FDs.
}


int
_haiku_build_utimensat(int fd, const char* path, const struct timespec times[2],
	int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return utimensat(fd, path, times, flag);

	struct stat stat;
	status_t status;
	uint32 mask = 0;

	// Init the stat time fields to the current time, if at least one time is
	// supposed to be set to it.
	if (times == NULL || times[0].tv_nsec == UTIME_NOW
		|| times[1].tv_nsec == UTIME_NOW) {
		timeval now;
		gettimeofday(&now, NULL);
		HAIKU_HOST_STAT_ATIM(stat).tv_sec
			= HAIKU_HOST_STAT_MTIM(stat).tv_sec = now.tv_sec;
		HAIKU_HOST_STAT_ATIM(stat).tv_nsec
			= HAIKU_HOST_STAT_MTIM(stat).tv_nsec = now.tv_usec * 1000;
	}

	if (times != NULL) {
		// access time
		if (times[0].tv_nsec != UTIME_OMIT) {
			mask |= B_STAT_ACCESS_TIME;

			if (times[0].tv_nsec != UTIME_NOW) {
				if (times[0].tv_nsec < 0 || times[0].tv_nsec > 999999999)
					RETURN_AND_SET_ERRNO(EINVAL);
			}

			HAIKU_HOST_STAT_ATIM(stat) = times[0];
		}

		// modified time
		if (times[1].tv_nsec != UTIME_OMIT) {
			mask |= B_STAT_MODIFICATION_TIME;

			if (times[1].tv_nsec != UTIME_NOW) {
				if (times[1].tv_nsec < 0 || times[1].tv_nsec > 999999999)
					RETURN_AND_SET_ERRNO(EINVAL);
			}

			HAIKU_HOST_STAT_MTIM(stat) = times[1];
		}
	} else
		mask |= B_STAT_ACCESS_TIME | B_STAT_MODIFICATION_TIME;

	// set the times -- as per spec we even need to do this, if both have
	// UTIME_OMIT set
	status = _kern_write_stat(fd, path, (flag & AT_SYMLINK_NOFOLLOW) == 0,
		&stat, sizeof(struct stat), mask);

	RETURN_AND_SET_ERRNO(status);
}


int
_haiku_build_futimens(int fd, const struct timespec times[2])
{
	return _haiku_build_utimensat(fd, NULL, times, AT_SYMLINK_NOFOLLOW);
}


int
_haiku_build_faccessat(int fd, const char* path, int accessMode, int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return faccessat(fd, path, accessMode, flag);

	// stat the file
	struct stat st;
	status_t error = _kern_read_stat(fd, path, false, &st, sizeof(st));
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	// get the current user
	uid_t uid = (flag & AT_EACCESS) != 0 ? geteuid() : getuid();

	int fileMode = 0;

	if (uid == 0) {
		// user is root
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		fileMode = R_OK | W_OK;
		if ((st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)
			fileMode |= X_OK;
	} else if (st.st_uid == uid) {
		// user is node owner
		if ((st.st_mode & S_IRUSR) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWUSR) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXUSR) != 0)
			fileMode |= X_OK;
	} else if (st.st_gid == ((flag & AT_EACCESS) != 0 ? getegid() : getgid())) {
		// user is in owning group
		if ((st.st_mode & S_IRGRP) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWGRP) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXGRP) != 0)
			fileMode |= X_OK;
	} else {
		// user is one of the others
		if ((st.st_mode & S_IROTH) != 0)
			fileMode |= R_OK;
		if ((st.st_mode & S_IWOTH) != 0)
			fileMode |= W_OK;
		if ((st.st_mode & S_IXOTH) != 0)
			fileMode |= X_OK;
	}

	if ((accessMode & ~fileMode) != 0)
		RETURN_AND_SET_ERRNO(EACCES);

	return 0;
}


int
_haiku_build_fchdir(int fd)
{
	if (is_unknown_or_system_descriptor(fd))
		return fchdir(fd);

	RETURN_AND_SET_ERRNO(B_FILE_ERROR);
}


int
_haiku_build_close(int fd)
{
	if (get_descriptor(fd) == NULL)
		return close(fd);

	RETURN_AND_SET_ERRNO(_kern_close(fd));
}


int
_haiku_build_dup(int fd)
{
	if (get_descriptor(fd) == NULL)
		return close(fd);

	RETURN_AND_SET_ERRNO(_kern_dup(fd));
}


int
_haiku_build_dup2(int fd1, int fd2)
{
	if (is_unknown_or_system_descriptor(fd1))
		return dup2(fd1, fd2);

	// TODO: Handle non-system FDs.
	RETURN_AND_SET_ERRNO(B_NOT_SUPPORTED);
}


int
_haiku_build_linkat(int toFD, const char* toPath, int pathFD, const char* path,
	int flag)
{
	return linkat(toFD, toPath, pathFD, path, flag);

	// TODO: Handle non-system FDs.
}


int
_haiku_build_unlinkat(int fd, const char* path, int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return unlinkat(fd, path, flag);

	RETURN_AND_SET_ERRNO(_kern_unlink(fd, path));
}


ssize_t
_haiku_build_readlinkat(int fd, const char* path, char* buffer,
	size_t bufferSize)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return readlinkat(fd, path, buffer, bufferSize);

	status_t error = _kern_read_link(fd, path, buffer, &bufferSize);
	if (error != B_OK)
		RETURN_AND_SET_ERRNO(error);

	return bufferSize;
}


int
_haiku_build_symlinkat(const char* toPath, int fd, const char* symlinkPath)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return symlinkat(toPath, fd, symlinkPath);

	RETURN_AND_SET_ERRNO(_kern_create_symlink(fd, symlinkPath, toPath,
		S_IRWXU | S_IRWXG | S_IRWXO));
}


int
_haiku_build_ftruncate(int fd, off_t newSize)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return ftruncate(fd, newSize);

	struct stat st;
	st.st_size = newSize;

	RETURN_AND_SET_ERRNO(_kern_write_stat(fd, NULL, false, &st, sizeof(st),
		B_STAT_SIZE));
}


int
_haiku_build_fchown(int fd, uid_t owner, gid_t group)
{
	return _haiku_build_fchownat(fd, NULL, owner, group, AT_SYMLINK_NOFOLLOW);
}


int
_haiku_build_fchownat(int fd, const char* path, uid_t owner, gid_t group,
	int flag)
{
	if (fd >= 0 && fd != AT_FDCWD && get_descriptor(fd) == NULL)
		return fchownat(fd, path, owner, group, flag);

	struct stat st;
	st.st_uid = owner;
	st.st_gid = group;

	RETURN_AND_SET_ERRNO(_kern_write_stat(fd, path,
		(flag & AT_SYMLINK_NOFOLLOW) == 0, &st, sizeof(st),
		B_STAT_UID | B_STAT_GID));
}


int
_haiku_build_mknodat(int fd, const char* name, mode_t mode, dev_t dev)
{
	return mknodat(fd, name, mode, dev);

	// TODO: Handle non-system FDs.
}


int
_haiku_build_creat(const char* path, mode_t mode)
{
	return _haiku_build_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}


int
_haiku_build_open(const char* path, int openMode, mode_t permissions)
{
	return _haiku_build_openat(AT_FDCWD, path, openMode, permissions);
}


int
_haiku_build_openat(int fd, const char* path, int openMode, mode_t permissions)
{
	// adapt the permissions as required by POSIX
	mode_t mask = umask(0);
	umask(mask);
	permissions &= ~mask;

	RETURN_AND_SET_ERRNO(_kern_open(fd, path, openMode, permissions));
}


int
_haiku_build_fcntl(int fd, int op, int argument)
{
	if (is_unknown_or_system_descriptor(fd))
		return fcntl(fd, op, argument);

	RETURN_AND_SET_ERRNO(B_NOT_SUPPORTED);
}


int
_haiku_build_renameat(int fromFD, const char* from, int toFD, const char* to)
{
	if ((fromFD >= 0 && fromFD != AT_FDCWD && get_descriptor(fromFD) == NULL)
		|| (toFD >= 0 && toFD != AT_FDCWD && get_descriptor(toFD) == NULL)) {
		return renameat(fromFD, from, toFD, to);
	}

	RETURN_AND_SET_ERRNO(_kern_rename(fromFD, from, toFD, to));
}
