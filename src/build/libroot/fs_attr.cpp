
#ifdef BUILDING_FS_SHELL
#	include "compat.h"
#	define B_OK			0
#	define B_BAD_VALUE	EINVAL
#	define B_FILE_ERROR	EBADF
#else
#	include <BeOSBuildCompatibility.h>
#	include <syscalls.h>
#endif

// Defined, if the host platform has extended attribute support.
// Unfortunately I don't seem to have extended attribute support under
// SuSE Linux 9.2 (kernel 2.6.8-...) with ReiserFS 3.6.
//#define HAS_EXTENDED_ATTRIBUTE_SUPPORT 1

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <string>

#include <fs_attr.h>

#include "fs_attr_impl.h"

#ifdef HAS_EXTENDED_ATTRIBUTE_SUPPORT
#include <sys/xattr.h>

static const char *kAttributeDirMarkAttributeName = "_has_haiku_attr_dir";
#endif

using namespace std;
using namespace BPrivate;

static const char *sAttributeDirBasePath = HAIKU_BUILD_ATTRIBUTES_DIR;

// init_attribute_dir_base_dir
static status_t
init_attribute_dir_base_dir()
{
	static bool initialized = false;
	static status_t initError;

	if (initialized)
		return initError;
	
	// stat the dir
	struct stat st;
	initError = B_OK;
	if (lstat(sAttributeDirBasePath, &st) == 0) {
		if (!S_ISDIR(st.st_mode)) {
			// the attribute dir base dir is no directory
			fprintf(stderr, "init_attribute_dir_base_dir(): The Attribute "
				"directory base directory exists, but is no directory!\n");
			initError = B_FILE_ERROR;
		}
	
	} else {
		// doesn't exist yet: create it
		if (mkdir(sAttributeDirBasePath, S_IRWXU | S_IRWXG | S_IRWXO) < 0)
			initError = errno;
	}

	initialized = true;
	return initError;
}

// escape_attr_name
static string
escape_attr_name(const char *name)
{
	string escapedName("_");
	while (*name != '\0') {
		// we replace '/' with "_s" and '_' with "__"
		if (*name == '/')
			escapedName += "_s";
		else if (*name == '_')
			escapedName += "__";
		else
			escapedName += *name;
	
		name++;
	}

	return escapedName;
}

// deescape_attr_name
static string
deescape_attr_name(const char *name)
{
	if (name[0] != '_') {
		debugger("deescape_attr_name(): name doesn't start with '_'!\n");
		return "___";
	}
	name++;
	
	string deescapedName;
	while (*name != '\0') {
		if (*name == '_') {
			name++;
			if (*name == 's') {
				deescapedName += '/';
			} else if (*name == '_') {
				deescapedName += '_';
			} else {
				debugger("deescape_attr_name(): name contains invalid escaped "
					"sequence!\n");
				name--;
			}
		} else
			deescapedName += *name;
		
		name++;
	}

	return deescapedName;	
}

// get_attribute_dir_path
static string
get_attribute_dir_path(NodeRef ref)
{
	string attrDirPath(sAttributeDirBasePath);
	char buffer[32];
	sprintf(buffer, "/%lld", (int64)ref.node);
	attrDirPath += buffer;
	return attrDirPath;
}

// has_attribute_dir_mark
static bool
has_attribute_dir_mark(const char *path, int fd)
{
	#ifdef HAS_EXTENDED_ATTRIBUTE_SUPPORT

		uint8 value;
		if (path) {
			return (lgetxattr(path, kAttributeDirMarkAttributeName, &value, 1)
				> 0);
		} else {
			return (fgetxattr(fd, kAttributeDirMarkAttributeName, &value, 1)
				> 0);
		}
	
	#else	// !HAS_EXTENDED_ATTRIBUTE_SUPPORT

		// No extended attribute support. We can't mark the file and thus
		// have to handle it as marked.
		return true;

	#endif
}

// set_attribute_dir_mark
static status_t
set_attribute_dir_mark(const char *path, int fd)
{
	#ifdef HAS_EXTENDED_ATTRIBUTE_SUPPORT

		uint8 value = 1;
		if (path) {
			if (lsetxattr(path, kAttributeDirMarkAttributeName, &value, 1, 0)
				< 0) {
				fprintf(stderr, "set_attribute_dir_mark(): lsetxattr() "
					"failed on \"%s\"\n", path);
				return errno;
			}
		} else {
			if (fsetxattr(fd, kAttributeDirMarkAttributeName, &value, 1, 0)
				< 0) {
				fprintf(stderr, "set_attribute_dir_mark(): fsetxattr() "
					"failed on FD \"%d\"\n", fd);
				return errno;
			}
		}
	
	#endif

	return B_OK;
}

// empty_attribute_dir
static status_t
empty_attribute_dir(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir)
		return errno;

	while (dirent *entry = readdir(dir)) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		string entryPath(path);
		entryPath += '/';
		entryPath += entry->d_name;

		// We assume the attribute dir contains files only (we only create
		// files) and thus use unlink().
		if (unlink(entryPath.c_str()) < 0) {
			status_t error = errno;
			closedir(dir);
			return error;
		}
	}

	closedir(dir);
	return B_OK;
}

// ensure_attribute_dir_exists
static status_t
ensure_attribute_dir_exists(NodeRef ref, const char *path, int fd)
{
	// init the base directory here
	status_t error = init_attribute_dir_base_dir();
	if (error != B_OK)
		return error;

	// stat the dir
	string attrDirPath(get_attribute_dir_path(ref));
	struct stat st;
	if (lstat(attrDirPath.c_str(), &st) == 0) {
		if (!S_ISDIR(st.st_mode)) {
			// the attribute dir is no directory
			fprintf(stderr, "ensure_attribute_dir_exists(): Attribute "
				"directory for node %lld exists, but is no directory!\n",
				ref.node);
			return B_FILE_ERROR;
		}

		// already exists: Check whether the file is marked. If not, this
		// is a stale attribute directory from a deleted node that had the
		// same node ID as this one.
		if (has_attribute_dir_mark(path, fd))
			return B_OK;

		// empty the attribute dir
		error = empty_attribute_dir(attrDirPath.c_str());
		if (error != B_OK) {
			fprintf(stderr, "ensure_attribute_dir_exists(): Attribute "
				"directory for node %lld exists, the node has no mark, and "
				"emptying the attribute directory failed\n",
				ref.node);
			return error;
		}

		// mark the file
		return set_attribute_dir_mark(path, fd);
	}

	// doesn't exist yet: create it
	if (mkdir(attrDirPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) < 0)
		return errno;

	// mark the file
	return set_attribute_dir_mark(path, fd);
}

// open_attr_dir
DIR *
BPrivate::open_attr_dir(NodeRef ref, const char *path, int fd)
{
	// make sure the directory exists
	status_t error = ensure_attribute_dir_exists(ref, path, fd);
	if (error != B_OK) {
		errno = error;
		return NULL;
	}

	// open it
	string dirPath(get_attribute_dir_path(ref));
	return opendir(dirPath.c_str());
}

// get_attribute_path
status_t
BPrivate::get_attribute_path(NodeRef ref, const char *path, int fd,
	const char *attribute, string &attrPath, string &typePath)
{
	if (!attribute || strlen(attribute) == 0)
		return B_BAD_VALUE;

	// make sure the attribute dir for the node exits
	status_t error = ensure_attribute_dir_exists(ref, path, fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}
	
	// construct the attribute path
	attrPath = get_attribute_dir_path(ref) + '/';
	string attrName(escape_attr_name(attribute));
	typePath = attrPath + "t" + attrName;
	attrPath += attrName;

	return B_OK;
}

// get_attribute_path
static status_t
get_attribute_path(int fd, const char *attribute, string &attrPath,
	string &typePath)
{
	// stat the file to get a NodeRef
	struct stat st;
	if (fstat(fd, &st) < 0)
		return errno;
	NodeRef ref(st);

	return get_attribute_path(ref, NULL, fd, attribute, attrPath, typePath);
}

// fs_open_attr_dir
DIR *
fs_open_attr_dir(const char *path)
{
	struct stat st;
	if (lstat(path, &st))
		return NULL;

	return open_attr_dir(NodeRef(st), path, -1);
}

// fs_fopen_attr_dir
DIR *
fs_fopen_attr_dir(int fd)
{
	struct stat st;

#ifdef BUILDING_FS_SHELL

	if (fstat(fd, &st) < 0)
		return NULL;

	return open_attr_dir(NodeRef(st), NULL, fd);

#else

	status_t error = _kern_read_stat(fd, NULL, false, &st,
		sizeof(struct stat));
	if (error != B_OK) {
		errno = error;
		return NULL;
	}

	// Try to get a path. If we can't get a path, this is must be a "real"
	// (i.e. system) file descriptor, which is just as well.
	string path;
	bool pathValid = (get_path(fd, NULL, path) == B_OK);
	
	// get the attribute path
	return open_attr_dir(NodeRef(st), (pathValid ? path.c_str() : NULL),
		(pathValid ? -1 : fd));

#endif
}

// fs_close_attr_dir
int
fs_close_attr_dir(DIR *dir)
{
	return closedir(dir);
}

// fs_read_attr_dir
struct dirent *
fs_read_attr_dir(DIR *dir)
{
	struct dirent *entry = NULL;
	while (true) {
		// read the next entry
		entry = readdir(dir);
		if (!entry)
			return NULL;
	
		// ignore administrative entries; the 
		if (entry->d_name[0] == '_') {
			string attrName = deescape_attr_name(entry->d_name);
			strcpy(entry->d_name, attrName.c_str());
			return entry;
		}
	}
}

// fs_rewind_attr_dir
void
fs_rewind_attr_dir(DIR *dir)
{
	rewinddir(dir);
}

// fs_open_attr
int
fs_open_attr(int fd, const char *attribute, uint32 type, int openMode)
{
	if (!attribute) {
		errno = B_BAD_VALUE;
		return -1;
	}
	
	// get the attribute path
	string attrPath;
	string typePath;
	status_t error = get_attribute_path(fd, attribute, attrPath, typePath);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// check, if the attribute already exists
	struct stat st;
	bool exists = (lstat(attrPath.c_str(), &st) == 0);

	// open the attribute
	int attrFD = open(attrPath.c_str(), openMode, S_IRWXU | S_IRWXG | S_IRWXO);
	if (attrFD < 0)
		return -1;

	// set the type, if the attribute didn't exist yet
	if (!exists) {
		// create a file prefixed "t"
		int typeFD = creat(typePath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
		if (typeFD >= 0) {
			// write the type into the file
			if (write(typeFD, &type, sizeof(type)) < 0)
				error = errno;
			
			close(typeFD);

		} else
			error = errno;

		// remove type and attribute file, if something went wrong
		if (error != B_OK) {
			if (typeFD > 0) {
				unlink(typePath.c_str());
			}
		
			close(attrFD);
			unlink(attrPath.c_str());
		
			errno = error;
			return -1;
		}
	}

	return attrFD;
}

// fs_close_attr
int
fs_close_attr(int fd)
{
	return close(fd);
}

// fs_read_attr
ssize_t
fs_read_attr(int fd, const char *attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	// open the attribute
	int attrFD = fs_open_attr(fd, attribute, type, O_RDONLY);
	if (attrFD < 0)
		return attrFD;

	// read
	ssize_t bytesRead = read_pos(attrFD, pos, buffer, readBytes);
	status_t error = errno;

	// close the attribute
	fs_close_attr(attrFD);

	if (bytesRead < 0) {
		errno = error;
		return -1;
	}
	
	return bytesRead;
}

// fs_write_attr
ssize_t
fs_write_attr(int fd, const char *attribute, uint32 type, off_t pos,
	const void *buffer, size_t readBytes)
{
	// open the attribute
	int attrFD = fs_open_attr(fd, attribute, type,
		O_WRONLY | O_CREAT | O_TRUNC);
	if (attrFD < 0)
		return attrFD;

	// read
	ssize_t bytesWritten = write_pos(attrFD, pos, buffer, readBytes);
	status_t error = errno;

	// close the attribute
	fs_close_attr(attrFD);

	if (bytesWritten < 0) {
		errno = error;
		return -1;
	}
	
	return bytesWritten;
}

// fs_remove_attr
int
fs_remove_attr(int fd, const char *attribute)
{
	if (!attribute) {
		errno = B_BAD_VALUE;
		return -1;
	}
	
	// get the attribute path
	string attrPath;
	string typePath;
	status_t error = get_attribute_path(fd, attribute, attrPath, typePath);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// remove the attribute
	if (unlink(attrPath.c_str()) < 0)
		return -1;
	
	unlink(typePath.c_str());
	
	return B_OK;
}

// fs_stat_attr
int
fs_stat_attr(int fd, const char *attribute, struct attr_info *attrInfo)
{
	if (!attribute || !attrInfo) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// get the attribute path
	string attrPath;
	string typePath;
	status_t error = get_attribute_path(fd, attribute, attrPath, typePath);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// stat the attribute file to get the size of the attribute
	struct stat st;
	if (lstat(attrPath.c_str(), &st) < 0)
		return -1;
		
	attrInfo->size = st.st_size;

	// now open the attribute type file and read the attribute's type
	int typeFD = open(typePath.c_str(), O_RDONLY);
	if (typeFD < 0)
		return -1;

	ssize_t bytesRead = read(typeFD, &attrInfo->type, sizeof(attrInfo->type));
	if (bytesRead < 0)
		error = errno;
	else if (bytesRead < (ssize_t)sizeof(attrInfo->type))
		error = B_FILE_ERROR;

	close(typeFD);

	// fail on error
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	return 0;					
}

