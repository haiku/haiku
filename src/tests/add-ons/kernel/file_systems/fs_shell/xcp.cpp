/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "compat.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <fs_attr.h>
#include <StorageDefs.h>

#include "kprotos.h"
#include "path_util.h"
#include "stat_util.h"
#include "xcp.h"

static void *sCopyBuffer = NULL;
static const int sCopyBufferSize = 64 * 1024;	// 64 KB

struct Options {
	Options()
		: dereference(true),
		  force(false),
		  recursive(false)
	{
	}

	bool	dereference;
	bool	force;
	bool	recursive;
};

class Directory;
class File;
class SymLink;

// Node
class Node {
public:
	Node() {}
	virtual ~Node() {}

	const my_stat &Stat() const	{ return fStat; }
	bool IsFile() const			{ return MY_S_ISREG(fStat.mode); }
	bool IsDirectory() const	{ return MY_S_ISDIR(fStat.mode); }
	bool IsSymLink() const		{ return MY_S_ISLNK(fStat.mode); }

	virtual File *ToFile()				{ return NULL; }
	virtual Directory *ToDirectory()	{ return NULL; }
	virtual SymLink *ToSymLink()		{ return NULL; }

	virtual	ssize_t GetNextAttr(char *name, int size) = 0;
	virtual status_t GetAttrInfo(const char *name, my_attr_info &info) = 0;
	virtual ssize_t ReadAttr(const char *name, uint32 type, fs_off_t pos,
		void *buffer, int size) = 0;
	virtual ssize_t WriteAttr(const char *name, uint32 type, fs_off_t pos,
		const void *buffer, int size) = 0;
	virtual status_t RemoveAttr(const char *name) = 0;

protected:
	struct my_stat	fStat;	// To be initialized by implementing classes.
};

// Directory
class Directory : public virtual Node {
public:
	virtual Directory *ToDirectory()	{ return this; }

	virtual	ssize_t GetNextEntry(struct my_dirent *entry, int size) = 0;
};

// File
class File : public virtual Node {
public:
	virtual File *ToFile()				{ return this; }

	virtual ssize_t Read(void *buffer, int size) = 0;
	virtual ssize_t Write(const void *buffer, int size) = 0;
};

// SymLink
class SymLink : public virtual Node {
public:
	virtual SymLink *ToSymLink()		{ return this; }

	virtual ssize_t ReadLink(char *buffer, int bufferSize) = 0;
};

// FSDomain
class FSDomain {
public:
	virtual status_t Open(const char *path, int openMode, Node *&node) = 0;
	
	virtual status_t CreateFile(const char *path, const struct my_stat &st,
		File *&file) = 0;
	virtual status_t CreateDirectory(const char *path, const struct my_stat &st,
		Directory *&dir) = 0;
	virtual status_t CreateSymLink(const char *path, const char *linkTo,
		const struct my_stat &st, SymLink *&link) = 0;

	virtual status_t Unlink(const char *path) = 0;
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
			close(fFD);
		if (fAttrDir)
			fs_close_attr_dir(fAttrDir);
	}

	virtual status_t Init(const char *path, int fd, const my_stat &st)
	{
		fFD = fd;
		fStat = st;

		// open the attribute directory
		fAttrDir = fs_fopen_attr_dir(fd);
		if (!fAttrDir)
			return from_platform_error(errno);

		return FS_OK;
	}

	virtual	ssize_t GetNextAttr(char *name, int size)
	{
		if (!fAttrDir)
			return 0;
		
		errno = 0;
		struct dirent *entry = fs_read_attr_dir(fAttrDir);
		if (!entry)
			return from_platform_error(errno);

		int len = strlen(entry->d_name);
		if (len >= size)
			return FS_NAME_TOO_LONG;

		strcpy(name, entry->d_name);
		return 1;
	}

	virtual status_t GetAttrInfo(const char *name, my_attr_info &info)
	{
		attr_info hostInfo;
		if (fs_stat_attr(fFD, name, &hostInfo) < 0)
			return from_platform_error(errno);

		info.type = hostInfo.type;
		info.size = hostInfo.size;
		return FS_OK;
	}

	virtual ssize_t ReadAttr(const char *name, uint32 type, fs_off_t pos,
		void *buffer, int size)
	{
		ssize_t bytesRead = fs_read_attr(fFD, name, type, pos, buffer, size);
		return (bytesRead >= 0 ? bytesRead : from_platform_error(errno));
	}

	virtual ssize_t WriteAttr(const char *name, uint32 type, fs_off_t pos,
		const void *buffer, int size)
	{
		ssize_t bytesWritten = fs_write_attr(fFD, name, type, pos, buffer,
			size);
		return (bytesWritten >= 0 ? bytesWritten : from_platform_error(errno));
	}

	virtual status_t RemoveAttr(const char *name)
	{
		return (fs_remove_attr(fFD, name) == 0
			? 0 : from_platform_error(errno));
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

	virtual status_t Init(const char *path, int fd, const my_stat &st)
	{
		status_t error = HostNode::Init(path, fd, st);
		if (error != FS_OK)
			return error;

		fDir = opendir(path);
		if (!fDir)
			return from_platform_error(errno);

		return FS_OK;
	}

	virtual	ssize_t GetNextEntry(struct my_dirent *entry, int size)
	{
		errno = 0;
		struct dirent *hostEntry = readdir(fDir);
		if (!hostEntry)
			return from_platform_error(errno);

		int nameLen = strlen(hostEntry->d_name);
		int recLen = entry->d_name + nameLen + 1 - (char*)entry;
		if (recLen > size)
			return FS_NAME_TOO_LONG;

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

	virtual ssize_t Read(void *buffer, int size)
	{
		ssize_t bytesRead = read(fFD, buffer, size);
		return (bytesRead >= 0 ? bytesRead : from_platform_error(errno));
	}

	virtual ssize_t Write(const void *buffer, int size)
	{
		ssize_t bytesWritten = write(fFD, buffer, size);
		return (bytesWritten >= 0 ? bytesWritten : from_platform_error(errno));
	}
};

// HostSymLink
class HostSymLink : public SymLink, public HostNode {
public:
	HostSymLink()
		: SymLink(),
		  HostNode()
	{
	}

	virtual ~HostSymLink()
	{
		if (fPath)
			free(fPath);
	}

	virtual status_t Init(const char *path, int fd, const my_stat &st)
	{
		status_t error = HostNode::Init(path, fd, st);
		if (error != FS_OK)
			return error;

		fPath = strdup(path);
		if (!fPath)
			return FS_NO_MEMORY;

		return FS_OK;
	}

	virtual ssize_t ReadLink(char *buffer, int bufferSize)
	{
		ssize_t bytesRead = readlink(fPath, buffer, bufferSize);
		return (bytesRead >= 0 ? bytesRead : from_platform_error(errno));
	}

private:
	char	*fPath;
};

// HostFSDomain
class HostFSDomain : public FSDomain {
public:
	HostFSDomain() {}
	virtual ~HostFSDomain() {}

	virtual status_t Open(const char *path, int openMode, Node *&_node)
	{
		// open the node
		int fd = open(path, to_platform_open_mode(openMode));
		if (fd < 0)
			return from_platform_error(errno);

		// stat the node
		struct stat st;
		if (fstat(fd, &st) < 0) {
			close(fd);
			return from_platform_error(errno);
		}

		// check the node type and create the node
		HostNode *node = NULL;
		switch (st.st_mode & S_IFMT) {
			case S_IFLNK:
				node = new HostSymLink;
				break;
			case S_IFREG:
				node = new HostFile;
				break;
			case S_IFDIR:
				node = new HostDirectory;
				break;
			default:
				close(fd);
				return FS_EINVAL;
		}

		// convert the stat
		struct my_stat myst;
		from_platform_stat(&st, &myst);

		// init the node
		status_t error = node->Init(path, fd, myst);
			// the node receives ownership of the FD
		if (error != FS_OK) {
			delete node;
			return error;
		}

		_node = node;
		return FS_OK;
	}
	
	virtual status_t CreateFile(const char *path, const struct my_stat &myst,
		File *&_file)
	{
		struct stat st;
		to_platform_stat(&myst, &st);

		// create the file
		int fd = creat(path, st.st_mode & S_IUMSK);
		if (fd < 0)
			return from_platform_error(errno);

		// apply the other stat fields
		status_t error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			close(fd);
			return error;
		}

		// create the object
		HostFile *file = new HostFile;
		error = file->Init(path, fd, myst);
		if (error != FS_OK) {
			delete file;
			return error;
		}

		_file = file;
		return FS_OK;
	}

	virtual status_t CreateDirectory(const char *path,
		const struct my_stat &myst, Directory *&_dir)
	{
		struct stat st;
		to_platform_stat(&myst, &st);

		// create the dir
		if (mkdir(path, st.st_mode & S_IUMSK) < 0)
			return from_platform_error(errno);

		// open the dir node
		int fd = open(path, O_RDONLY | O_NOTRAVERSE);
		if (fd < 0)
			return from_platform_error(errno);

		// apply the other stat fields
		status_t error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			close(fd);
			return error;
		}

		// create the object
		HostDirectory *dir = new HostDirectory;
		error = dir->Init(path, fd, myst);
		if (error != FS_OK) {
			delete dir;
			return error;
		}

		_dir = dir;
		return FS_OK;
	}

	virtual status_t CreateSymLink(const char *path, const char *linkTo,
		const struct my_stat &myst, SymLink *&_link)
	{
		struct stat st;
		to_platform_stat(&myst, &st);

		// create the link
		if (symlink(linkTo, path) < 0)
			return from_platform_error(errno);

		// open the symlink node
		int fd = open(path, O_RDONLY | O_NOTRAVERSE);
		if (fd < 0)
			return from_platform_error(errno);

		// apply the other stat fields
		status_t error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			close(fd);
			return error;
		}

		// create the object
		HostSymLink *link = new HostSymLink;
		error = link->Init(path, fd, myst);
		if (error != FS_OK) {
			delete link;
			return error;
		}

		_link = link;
		return FS_OK;
	}


	virtual status_t Unlink(const char *path)
	{
		if (unlink(path) < 0)
			return from_platform_error(errno);
		return FS_OK;
	}

private:
	status_t _ApplyStat(int fd, const struct stat &st)
	{
		// TODO: Set times...
		return FS_OK;
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
			sys_close(true, fFD);
		if (fAttrDir)
			sys_closedir(true, fAttrDir);
	}

	virtual status_t Init(const char *path, int fd, const my_stat &st)
	{
		fFD = fd;
		fStat = st;

		// open the attribute directory
		fAttrDir = sys_open_attr_dir(true, fd, NULL);
		if (fAttrDir < 0)
			return fAttrDir;

		return FS_OK;
	}

	virtual	ssize_t GetNextAttr(char *name, int size)
	{
		if (fAttrDir < 0)
			return 0;
		
		char buffer[sizeof(my_dirent) + B_ATTR_NAME_LENGTH];
		struct my_dirent *entry = (my_dirent *)buffer;
		int numRead = sys_readdir(true, fAttrDir, entry, sizeof(buffer), 1);
		if (numRead < 0)
			return numRead;
		if (numRead == 0)
			return 0;

		int len = strlen(entry->d_name);
		if (len >= size)
			return FS_NAME_TOO_LONG;

		strcpy(name, entry->d_name);
		return 1;
	}

	virtual status_t GetAttrInfo(const char *name, my_attr_info &info)
	{
		return sys_stat_attr(true, fFD, NULL, name, &info);
	}

	virtual ssize_t ReadAttr(const char *name, uint32 type, fs_off_t pos,
		void *buffer, int size)
	{
		return sys_read_attr(true, fFD, name, type, buffer, size, pos);
	}

	virtual ssize_t WriteAttr(const char *name, uint32 type, fs_off_t pos,
		const void *buffer, int size)
	{
		return sys_write_attr(true, fFD, name, type, buffer, size, pos);
	}

	virtual status_t RemoveAttr(const char *name)
	{
		return sys_remove_attr(true, fFD, name);
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
			sys_closedir(true, fDir);
	}

	virtual status_t Init(const char *path, int fd, const my_stat &st)
	{
		status_t error = GuestNode::Init(path, fd, st);
		if (error != FS_OK)
			return error;

		fDir = sys_opendir(true, fd, NULL, true);
		if (fDir < 0)
			return fDir;

		return FS_OK;
	}

	virtual	ssize_t GetNextEntry(struct my_dirent *entry, int size)
	{
		return sys_readdir(true, fDir, entry, size, 1);
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

	virtual ssize_t Read(void *buffer, int size)
	{
		return sys_read(true, fFD, buffer, size);
	}

	virtual ssize_t Write(const void *buffer, int size)
	{
		return sys_write(true, fFD, buffer, size);
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

	virtual ssize_t ReadLink(char *buffer, int bufferSize)
	{
		return sys_readlink(true, fFD, NULL, buffer, bufferSize);
	}
};

// GuestFSDomain
class GuestFSDomain : public FSDomain {
public:
	GuestFSDomain() {}
	virtual ~GuestFSDomain() {}

	virtual status_t Open(const char *path, int openMode, Node *&_node)
	{
		// open the node
		int fd = sys_open(true, -1, path, openMode, 0, true);
		if (fd < 0)
			return fd;

		// stat the node
		struct my_stat st;
		status_t error = sys_rstat(true, fd, NULL, &st, false);
		if (error < 0) {
			sys_close(true, fd);
			return error;
		}

		// check the node type and create the node
		GuestNode *node = NULL;
		switch (st.mode & MY_S_IFMT) {
			case MY_S_IFLNK:
				node = new GuestSymLink;
				break;
			case MY_S_IFREG:
				node = new GuestFile;
				break;
			case MY_S_IFDIR:
				node = new GuestDirectory;
				break;
			default:
				sys_close(true, fd);
				return FS_EINVAL;
		}

		// init the node
		error = node->Init(path, fd, st);
			// the node receives ownership of the FD
		if (error != FS_OK) {
			delete node;
			return error;
		}

		_node = node;
		return FS_OK;
	}
	
	virtual status_t CreateFile(const char *path, const struct my_stat &st,
		File *&_file)
	{
		// create the file
		int fd = sys_open(true, -1, path, MY_O_RDWR | MY_O_EXCL | MY_O_CREAT,
			st.mode & MY_S_IUMSK, true);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		status_t error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			sys_close(true, fd);
			return error;
		}

		// create the object
		GuestFile *file = new GuestFile;
		error = file->Init(path, fd, st);
		if (error != FS_OK) {
			delete file;
			return error;
		}

		_file = file;
		return FS_OK;
	}

	virtual status_t CreateDirectory(const char *path, const struct my_stat &st,
		Directory *&_dir)
	{
		// create the dir
		status_t error = sys_mkdir(true, -1, path, st.mode & MY_S_IUMSK);
		if (error < 0)
			return error;

		// open the dir node
		int fd = sys_open(true, -1, path, MY_O_RDONLY | MY_O_NOTRAVERSE,
			0, true);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			sys_close(true, fd);
			return error;
		}

		// create the object
		GuestDirectory *dir = new GuestDirectory;
		error = dir->Init(path, fd, st);
		if (error != FS_OK) {
			delete dir;
			return error;
		}

		_dir = dir;
		return FS_OK;
	}

	virtual status_t CreateSymLink(const char *path, const char *linkTo,
		const struct my_stat &st, SymLink *&_link)
	{
		// create the link
		status_t error = sys_symlink(true, linkTo, -1, path);
		if (error < 0)
			return error;

		// open the symlink node
		int fd = sys_open(true, -1, path, MY_O_RDONLY | MY_O_NOTRAVERSE,
			0, true);
		if (fd < 0)
			return fd;

		// apply the other stat fields
		error = _ApplyStat(fd, st);
		if (error != FS_OK) {
			sys_close(true, fd);
			return error;
		}

		// create the object
		GuestSymLink *link = new GuestSymLink;
		error = link->Init(path, fd, st);
		if (error != FS_OK) {
			delete link;
			return error;
		}

		_link = link;
		return FS_OK;
	}

	virtual status_t Unlink(const char *path)
	{
		return sys_unlink(true, -1, path);
	}

private:
	status_t _ApplyStat(int fd, const struct my_stat &st)
	{
		// TODO: Set times...
		return FS_OK;
	}
};


// #pragma mark -

static status_t copy_entry(FSDomain *sourceDomain, const char *source,
	FSDomain *targetDomain, const char *target, const Options &options);

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


static status_t
copy_file_contents(const char *source, File *sourceFile, const char *target,
	File *targetFile)
{
	ssize_t bytesRead;
	while ((bytesRead = sourceFile->Read(sCopyBuffer, sCopyBufferSize)) > 0) {
		ssize_t bytesWritten = targetFile->Write(sCopyBuffer, bytesRead);
		if (bytesWritten < 0) {
			fprintf(stderr, "Error while writing to file `%s': %s\n",
				target, fs_strerror(bytesWritten));
			return bytesWritten;
		}
	}

	if (bytesRead < 0) {
		fprintf(stderr, "Error while reading from file `%s': %s\n",
			source, fs_strerror(bytesRead));
		return bytesRead;
	}

	return FS_OK;
}


static status_t
copy_dir_contents(FSDomain *sourceDomain, const char *source,
	Directory *sourceDir, FSDomain *targetDomain, const char *target,
	const Options &options)
{
	char buffer[sizeof(my_dirent) + B_FILE_NAME_LENGTH];
	struct my_dirent *entry =  (struct my_dirent *)buffer;
	ssize_t numRead;
	while ((numRead = sourceDir->GetNextEntry(entry, sizeof(buffer))) > 0) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// compose a new source path name
		char *sourceEntry = make_path(source, entry->d_name);
		if (!sourceEntry) {
			fprintf(stderr, "Failed to allocate source path!\n");
			return FS_ENOMEM;
		}
		PathDeleter sourceDeleter(sourceEntry);

		// compose a new target path name
		char *targetEntry = make_path(target, entry->d_name);
		if (!targetEntry) {
			fprintf(stderr, "Failed to allocate target path!\n");
			return FS_ENOMEM;
		}
		PathDeleter targetDeleter(targetEntry);

		status_t error = copy_entry(sourceDomain, sourceEntry, targetDomain,
			targetEntry, options);
		if (error != FS_OK)
			return error;
	}

	if (numRead < 0) {
		fprintf(stderr, "Error reading directory `%s': %s\n", source,
			fs_strerror(numRead));
		return numRead;
	}

	return FS_OK;
}


static status_t
copy_attribute(const char *source, Node *sourceNode, const char *target,
	Node *targetNode, const char *name, const my_attr_info &info)
{
	// remove the attribute first
	targetNode->RemoveAttr(name);

	// special case: empty attribute
	if (info.size <= 0) {
		ssize_t bytesWritten = targetNode->WriteAttr(name, info.type, 0,
			sCopyBuffer, 0);
		if (bytesWritten) {
			fprintf(stderr, "Error while writing to attribute `%s' of file "
				"`%s': %s\n", name, target, fs_strerror(bytesWritten));
			return bytesWritten;
		}

		return FS_OK;
	}

	// non-empty attribute
	fs_off_t pos = 0;
	int toCopy = info.size;
	while (toCopy > 0) {
		// read data from source
		int toRead = (toCopy < sCopyBufferSize ? toCopy : sCopyBufferSize);
		ssize_t bytesRead = sourceNode->ReadAttr(name, info.type, pos,
			sCopyBuffer, toRead);
		if (bytesRead < 0) {
			fprintf(stderr, "Error while reading from attribute `%s' of file "
				"`%s': %s\n", name, source, fs_strerror(bytesRead));
			return bytesRead;
		}

		// write data to target
		ssize_t bytesWritten = targetNode->WriteAttr(name, info.type, pos,
			sCopyBuffer, bytesRead);
		if (bytesWritten < 0) {
			fprintf(stderr, "Error while writing to attribute `%s' of file "
				"`%s': %s\n", name, target, fs_strerror(bytesWritten));
			return bytesWritten;
		}

		pos += bytesRead;
		toCopy -= bytesRead;
	}

	return FS_OK;
}


static status_t
copy_attributes(const char *source, Node *sourceNode, const char *target,
	Node *targetNode)
{
	char name[B_ATTR_NAME_LENGTH];
	ssize_t numRead;
	while ((numRead = sourceNode->GetNextAttr(name, sizeof(name))) > 0) {
		my_attr_info info;
		// get attribute info
		status_t error = sourceNode->GetAttrInfo(name, info);
		if (error != FS_OK) {
			fprintf(stderr, "Error getting info for attribute `%s' of file "
				"`%s': %s\n", name, source, fs_strerror(error));
			return error;
		}

		// copy the attribute
		error = copy_attribute(source, sourceNode, target, targetNode, name,
			info);
		if (error != FS_OK)
			return error;
	}

	if (numRead < 0) {
		fprintf(stderr, "Error reading attribute directory of `%s': %s\n",
			source, fs_strerror(numRead));
		return numRead;
	}

	return FS_OK;
}


static status_t
copy_entry(FSDomain *sourceDomain, const char *source,
	FSDomain *targetDomain, const char *target, const Options &options)
{
	// open the source node
	Node *sourceNode;
	status_t error = sourceDomain->Open(source,
		MY_O_RDONLY | (options.dereference ? 0 : MY_O_NOTRAVERSE),
		sourceNode);
	if (error != FS_OK) {
		fprintf(stderr, "Failed to open source path `%s': %s\n", source,
			fs_strerror(error));
		return error;
	}
	NodeDeleter sourceDeleter(sourceNode);

	// check, if target exists
	Node *targetNode = NULL;
	// try opening with resolving symlinks first
	error = targetDomain->Open(target, MY_O_RDONLY | MY_O_NOTRAVERSE,
		targetNode);
	NodeDeleter targetDeleter;
	if (error == FS_OK) {
		// 1. target exists:
		//    check, if it is a dir and, if so, whether source is a dir too
		targetDeleter.SetTo(targetNode);

		// if the target is a symlink, try resolving it
		if (targetNode->IsSymLink()) {
			Node *resolvedTargetNode;
			error = targetDomain->Open(target, MY_O_RDONLY, resolvedTargetNode);
			if (error == FS_OK) {
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
				if (error != FS_OK) {
					fprintf(stderr, "Failed to remove `%s'\n", target);
					return error;
				}
			} else if (sourceNode->IsFile() && targetNode->IsFile()) {
				// 1.2.1.1. !/force/, but both source and target are files
				//          -> truncate the target file and continue
				targetDeleter.Delete();
				targetNode = NULL;
				error = targetDomain->Open(target, MY_O_RDWR | MY_O_TRUNC,
					targetNode);
				if (error != FS_OK) {
					fprintf(stderr, "Failed to open `%s' for writing\n",
						target);
					return error;
				}
			} else {
				// 1.2.1.2. !/force/, source or target isn't a file
				//          -> fail
				fprintf(stderr, "File `%s' does exist.\n", target);
				return FS_FILE_EXISTS;
			}
		}
	} // else: 2. target doesn't exist: -> just create it

	// create the target node
	error = FS_OK;
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
			fprintf(stderr, "Entry `%s' is a directory.\n", source);
			return FS_EISDIR;
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
		char linkTo[B_PATH_NAME_LENGTH];
		ssize_t bytesRead = sourceLink->ReadLink(linkTo, sizeof(linkTo) - 1);
		if (bytesRead < 0) {
			fprintf(stderr, "Failed to read symlink `%s': %s\n", source,
				fs_strerror(bytesRead));
		}
		linkTo[bytesRead] = '\0';	// always NULL-terminate
		
		// create the target link
		SymLink *link;
		error = targetDomain->CreateSymLink(target, linkTo,
			sourceNode->Stat(),	link);
		if (error == 0)
			targetNode = link;
	} else {
		fprintf(stderr, "Unknown node type. We shouldn't be here!\n");
		return FS_EINVAL;
	}

	if (error != FS_OK) {
		fprintf(stderr, "Failed to create `%s': %s\n", target,
			fs_strerror(error));
		return error;
	}
	targetDeleter.SetTo(targetNode);

	// copy attributes
	error = copy_attributes(source, sourceNode, target, targetNode);
	if (error != FS_OK)
		return error;

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


int
do_xcp(int argc, char **argv)
{
	int sourceCount = 0;
	Options options;

	const char **sources = new const char*[argc];
	if (!sources) {
		fprintf(stderr, "No memory!\n");
		return FS_EINVAL;
	}
	ArrayDeleter<const char*> _(sources);

	// parse parameters
	for (int argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] == '-') {
			if (arg[1] == '\0') {
				fprintf(stderr, "Invalid option `-'\n");
				return FS_EINVAL;
			}

			for (int i = 1; arg[i]; i++) {
				switch (arg[i]) {
					case 'd':
						options.dereference = false;
						break;
					case 'f':
						options.force = true;
						break;
					case 'r':
						options.recursive = true;
						break;
					default:
						fprintf(stderr, "Unknown option `-%c'\n", arg[i]);
						return FS_EINVAL;
				}
			}
		} else {
			sources[sourceCount++] = arg;
		}
	}

	// check params
	if (sourceCount < 2) {
		fprintf(stderr, "Must specify at least 2 files!\n");
		return FS_EINVAL;
	}

	// check the target
	const char *target = sources[--sourceCount];
	bool targetIsDir = false;
	bool targetExists = false;
	FSDomain *targetDomain = get_file_domain(target, target);
	DomainDeleter targetDomainDeleter(targetDomain);

	Node *targetNode;
	status_t error = targetDomain->Open(target, MY_O_RDONLY, targetNode);
	if (error == 0) {
		NodeDeleter targetDeleter(targetNode);
		targetExists = true;

		if (targetNode->IsDirectory()) {
			targetIsDir = true;
		} else {
			if (sourceCount > 1) {
				fprintf(stderr, "Destination `%s' is not a directory!",
					target);
				return FS_NOT_A_DIRECTORY;
			}
		}
	} else {
		if (sourceCount > 1) {
			fprintf(stderr, "Failed to open destination directory `%s': `%s'\n",
				target, fs_strerror(error));
			return error;
		}
	}

	// allocate a copy buffer
	sCopyBuffer = malloc(sCopyBufferSize);
	if (!sCopyBuffer) {
		fprintf(stderr, "Failed to allocate copy buffer.\n");
		return FS_ENOMEM;
	}
	MemoryDeleter copyBufferDeleter(sCopyBuffer);

	// the copy loop
	for (int i = 0; i < sourceCount; i++) {
		const char *source = sources[i];
		FSDomain *sourceDomain = get_file_domain(source, source);
		DomainDeleter sourceDomainDeleter(sourceDomain);
		if (targetExists && targetIsDir) {
			// 1. target exists:
			// 1.1. target is a dir:
			// get the source leaf name
			char leafName[B_FILE_NAME_LENGTH];
			error = get_last_path_component(source, leafName, sizeof(leafName));
			if (error != FS_OK) {
				fprintf(stderr, "Failed to get last path component of `%s': "
					"%s\n", source, fs_strerror(error));
				return error;
			}

			if (strcmp(leafName, ".") == 0 || strcmp(leafName, "..") == 0) {
				// 1.1.1. source name is `.' or `..'
				//        -> copy the contents only
				//           (copy_dir_contents())
				// open the source dir
				Node *sourceNode;
				error = sourceDomain->Open(source,
					MY_O_RDONLY | (options.dereference ? 0 : MY_O_NOTRAVERSE),
					sourceNode);
				if (error != FS_OK) {
					fprintf(stderr, "Failed to open `%s': %s.\n", source,
						fs_strerror(error));
					return error;
				}
				NodeDeleter sourceDeleter(sourceNode);

				// check, if it is a dir
				Directory *sourceDir = sourceNode->ToDirectory();
				if (!sourceDir) {
					fprintf(stderr, "Source `%s' is not a directory although"
						"it's last path component is `%s'\n", source, leafName);
					return FS_EINVAL;
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
					fprintf(stderr, "Failed to allocate target path!\n");
					return FS_ENOMEM;
				}
				PathDeleter targetDeleter(targetEntry);

				error = copy_entry(sourceDomain, source, targetDomain,
					targetEntry, options);
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
				options);
		}

		if (error != 0)
			return error;
	}

	return 0;
}

