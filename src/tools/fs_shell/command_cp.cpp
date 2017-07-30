/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "command_cp.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <EntryFilter.h>
#include <fs_attr.h>
#include <StorageDefs.h>

#include "fssh_dirent.h"
#include "fssh_errno.h"
#include "fssh_errors.h"
#include "fssh_fcntl.h"
#include "fssh_fs_attr.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "fssh_unistd.h"
#include "path_util.h"
#include "stat_util.h"
#include "syscalls.h"


using BPrivate::EntryFilter;


namespace FSShell {


static void *sCopyBuffer = NULL;
static const int sCopyBufferSize = 64 * 1024;	// 64 KB

struct Options {
	Options()
		: entryFilter(),
		  attributesOnly(false),
		  ignoreAttributes(false),
		  dereference(true),
		  alwaysDereference(false),
		  force(false),
		  recursive(false)
	{
	}

	EntryFilter	entryFilter;
	bool		attributesOnly;
	bool		ignoreAttributes;
	bool		dereference;
	bool		alwaysDereference;
	bool		force;
	bool		recursive;
};

class Directory;
class File;
class SymLink;

// Node
class Node {
public:
	Node() {}
	virtual ~Node() {}

	const struct fssh_stat &Stat() const	{ return fStat; }
	bool IsFile() const				{ return FSSH_S_ISREG(fStat.fssh_st_mode); }
	bool IsDirectory() const		{ return FSSH_S_ISDIR(fStat.fssh_st_mode); }
	bool IsSymLink() const			{ return FSSH_S_ISLNK(fStat.fssh_st_mode); }

	virtual File *ToFile()				{ return NULL; }
	virtual Directory *ToDirectory()	{ return NULL; }
	virtual SymLink *ToSymLink()		{ return NULL; }

	virtual	fssh_ssize_t GetNextAttr(char *name, int size) = 0;
	virtual fssh_status_t GetAttrInfo(const char *name,
		fssh_attr_info &info) = 0;
	virtual fssh_ssize_t ReadAttr(const char *name, uint32_t type,
		fssh_off_t pos, void *buffer, int size) = 0;
	virtual fssh_ssize_t WriteAttr(const char *name, uint32_t type,
		fssh_off_t pos, const void *buffer, int size) = 0;
	virtual fssh_status_t RemoveAttr(const char *name) = 0;

protected:
	struct fssh_stat	fStat;	// To be initialized by implementing classes.
};

// Directory
class Directory : public virtual Node {
public:
	virtual Directory *ToDirectory()	{ return this; }

	virtual	fssh_ssize_t GetNextEntry(struct fssh_dirent *entry, int size) = 0;
};

// File
class File : public virtual Node {
public:
	virtual File *ToFile()				{ return this; }

	virtual fssh_ssize_t Read(void *buffer, int size) = 0;
	virtual fssh_ssize_t Write(const void *buffer, int size) = 0;
};

// SymLink
class SymLink : public virtual Node {
public:
	virtual SymLink *ToSymLink()		{ return this; }

	virtual fssh_ssize_t ReadLink(char *buffer, int bufferSize) = 0;
};

// FSDomain
class FSDomain {
public:
	virtual ~FSDomain()	{}

	virtual fssh_status_t Open(const char *path, int openMode, Node *&node) = 0;

	virtual fssh_status_t CreateFile(const char *path,
		const struct fssh_stat &st, File *&file) = 0;
	virtual fssh_status_t CreateDirectory(const char *path,
		const struct fssh_stat &st, Directory *&dir) = 0;
	virtual fssh_status_t CreateSymLink(const char *path, const char *linkTo,
		const struct fssh_stat &st, SymLink *&link) = 0;

	virtual fssh_status_t Unlink(const char *path) = 0;
};


// #pragma mark -

// HostNode
class HostNode : public virtual Node {
public:
	HostNode()
		: Node(),
		  fFD(-1),
		  fAttrDir(NULL)
	{
	}

	virtual ~HostNode()
	{
		if (fFD >= 0)
			fssh_close(fFD);
		if (fAttrDir)
			fs_close_attr_dir(fAttrDir);
	}

	virtual fssh_status_t Init(const char *path, int fd,
		const struct fssh_stat &st)
	{
		fFD = fd;
		fStat = st;

		// open the attribute directory
		fAttrDir = fs_fopen_attr_dir(fd);
		if (!fAttrDir)
			return fssh_get_errno();

		return FSSH_B_OK;
	}

	virtual	fssh_ssize_t GetNextAttr(char *name, int size)
	{
		if (!fAttrDir)
			return 0;

		fssh_set_errno(FSSH_B_OK);
		struct dirent *entry = fs_read_attr_dir(fAttrDir);
		if (!entry)
			return fssh_get_errno();

		int len = strlen(entry->d_name);
		if (len >= size)
			return FSSH_B_NAME_TOO_LONG;

		strcpy(name, entry->d_name);
		return 1;
	}

	virtual fssh_status_t GetAttrInfo(const char *name, fssh_attr_info &info)
	{
		attr_info hostInfo;
		if (fs_stat_attr(fFD, name, &hostInfo) < 0)
			return fssh_get_errno();

		info.type = hostInfo.type;
		info.size = hostInfo.size;
		return FSSH_B_OK;
	}

	virtual fssh_ssize_t ReadAttr(const char *name, uint32_t type,
		fssh_off_t pos, void *buffer, int size)
	{
		fssh_ssize_t bytesRead = fs_read_attr(fFD, name, type, pos, buffer,
			size);
		return (bytesRead >= 0 ? bytesRead : fssh_get_errno());
	}

	virtual fssh_ssize_t WriteAttr(const char *name, uint32_t type,
		fssh_off_t pos, const void *buffer, int size)
	{
		fssh_ssize_t bytesWritten = fs_write_attr(fFD, name, type, pos, buffer,
			size);
		return (bytesWritten >= 0 ? bytesWritten : fssh_get_errno());
	}

	virtual fssh_status_t RemoveAttr(const char *name)
	{
		return (fs_remove_attr(fFD, name) == 0 ? 0 : fssh_get_errno());
	}

protected:
	int				fFD;
	DIR				*fAttrDir;
};

// HostDirectory
class HostDirectory : public Directory, public HostNode {
public:
	HostDirectory()
		: Directory(),
		  HostNode(),
		  fDir(NULL)
	{
	}

	virtual ~HostDirectory()
	{
		if (fDir)
			closedir(fDir);
	}

	virtual fssh_status_t Init(const char *path, int fd,
		const struct fssh_stat &st)
	{
		fssh_status_t error = HostNode::Init(path, fd, st);
		if (error != FSSH_B_OK)
			return error;

		fDir = opendir(path);
		if (!fDir)
			return fssh_get_errno();

		return FSSH_B_OK;
	}

	virtual	fssh_ssize_t GetNextEntry(struct fssh_dirent *entry, int size)
	{
		fssh_set_errno(FSSH_B_OK);
		struct dirent *hostEntry = readdir(fDir);
		if (!hostEntry)
			return fssh_get_errno();

		int nameLen = strlen(hostEntry->d_name);
		int recLen = entry->d_name + nameLen + 1 - (char*)entry;
		if (recLen > size)
			return FSSH_B_NAME_TOO_LONG;

		#if (defined(__BEOS__) || defined(__HAIKU__))
			entry->d_dev = hostEntry->d_dev;
		#endif
		entry->d_ino = hostEntry->d_ino;
		strcpy(entry->d_name, hostEntry->d_name);
		entry->d_reclen = recLen;

		return 1;
	}

private:
	DIR	*fDir;
};

// HostFile
class HostFile : public File, public HostNode {
public:
	HostFile()
		: File(),
		  HostNode()
	{
	}

	virtual ~HostFile()
	{
	}

	virtual fssh_ssize_t Read(void *buffer, int size)
	{
		fssh_ssize_t bytesRead = read(fFD, buffer, size);
		return (bytesRead >= 0 ? bytesRead : fssh_get_errno());
	}

	virtual fssh_ssize_t Write(const void *buffer, int size)
	{
		fssh_ssize_t bytesWritten = write(fFD, buffer, size);
		return (bytesWritten >= 0 ? bytesWritten : fssh_get_errno());
	}
};

// HostSymLink
class HostSymLink : public SymLink, public HostNode {
public:
	HostSymLink()
		: SymLink(),
		  HostNode(),
		  fPath(NULL)
	{
	}

	virtual ~HostSymLink()
	{
		if (fPath)
			free(fPath);
	}

	virtual fssh_status_t Init(const char *path, int fd,
		const struct fssh_stat &st)
	{
		fssh_status_t error = HostNode::Init(path, fd, st);
		if (error != FSSH_B_OK)
			return error;

		fPath = strdup(path);
		if (!fPath)
			return FSSH_B_NO_MEMORY;

		return FSSH_B_OK;
	}

	virtual fssh_ssize_t ReadLink(char *buffer, int bufferSize)
	{
		fssh_ssize_t bytesRead = readlink(fPath, buffer, bufferSize);
		return (bytesRead >= 0 ? bytesRead : fssh_get_errno());
	}

private:
	char	*fPath;
};

// HostFSDomain
class HostFSDomain : public FSDomain {
public:
	HostFSDomain() {}
	virtual ~HostFSDomain() {}

	virtual fssh_status_t Open(const char *path, int openMode, Node *&_node)
	{
		// open the node
		int fd = fssh_open(path, openMode);
		if (fd < 0)
			return fssh_get_errno();

		// stat the node
		struct fssh_stat st;
		if (fssh_fstat(fd, &st) < 0) {
			fssh_close(fd);
			return fssh_get_errno();
		}

		// check the node type and create the node
		HostNode *node = NULL;
		switch (st.fssh_st_mode & FSSH_S_IFMT) {
			case FSSH_S_IFLNK:
				node = new HostSymLink;
				break;
			case FSSH_S_IFREG:
				node = new HostFile;
				break;
			case FSSH_S_IFDIR:
				node = new HostDirectory;
				break;
			default:
				fssh_close(fd);
				return FSSH_EINVAL;
		}

		// init the node
		fssh_status_t error = node->Init(path, fd, st);
			// the node receives ownership of the FD
		if (error != FSSH_B_OK) {
			delete node;
			return error;
		}

		_node = node;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateFile(const char *path,
		const struct fssh_stat &st, File *&_file)
	{
		// create the file
		int fd = fssh_creat(path, st.fssh_st_mode & FSSH_S_IUMSK);
		if (fd < 0)
			return fssh_get_errno();

		// apply the other stat fields
		fssh_status_t error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			fssh_close(fd);
			return error;
		}

		// create the object
		HostFile *file = new HostFile;
		error = file->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete file;
			return error;
		}

		_file = file;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateDirectory(const char *path,
		const struct fssh_stat &st, Directory *&_dir)
	{
		// create the dir
		if (fssh_mkdir(path, st.fssh_st_mode & FSSH_S_IUMSK) < 0)
			return fssh_get_errno();

		// open the dir node
		int fd = fssh_open(path, FSSH_O_RDONLY | FSSH_O_NOTRAVERSE);
		if (fd < 0)
			return fssh_get_errno();

		// apply the other stat fields
		fssh_status_t error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			fssh_close(fd);
			return error;
		}

		// create the object
		HostDirectory *dir = new HostDirectory;
		error = dir->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete dir;
			return error;
		}

		_dir = dir;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateSymLink(const char *path, const char *linkTo,
		const struct fssh_stat &st, SymLink *&_link)
	{
		// create the link
		if (symlink(linkTo, path) < 0)
			return fssh_get_errno();

		// open the symlink node
		int fd = fssh_open(path, FSSH_O_RDONLY | FSSH_O_NOTRAVERSE);
		if (fd < 0)
			return fssh_get_errno();

		// apply the other stat fields
		fssh_status_t error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			fssh_close(fd);
			return error;
		}

		// create the object
		HostSymLink *link = new HostSymLink;
		error = link->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete link;
			return error;
		}

		_link = link;
		return FSSH_B_OK;
	}


	virtual fssh_status_t Unlink(const char *path)
	{
		if (fssh_unlink(path) < 0)
			return fssh_get_errno();
		return FSSH_B_OK;
	}

private:
	fssh_status_t _ApplyStat(int fd, const struct fssh_stat &st)
	{
		// TODO: Set times...
		return FSSH_B_OK;
	}
};


// #pragma mark -

// GuestNode
class GuestNode : public virtual Node {
public:
	GuestNode()
		: Node(),
		  fFD(-1),
		  fAttrDir(-1)
	{
	}

	virtual ~GuestNode()
	{
		if (fFD >= 0)
			_kern_close(fFD);
		if (fAttrDir)
			_kern_close(fAttrDir);
	}

	virtual fssh_status_t Init(const char *path, int fd,
		const struct fssh_stat &st)
	{
		fFD = fd;
		fStat = st;

		// open the attribute directory
		fAttrDir = _kern_open_attr_dir(fd, NULL);
		if (fAttrDir < 0) {
			// TODO: check if the file system supports attributes, and fail
		}

		return FSSH_B_OK;
	}

	virtual	fssh_ssize_t GetNextAttr(char *name, int size)
	{
		if (fAttrDir < 0)
			return 0;

		char buffer[sizeof(fssh_dirent) + B_ATTR_NAME_LENGTH];
		struct fssh_dirent *entry = (fssh_dirent *)buffer;
		int numRead = _kern_read_dir(fAttrDir, entry, sizeof(buffer), 1);
		if (numRead < 0)
			return numRead;
		if (numRead == 0)
			return 0;

		int len = strlen(entry->d_name);
		if (len >= size)
			return FSSH_B_NAME_TOO_LONG;

		strcpy(name, entry->d_name);
		return 1;
	}

	virtual fssh_status_t GetAttrInfo(const char *name, fssh_attr_info &info)
	{
		// open attr
		int attrFD = _kern_open_attr(fFD, name, FSSH_O_RDONLY);
		if (attrFD < 0)
			return attrFD;

		// stat attr
		struct fssh_stat st;
		fssh_status_t error = _kern_read_stat(attrFD, NULL, false, &st,
			sizeof(st));

		// close attr
		_kern_close(attrFD);

		if (error != FSSH_B_OK)
			return error;

		// convert stat to attr info
		info.type = st.fssh_st_type;
		info.size = st.fssh_st_size;

		return FSSH_B_OK;
	}

	virtual fssh_ssize_t ReadAttr(const char *name, uint32_t type,
		fssh_off_t pos, void *buffer, int size)
	{
		// open attr
		int attrFD = _kern_open_attr(fFD, name, FSSH_O_RDONLY);
		if (attrFD < 0)
			return attrFD;

		// stat attr
		fssh_ssize_t bytesRead = _kern_read(attrFD, pos, buffer, size);

		// close attr
		_kern_close(attrFD);

		return bytesRead;
	}

	virtual fssh_ssize_t WriteAttr(const char *name, uint32_t type,
		fssh_off_t pos, const void *buffer, int size)
	{
		// open attr
		int attrFD = _kern_create_attr(fFD, name, type, FSSH_O_WRONLY);
		if (attrFD < 0)
			return attrFD;

		// stat attr
		fssh_ssize_t bytesWritten = _kern_write(attrFD, pos, buffer, size);

		// close attr
		_kern_close(attrFD);

		return bytesWritten;
	}

	virtual fssh_status_t RemoveAttr(const char *name)
	{
		return _kern_remove_attr(fFD, name);
	}

protected:
	int				fFD;
	int				fAttrDir;
};

// GuestDirectory
class GuestDirectory : public Directory, public GuestNode {
public:
	GuestDirectory()
		: Directory(),
		  GuestNode(),
		  fDir(-1)
	{
	}

	virtual ~GuestDirectory()
	{
		if (fDir)
			_kern_close(fDir);
	}

	virtual fssh_status_t Init(const char *path, int fd,
		const struct fssh_stat &st)
	{
		fssh_status_t error = GuestNode::Init(path, fd, st);
		if (error != FSSH_B_OK)
			return error;

		fDir = _kern_open_dir(fd, NULL);
		if (fDir < 0)
			return fDir;

		return FSSH_B_OK;
	}

	virtual	fssh_ssize_t GetNextEntry(struct fssh_dirent *entry, int size)
	{
		return _kern_read_dir(fDir, entry, size, 1);
	}

private:
	int	fDir;
};

// GuestFile
class GuestFile : public File, public GuestNode {
public:
	GuestFile()
		: File(),
		  GuestNode()
	{
	}

	virtual ~GuestFile()
	{
	}

	virtual fssh_ssize_t Read(void *buffer, int size)
	{
		return _kern_read(fFD, -1, buffer, size);
	}

	virtual fssh_ssize_t Write(const void *buffer, int size)
	{
		return _kern_write(fFD, -1, buffer, size);
	}
};

// GuestSymLink
class GuestSymLink : public SymLink, public GuestNode {
public:
	GuestSymLink()
		: SymLink(),
		  GuestNode()
	{
	}

	virtual ~GuestSymLink()
	{
	}

	virtual fssh_ssize_t ReadLink(char *buffer, int _bufferSize)
	{
		fssh_size_t bufferSize = _bufferSize;
		fssh_status_t error = _kern_read_link(fFD, NULL, buffer, &bufferSize);
		return (error == FSSH_B_OK ? bufferSize : error);
	}
};

// GuestFSDomain
class GuestFSDomain : public FSDomain {
public:
	GuestFSDomain() {}
	virtual ~GuestFSDomain() {}

	virtual fssh_status_t Open(const char *path, int openMode, Node *&_node)
	{
		// open the node
		int fd = _kern_open(-1, path, openMode, 0);
		if (fd < 0)
			return fd;

		// stat the node
		struct fssh_stat st;
		fssh_status_t error = _kern_read_stat(fd, NULL, false, &st, sizeof(st));
		if (error < 0) {
			_kern_close(fd);
			return error;
		}

		// check the node type and create the node
		GuestNode *node = NULL;
		switch (st.fssh_st_mode & FSSH_S_IFMT) {
			case FSSH_S_IFLNK:
				node = new GuestSymLink;
				break;
			case FSSH_S_IFREG:
				node = new GuestFile;
				break;
			case FSSH_S_IFDIR:
				node = new GuestDirectory;
				break;
			default:
				_kern_close(fd);
				return FSSH_EINVAL;
		}

		// init the node
		error = node->Init(path, fd, st);
			// the node receives ownership of the FD
		if (error != FSSH_B_OK) {
			delete node;
			return error;
		}

		_node = node;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateFile(const char *path,
		const struct fssh_stat &st, File *&_file)
	{
		// create the file
		int fd = _kern_open(-1, path, FSSH_O_RDWR | FSSH_O_EXCL | FSSH_O_CREAT,
			st.fssh_st_mode & FSSH_S_IUMSK);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		fssh_status_t error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			_kern_close(fd);
			return error;
		}

		// create the object
		GuestFile *file = new GuestFile;
		error = file->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete file;
			return error;
		}

		_file = file;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateDirectory(const char *path,
		const struct fssh_stat &st, Directory *&_dir)
	{
		// create the dir
		fssh_status_t error = _kern_create_dir(-1, path,
			st.fssh_st_mode & FSSH_S_IUMSK);
		if (error < 0)
			return error;

		// open the dir node
		int fd = _kern_open(-1, path, FSSH_O_RDONLY | FSSH_O_NOTRAVERSE, 0);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			_kern_close(fd);
			return error;
		}

		// create the object
		GuestDirectory *dir = new GuestDirectory;
		error = dir->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete dir;
			return error;
		}

		_dir = dir;
		return FSSH_B_OK;
	}

	virtual fssh_status_t CreateSymLink(const char *path, const char *linkTo,
		const struct fssh_stat &st, SymLink *&_link)
	{
		// create the link
		fssh_status_t error = _kern_create_symlink(-1, path, linkTo,
			st.fssh_st_mode & FSSH_S_IUMSK);
		if (error < 0)
			return error;

		// open the symlink node
		int fd = _kern_open(-1, path, FSSH_O_RDONLY | FSSH_O_NOTRAVERSE, 0);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		error = _ApplyStat(fd, st);
		if (error != FSSH_B_OK) {
			_kern_close(fd);
			return error;
		}

		// create the object
		GuestSymLink *link = new GuestSymLink;
		error = link->Init(path, fd, st);
		if (error != FSSH_B_OK) {
			delete link;
			return error;
		}

		_link = link;
		return FSSH_B_OK;
	}

	virtual fssh_status_t Unlink(const char *path)
	{
		return _kern_unlink(-1, path);
	}

private:
	fssh_status_t _ApplyStat(int fd, const struct fssh_stat &st)
	{
		// TODO: Set times...
		return FSSH_B_OK;
	}
};


// #pragma mark -

static fssh_status_t copy_entry(FSDomain *sourceDomain, const char *source,
	FSDomain *targetDomain, const char *target, const Options &options,
	bool dereference);

static FSDomain *
get_file_domain(const char *target, const char *&fsTarget)
{
	if (target[0] == ':') {
		fsTarget = target + 1;
		return new HostFSDomain;
	} else {
		fsTarget = target;
		return new GuestFSDomain;
	}
}

typedef ObjectDeleter<Node> NodeDeleter;
typedef ObjectDeleter<FSDomain> DomainDeleter;
typedef MemoryDeleter PathDeleter;


static fssh_status_t
copy_file_contents(const char *source, File *sourceFile, const char *target,
	File *targetFile)
{
	fssh_off_t chunkSize = (sourceFile->Stat().fssh_st_size / 20) / sCopyBufferSize * sCopyBufferSize;
	if (chunkSize == 0)
		chunkSize = 1;

	bool progress = sourceFile->Stat().fssh_st_size > 1024 * 1024;
	if (progress) {
		printf("%s ", strrchr(target, '/') ? strrchr(target, '/') + 1 : target);
		fflush(stdout);
	}

	fssh_off_t total = 0;
	fssh_ssize_t bytesRead;
	while ((bytesRead = sourceFile->Read(sCopyBuffer, sCopyBufferSize)) > 0) {
		fssh_ssize_t bytesWritten = targetFile->Write(sCopyBuffer, bytesRead);
		if (progress && (total % chunkSize) == 0) {
			putchar('.');
			fflush(stdout);
		}
		if (bytesWritten < 0) {
			fprintf(stderr, "Error while writing to file `%s': %s\n",
				target, fssh_strerror(bytesWritten));
			return bytesWritten;
		}
		if (bytesWritten != bytesRead) {
			fprintf(stderr, "Could not write all data to file \"%s\".\n",
				target);
			return FSSH_B_IO_ERROR;
		}
		total += bytesWritten;
	}

	if (bytesRead < 0) {
		fprintf(stderr, "Error while reading from file `%s': %s\n",
			source, fssh_strerror(bytesRead));
		return bytesRead;
	}

	if (progress)
		putchar('\n');

	return FSSH_B_OK;
}


static fssh_status_t
copy_dir_contents(FSDomain *sourceDomain, const char *source,
	Directory *sourceDir, FSDomain *targetDomain, const char *target,
	const Options &options)
{
	char buffer[sizeof(fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
	struct fssh_dirent *entry =  (struct fssh_dirent *)buffer;
	fssh_ssize_t numRead;
	while ((numRead = sourceDir->GetNextEntry(entry, sizeof(buffer))) > 0) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// compose a new source path name
		char *sourceEntry = make_path(source, entry->d_name);
		if (!sourceEntry) {
			fprintf(stderr, "Error: Failed to allocate source path!\n");
			return FSSH_ENOMEM;
		}
		PathDeleter sourceDeleter(sourceEntry);

		// compose a new target path name
		char *targetEntry = make_path(target, entry->d_name);
		if (!targetEntry) {
			fprintf(stderr, "Error: Failed to allocate target path!\n");
			return FSSH_ENOMEM;
		}
		PathDeleter targetDeleter(targetEntry);

		fssh_status_t error = copy_entry(sourceDomain, sourceEntry,
			targetDomain, targetEntry, options, options.alwaysDereference);
		if (error != FSSH_B_OK)
			return error;
	}

	if (numRead < 0) {
		fprintf(stderr, "Error reading directory `%s': %s\n", source,
			fssh_strerror(numRead));
		return numRead;
	}

	return FSSH_B_OK;
}


static fssh_status_t
copy_attribute(const char *source, Node *sourceNode, const char *target,
	Node *targetNode, const char *name, const fssh_attr_info &info)
{
	// remove the attribute first
	targetNode->RemoveAttr(name);

	// special case: empty attribute
	if (info.size <= 0) {
		fssh_ssize_t bytesWritten = targetNode->WriteAttr(name, info.type, 0,
			sCopyBuffer, 0);
		if (bytesWritten) {
			fprintf(stderr, "Error while writing to attribute `%s' of file "
				"`%s': %s\n", name, target, fssh_strerror(bytesWritten));
			return bytesWritten;
		}

		return FSSH_B_OK;
	}

	// non-empty attribute
	fssh_off_t pos = 0;
	int toCopy = info.size;
	while (toCopy > 0) {
		// read data from source
		int toRead = (toCopy < sCopyBufferSize ? toCopy : sCopyBufferSize);
		fssh_ssize_t bytesRead = sourceNode->ReadAttr(name, info.type, pos,
			sCopyBuffer, toRead);
		if (bytesRead < 0) {
			fprintf(stderr, "Error while reading from attribute `%s' of file "
				"`%s': %s\n", name, source, fssh_strerror(bytesRead));
			return bytesRead;
		}

		// write data to target
		fssh_ssize_t bytesWritten = targetNode->WriteAttr(name, info.type, pos,
			sCopyBuffer, bytesRead);
		if (bytesWritten < 0) {
			fprintf(stderr, "Error while writing to attribute `%s' of file "
				"`%s': %s\n", name, target, fssh_strerror(bytesWritten));
			return bytesWritten;
		}

		pos += bytesRead;
		toCopy -= bytesRead;
	}

	return FSSH_B_OK;
}


static fssh_status_t
copy_attributes(const char *source, Node *sourceNode, const char *target,
	Node *targetNode)
{
	char name[B_ATTR_NAME_LENGTH];
	fssh_ssize_t numRead;
	while ((numRead = sourceNode->GetNextAttr(name, sizeof(name))) > 0) {
		fssh_attr_info info;
		// get attribute info
		fssh_status_t error = sourceNode->GetAttrInfo(name, info);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error getting info for attribute `%s' of file "
				"`%s': %s\n", name, source, fssh_strerror(error));
			return error;
		}

		// copy the attribute
		error = copy_attribute(source, sourceNode, target, targetNode, name,
			info);
		if (error != FSSH_B_OK)
			return error;
	}

	if (numRead < 0) {
		fprintf(stderr, "Error reading attribute directory of `%s': %s\n",
			source, fssh_strerror(numRead));
		return numRead;
	}

	return FSSH_B_OK;
}


static fssh_status_t
copy_entry(FSDomain *sourceDomain, const char *source,
	FSDomain *targetDomain, const char *target, const Options &options,
	bool dereference)
{
	// apply entry filter
	if (!options.entryFilter.Filter(source))
		return FSSH_B_OK;

	// open the source node
	Node *sourceNode;
	fssh_status_t error = sourceDomain->Open(source,
		FSSH_O_RDONLY | (dereference ? 0 : FSSH_O_NOTRAVERSE),
		sourceNode);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to open source path `%s': %s\n", source,
			fssh_strerror(error));
		return error;
	}
	NodeDeleter sourceDeleter(sourceNode);

	// check, if target exists
	Node *targetNode = NULL;
	// try opening with resolving symlinks first
	error = targetDomain->Open(target, FSSH_O_RDONLY | FSSH_O_NOTRAVERSE,
		targetNode);
	NodeDeleter targetDeleter;
	if (error == FSSH_B_OK) {
		// 1. target exists:
		//    check, if it is a dir and, if so, whether source is a dir too
		targetDeleter.SetTo(targetNode);

		// if the target is a symlink, try resolving it
		if (targetNode->IsSymLink()) {
			Node *resolvedTargetNode;
			error = targetDomain->Open(target, FSSH_O_RDONLY,
				resolvedTargetNode);
			if (error == FSSH_B_OK) {
				targetNode = resolvedTargetNode;
				targetDeleter.SetTo(targetNode);
			}
		}

		if (sourceNode->IsDirectory() && targetNode->IsDirectory()) {
			// 1.1. target and source are dirs:
			//      -> just copy their contents
			// ...
		} else {
			// 1.2. source and/or target are no dirs

			if (options.force) {
				// 1.2.1. /force/
				//        -> remove the target and continue with 2.
				targetDeleter.Delete();
				targetNode = NULL;
				error = targetDomain->Unlink(target);
				if (error != FSSH_B_OK) {
					fprintf(stderr, "Error: Failed to remove `%s'\n", target);
					return error;
				}
			} else if (sourceNode->IsFile() && targetNode->IsFile()) {
				// 1.2.1.1. !/force/, but both source and target are files
				//          -> truncate the target file and continue
				targetDeleter.Delete();
				targetNode = NULL;
				error = targetDomain->Open(target, FSSH_O_RDWR | FSSH_O_TRUNC,
					targetNode);
				if (error != FSSH_B_OK) {
					fprintf(stderr, "Error: Failed to open `%s' for writing\n",
						target);
					return error;
				}
			} else {
				// 1.2.1.2. !/force/, source or target isn't a file
				//          -> fail
				fprintf(stderr, "Error: File `%s' does exist.\n", target);
				return FSSH_B_FILE_EXISTS;
			}
		}
	} // else: 2. target doesn't exist: -> just create it

	// create the target node
	error = FSSH_B_OK;
	if (sourceNode->IsFile()) {
		if (!targetNode) {
			File *file = NULL;
			error = targetDomain->CreateFile(target, sourceNode->Stat(), file);
			if (error == 0)
				targetNode = file;
		}
	} else if (sourceNode->IsDirectory()) {
		// check /recursive/
		if (!options.recursive) {
			fprintf(stderr, "Error: Entry `%s' is a directory.\n", source);
			return FSSH_EISDIR;
		}

		// create the target only, if it doesn't already exist
		if (!targetNode) {
			Directory *dir = NULL;
			error = targetDomain->CreateDirectory(target, sourceNode->Stat(),
				dir);
			if (error == 0)
				targetNode = dir;
		}
	} else if (sourceNode->IsSymLink()) {
		// read the source link
		SymLink *sourceLink = sourceNode->ToSymLink();
		char linkTo[FSSH_B_PATH_NAME_LENGTH];
		fssh_ssize_t bytesRead = sourceLink->ReadLink(linkTo,
			sizeof(linkTo) - 1);
		if (bytesRead < 0) {
			fprintf(stderr, "Error: Failed to read symlink `%s': %s\n", source,
				fssh_strerror(bytesRead));
		}
		linkTo[bytesRead] = '\0';	// always NULL-terminate

		// create the target link
		SymLink *link;
		error = targetDomain->CreateSymLink(target, linkTo,
			sourceNode->Stat(),	link);
		if (error == 0)
			targetNode = link;
	} else {
		fprintf(stderr, "Error: Unknown node type. We shouldn't be here!\n");
		return FSSH_EINVAL;
	}

	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to create `%s': %s\n", target,
			fssh_strerror(error));
		return error;
	}
	targetDeleter.SetTo(targetNode);

	// copy attributes
	if (!options.ignoreAttributes) {
		error = copy_attributes(source, sourceNode, target, targetNode);
		if (error != FSSH_B_OK)
			return error;
	}

	// copy contents
	if (sourceNode->IsFile()) {
		error = copy_file_contents(source, sourceNode->ToFile(), target,
			targetNode->ToFile());
	} else if (sourceNode->IsDirectory()) {
		error = copy_dir_contents(sourceDomain, source,
			sourceNode->ToDirectory(), targetDomain, target, options);
	}

	return error;
}


fssh_status_t
command_cp(int argc, const char* const* argv)
{
	int sourceCount = 0;
	Options options;

	const char **sources = new const char*[argc];
	if (!sources) {
		fprintf(stderr, "Error: No memory!\n");
		return FSSH_EINVAL;
	}
	ArrayDeleter<const char*> _(sources);

	// parse parameters
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (arg[1] == '\0') {
				fprintf(stderr, "Error: Invalid option '-'\n");
				return FSSH_EINVAL;
			}

			if (arg[1] == '-') {
				if (strcmp(arg, "--ignore-attributes") == 0) {
					options.ignoreAttributes = true;
				} else {
					fprintf(stderr, "Error: Unknown option '%s'\n", arg);
					return FSSH_EINVAL;
				}
			} else {
				for (int i = 1; arg[i]; i++) {
					switch (arg[i]) {
						case 'a':
							options.attributesOnly = true;
							break;
						case 'd':
							options.dereference = false;
							break;
						case 'f':
							options.force = true;
							break;
						case 'L':
							options.dereference = true;
							options.alwaysDereference = true;
							break;
						case 'r':
							options.recursive = true;
							break;
						case 'x':
						case 'X':
						{
							const char* pattern;
							if (arg[i + 1] == '\0') {
								if (++argi >= argc) {
									fprintf(stderr, "Error: Option '-%c' need "
										"a pattern as parameter\n", arg[i]);
									return FSSH_EINVAL;
								}
								pattern = argv[argi];
							} else
								pattern = arg + i + 1;

							options.entryFilter.AddExcludeFilter(pattern,
								arg[i] == 'x');
							break;
						}
						default:
							fprintf(stderr, "Error: Unknown option '-%c'\n",
								arg[i]);
							return FSSH_EINVAL;
					}
				}
			}
		} else {
			sources[sourceCount++] = arg;
		}
	}

	// check params
	if (sourceCount < 2) {
		fprintf(stderr, "Error: Must specify at least 2 files!\n");
		return FSSH_EINVAL;
	}

	// check the target
	const char *target = sources[--sourceCount];
	bool targetIsDir = false;
	bool targetExists = false;
	FSDomain *targetDomain = get_file_domain(target, target);
	DomainDeleter targetDomainDeleter(targetDomain);

	Node *targetNode;
	fssh_status_t error = targetDomain->Open(target, FSSH_O_RDONLY, targetNode);
	if (error == 0) {
		NodeDeleter targetDeleter(targetNode);
		targetExists = true;

		if (options.attributesOnly) {
			// That's how it should be; we don't care whether the target is
			// a directory or not. We append the attributes to that node in
			// either case.
		} else if (targetNode->IsDirectory()) {
			targetIsDir = true;
		} else {
			if (sourceCount > 1) {
				fprintf(stderr, "Error: Destination `%s' is not a directory!",
					target);
				return FSSH_B_NOT_A_DIRECTORY;
			}
		}
	} else {
		if (options.attributesOnly) {
			fprintf(stderr, "Error: Failed to open target `%s' (it must exist "
				"in attributes only mode): `%s'\n", target,
				fssh_strerror(error));
			return error;
		} else if (sourceCount > 1) {
			fprintf(stderr, "Error: Failed to open destination directory `%s':"
				" `%s'\n", target, fssh_strerror(error));
			return error;
		}
	}

	// allocate a copy buffer
	sCopyBuffer = malloc(sCopyBufferSize);
	if (!sCopyBuffer) {
		fprintf(stderr, "Error: Failed to allocate copy buffer.\n");
		return FSSH_ENOMEM;
	}
	MemoryDeleter copyBufferDeleter(sCopyBuffer);

	// open the target node for attributes only mode
	NodeDeleter targetDeleter;
	if (options.attributesOnly) {
		error = targetDomain->Open(target, FSSH_O_RDONLY, targetNode);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to open target `%s' for writing: "
				"`%s'\n", target, fssh_strerror(error));
			return error;
		}

		targetDeleter.SetTo(targetNode);
	}

	// the copy loop
	for (int i = 0; i < sourceCount; i++) {
		const char *source = sources[i];
		FSDomain *sourceDomain = get_file_domain(source, source);
		DomainDeleter sourceDomainDeleter(sourceDomain);
		if (options.attributesOnly) {
			// 0. copy attributes only
			// open the source node
			Node *sourceNode;
			error = sourceDomain->Open(source,
				FSSH_O_RDONLY | (options.dereference ? 0 : FSSH_O_NOTRAVERSE),
				sourceNode);
			if (error != FSSH_B_OK) {
				fprintf(stderr, "Error: Failed to open `%s': %s.\n", source,
					fssh_strerror(error));
				return error;
			}
			NodeDeleter sourceDeleter(sourceNode);

			// copy the attributes
			error = copy_attributes(source, sourceNode, target, targetNode);

		} else if (targetExists && targetIsDir) {
			// 1. target exists:
			// 1.1. target is a dir:
			// get the source leaf name
			char leafName[FSSH_B_FILE_NAME_LENGTH];
			error = get_last_path_component(source, leafName, sizeof(leafName));
			if (error != FSSH_B_OK) {
				fprintf(stderr, "Error: Failed to get last path component of "
					"`%s': %s\n", source, fssh_strerror(error));
				return error;
			}

			if (strcmp(leafName, ".") == 0 || strcmp(leafName, "..") == 0) {
				// 1.1.1. source name is `.' or `..'
				//        -> copy the contents only
				//           (copy_dir_contents())
				// open the source dir
				Node *sourceNode;
				error = sourceDomain->Open(source,
					FSSH_O_RDONLY
						| (options.dereference ? 0 : FSSH_O_NOTRAVERSE),
					sourceNode);
				if (error != FSSH_B_OK) {
					fprintf(stderr, "Error: Failed to open `%s': %s.\n", source,
						fssh_strerror(error));
					return error;
				}
				NodeDeleter sourceDeleter(sourceNode);

				// check, if it is a dir
				Directory *sourceDir = sourceNode->ToDirectory();
				if (!sourceDir) {
					fprintf(stderr, "Error: Source `%s' is not a directory "
						"although it's last path component is `%s'\n", source,
						leafName);
					return FSSH_EINVAL;
				}

				error = copy_dir_contents(sourceDomain, source, sourceDir,
					targetDomain, target, options);
			} else {
				// 1.1.2. source has normal name
				//        -> we copy into the dir
				//           (copy_entry(<source>, <target>/<source leaf>))
				// compose a new target path name
				char *targetEntry = make_path(target, leafName);
				if (!targetEntry) {
					fprintf(stderr, "Error: Failed to allocate target path!\n");
					return FSSH_ENOMEM;
				}
				PathDeleter targetDeleter(targetEntry);

				error = copy_entry(sourceDomain, source, targetDomain,
					targetEntry, options, options.dereference);
			}
		} else {
			// 1.2. target is no dir:
			//      -> if /force/ is given, we replace the target, otherwise
			//         we fail
			//         (copy_entry(<source>, <target>))
			// or
			// 2. target doesn't exist:
			//    -> we create the target as a clone of the source
			//         (copy_entry(<source>, <target>))
			error = copy_entry(sourceDomain, source, targetDomain, target,
				options, options.dereference);
		}

		if (error != 0)
			return error;
	}

	return FSSH_B_OK;
}


}	// namespace FSShell
