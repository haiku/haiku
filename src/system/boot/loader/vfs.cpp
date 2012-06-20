/*
 * Copyright 2003-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/vfs.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>
#include <unistd.h>

#include <StorageDefs.h>

#include <boot/platform.h>
#include <boot/partitions.h>
#include <boot/stdio.h>
#include <boot/stage2.h>
#include <util/kernel_cpp.h>
#include <syscall_utils.h>

#include "RootFileSystem.h"


using namespace boot;

//#define TRACE_VFS
#ifdef TRACE_VFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


class Descriptor {
	public:
		Descriptor(Node *node, void *cookie);
		~Descriptor();

		ssize_t ReadAt(off_t pos, void *buffer, size_t bufferSize);
		ssize_t Read(void *buffer, size_t bufferSize);
		ssize_t WriteAt(off_t pos, const void *buffer, size_t bufferSize);
		ssize_t Write(const void *buffer, size_t bufferSize);

		status_t Stat(struct stat &stat);

		off_t Offset() const { return fOffset; }
		int32 RefCount() const { return fRefCount; }

		status_t Acquire();
		status_t Release();

		Node *GetNode() const { return fNode; }

	private:
		Node	*fNode;
		void	*fCookie;
		off_t	fOffset;
		int32	fRefCount;
};

#define MAX_VFS_DESCRIPTORS 64

NodeList gBootDevices;
NodeList gPartitions;
RootFileSystem *gRoot;
static Descriptor *sDescriptors[MAX_VFS_DESCRIPTORS];
static Node *sBootDevice;


Node::Node()
	:
	fRefCount(1)
{
}


Node::~Node()
{
}


status_t
Node::Open(void **_cookie, int mode)
{
	TRACE(("%p::Open()\n", this));
	return Acquire();
}


status_t
Node::Close(void *cookie)
{
	TRACE(("%p::Close()\n", this));
	return Release();
}


status_t
Node::GetName(char *nameBuffer, size_t bufferSize) const
{
	return B_ERROR;
}


status_t
Node::GetFileMap(struct file_map_run *runs, int32 *count)
{
	return B_ERROR;
}


int32
Node::Type() const
{
	return 0;
}


off_t
Node::Size() const
{
	return 0LL;
}


ino_t
Node::Inode() const
{
	return 0;
}


status_t
Node::Acquire()
{
	fRefCount++;
	TRACE(("%p::Acquire(), fRefCount = %ld\n", this, fRefCount));
	return B_OK;
}


status_t
Node::Release()
{
	TRACE(("%p::Release(), fRefCount = %ld\n", this, fRefCount));
	if (--fRefCount == 0) {
		TRACE(("delete node: %p\n", this));
		delete this;
		return 1;
	}

	return B_OK;
}


//	#pragma mark -


ConsoleNode::ConsoleNode()
	: Node()
{
}


ssize_t
ConsoleNode::Read(void *buffer, size_t bufferSize)
{
	return ReadAt(NULL, -1, buffer, bufferSize);
}


ssize_t
ConsoleNode::Write(const void *buffer, size_t bufferSize)
{
	return WriteAt(NULL, -1, buffer, bufferSize);
}


//	#pragma mark -


Directory::Directory()
	: Node()
{
}


ssize_t
Directory::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t
Directory::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


int32
Directory::Type() const
{
	return S_IFDIR;
}


status_t
Directory::CreateFile(const char *name, mode_t permissions, Node **_node)
{
	return EROFS;
}


//	#pragma mark -


MemoryDisk::MemoryDisk(const uint8* data, size_t size, const char* name)
	: Node(),
	  fData(data),
	  fSize(size)
{
	strlcpy(fName, name, sizeof(fName));
}


ssize_t
MemoryDisk::ReadAt(void* cookie, off_t pos, void* buffer, size_t bufferSize)
{
	if (pos >= fSize)
		return 0;

	if (pos + bufferSize > fSize)
		bufferSize = fSize - pos;

	memcpy(buffer, fData + pos, bufferSize);
	return bufferSize;
}


ssize_t
MemoryDisk::WriteAt(void* cookie, off_t pos, const void* buffer,
	size_t bufferSize)
{
	return B_NOT_ALLOWED;
}


off_t
MemoryDisk::Size() const
{
	return fSize;
}


status_t
MemoryDisk::GetName(char *nameBuffer, size_t bufferSize) const
{
	if (!nameBuffer)
		return B_BAD_VALUE;

	strlcpy(nameBuffer, fName, bufferSize);
	return B_OK;
}


//	#pragma mark -


Descriptor::Descriptor(Node *node, void *cookie)
	:
	fNode(node),
	fCookie(cookie),
	fOffset(0),
	fRefCount(1)
{
}


Descriptor::~Descriptor()
{
}


ssize_t
Descriptor::Read(void *buffer, size_t bufferSize)
{
	ssize_t bytesRead = fNode->ReadAt(fCookie, fOffset, buffer, bufferSize);
	if (bytesRead > B_OK)
		fOffset += bytesRead;

	return bytesRead;
}


ssize_t
Descriptor::ReadAt(off_t pos, void *buffer, size_t bufferSize)
{
	return fNode->ReadAt(fCookie, pos, buffer, bufferSize);
}


ssize_t
Descriptor::Write(const void *buffer, size_t bufferSize)
{
	ssize_t bytesWritten = fNode->WriteAt(fCookie, fOffset, buffer, bufferSize);
	if (bytesWritten > B_OK)
		fOffset += bytesWritten;

	return bytesWritten;
}


ssize_t
Descriptor::WriteAt(off_t pos, const void *buffer, size_t bufferSize)
{
	return fNode->WriteAt(fCookie, pos, buffer, bufferSize);
}


status_t
Descriptor::Stat(struct stat &stat)
{
	stat.st_mode = fNode->Type();
	stat.st_size = fNode->Size();
	stat.st_ino = fNode->Inode();

	return B_OK;
}


status_t
Descriptor::Acquire()
{
	fRefCount++;
	return B_OK;
}


status_t
Descriptor::Release()
{
	if (--fRefCount == 0) {
		status_t status = fNode->Close(fCookie);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


//	#pragma mark -


status_t
vfs_init(stage2_args *args)
{
	gRoot = new(nothrow) RootFileSystem();
	if (gRoot == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
register_boot_file_system(Directory *volume)
{
	gRoot->AddLink("boot", volume);

	Partition *partition;
	status_t status = gRoot->GetPartitionFor(volume, &partition);
	if (status != B_OK) {
		dprintf("register_boot_file_system(): could not locate boot volume in root!\n");
		return status;
	}

	gBootVolume.SetInt64(BOOT_VOLUME_PARTITION_OFFSET, partition->offset);

	Node *device = get_node_from(partition->FD());
	if (device == NULL) {
		dprintf("register_boot_file_system(): could not get boot device!\n");
		return B_ERROR;
	}

	return platform_register_boot_device(device);
}


/** Gets the boot device, scans all of its partitions, gets the
 *	boot partition, and mounts its file system.
 *	Returns the file system's root node or NULL for failure.
 */

Directory *
get_boot_file_system(stage2_args *args)
{
	Node *device;
	if (platform_add_boot_device(args, &gBootDevices) < B_OK)
		return NULL;

	// the boot device must be the first device in the list
	device = gBootDevices.First();

	if (add_partitions_for(device, false, true) < B_OK)
		return NULL;

	Partition *partition;
	if (platform_get_boot_partition(args, device, &gPartitions, &partition) < B_OK)
		return NULL;

	Directory *fileSystem;
	status_t status = partition->Mount(&fileSystem, true);

	if (status < B_OK) {
		// this partition doesn't contain any known file system; we
		// don't need it anymore
		gPartitions.Remove(partition);
		delete partition;
		return NULL;
	}

	sBootDevice = device;
	return fileSystem;
}


/** Mounts all file systems recognized on the given device by
 *	calling the add_partitions_for() function on them.
 */

status_t
mount_file_systems(stage2_args *args)
{
	// mount other partitions on boot device (if any)
	NodeIterator iterator = gPartitions.GetIterator();

	Partition *partition = NULL;
	while ((partition = (Partition *)iterator.Next()) != NULL) {
		// don't scan known partitions again
		if (partition->IsFileSystem())
			continue;

		// remove the partition if it doesn't contain a (known) file system
		if (partition->Scan(true) != B_OK && !partition->IsFileSystem()) {
			gPartitions.Remove(partition);
			delete partition;
		}
	}

	// add all block devices the platform has for us

	status_t status = platform_add_block_devices(args, &gBootDevices);
	if (status < B_OK)
		return status;

	iterator = gBootDevices.GetIterator();
	Node *device = NULL, *last = NULL;
	while ((device = iterator.Next()) != NULL) {
		// don't scan former boot device again
		if (device == sBootDevice)
			continue;

		if (add_partitions_for(device, true) == B_OK) {
			// ToDo: we can't delete the object here, because it must
			//	be removed from the list before we know that it was
			//	deleted.

/*			// if the Release() deletes the object, we need to skip it
			if (device->Release() > 0) {
				list_remove_item(&gBootDevices, device);
				device = last;
			}
*/
		}
		last = device;
	}

	if (gPartitions.IsEmpty())
		return B_ENTRY_NOT_FOUND;

#if 0
	void *cookie;
	if (gRoot->Open(&cookie, O_RDONLY) == B_OK) {
		Directory *directory;
		while (gRoot->GetNextNode(cookie, (Node **)&directory) == B_OK) {
			char name[256];
			if (directory->GetName(name, sizeof(name)) == B_OK)
				printf(":: %s (%p)\n", name, directory);

			void *subCookie;
			if (directory->Open(&subCookie, O_RDONLY) == B_OK) {
				while (directory->GetNextEntry(subCookie, name, sizeof(name)) == B_OK) {
					printf("\t%s\n", name);
				}
				directory->Close(subCookie);
			}
		}
		gRoot->Close(cookie);
	}
#endif

	return B_OK;
}


/*!	Resolves \a directory + \a path to a node.
	Note that \a path will be modified by the function.
*/
static status_t
get_node_for_path(Directory *directory, char *path, Node **_node)
{
	directory->Acquire();
		// balance Acquire()/Release() calls

	while (true) {
		Node *nextNode;
		char *nextPath;

		// walk to find the next path component ("path" will point to a single
		// path component), and filter out multiple slashes
		for (nextPath = path + 1; nextPath[0] != '\0' && nextPath[0] != '/'; nextPath++);

		if (*nextPath == '/') {
			*nextPath = '\0';
			do
				nextPath++;
			while (*nextPath == '/');
		}

		nextNode = directory->Lookup(path, true);
		directory->Release();

		if (nextNode == NULL)
			return B_ENTRY_NOT_FOUND;

		path = nextPath;
		if (S_ISDIR(nextNode->Type()))
			directory = (Directory *)nextNode;
		else if (path[0])
			return B_NOT_ALLOWED;

		// are we done?
		if (path[0] == '\0') {
			*_node = nextNode;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


//	#pragma mark -


static Descriptor *
get_descriptor(int fd)
{
	if (fd < 0 || fd >= MAX_VFS_DESCRIPTORS)
		return NULL;

	return sDescriptors[fd];
}


static void
free_descriptor(int fd)
{
	if (fd >= MAX_VFS_DESCRIPTORS)
		return;

	delete sDescriptors[fd];
	sDescriptors[fd] = NULL;
}


/**	Reserves an entry of the descriptor table and
 *	assigns the given node to it.
 */

int
open_node(Node *node, int mode)
{
	if (node == NULL)
		return B_ERROR;

	// get free descriptor

	int fd = 0;
	for (; fd < MAX_VFS_DESCRIPTORS; fd++) {
		if (sDescriptors[fd] == NULL)
			break;
	}
	if (fd == MAX_VFS_DESCRIPTORS)
		return B_ERROR;

	TRACE(("got descriptor %d for node %p\n", fd, node));

	// we got a free descriptor entry, now try to open the node

	void *cookie;
	status_t status = node->Open(&cookie, mode);
	if (status < B_OK)
		return status;

	TRACE(("could open node at %p\n", node));

	Descriptor *descriptor = new(nothrow) Descriptor(node, cookie);
	if (descriptor == NULL)
		return B_NO_MEMORY;

	sDescriptors[fd] = descriptor;

	return fd;
}


int
dup(int fd)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	descriptor->Acquire();
	RETURN_AND_SET_ERRNO(fd);
}


ssize_t
read_pos(int fd, off_t offset, void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	RETURN_AND_SET_ERRNO(descriptor->ReadAt(offset, buffer, bufferSize));
}


ssize_t
read(int fd, void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	RETURN_AND_SET_ERRNO(descriptor->Read(buffer, bufferSize));
}


ssize_t
write_pos(int fd, off_t offset, const void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	RETURN_AND_SET_ERRNO(descriptor->WriteAt(offset, buffer, bufferSize));
}


ssize_t
write(int fd, const void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	RETURN_AND_SET_ERRNO(descriptor->Write(buffer, bufferSize));
}


ssize_t
writev(int fd, const struct iovec* vecs, size_t count)
{
	size_t totalWritten = 0;

	for (size_t i = 0; i < count; i++) {
		ssize_t written = write(fd, vecs[i].iov_base, vecs[i].iov_len);
		if (written < 0)
			return totalWritten == 0 ? written : totalWritten;

		totalWritten += written;

		if ((size_t)written != vecs[i].iov_len)
			break;
	}

	return totalWritten;
}


int
open(const char *name, int mode, ...)
{
	mode_t permissions = 0;
	if ((mode & O_CREAT) != 0) {
        va_list args;
        va_start(args, mode);
        permissions = va_arg(args, int) /*& ~__gUmask*/;
            // adapt the permissions as required by POSIX
        va_end(args);
	}

	// we always start at the top (there is no notion of a current directory (yet?))
	RETURN_AND_SET_ERRNO(open_from(gRoot, name, mode, permissions));
}


int
open_from(Directory *directory, const char *name, int mode, mode_t permissions)
{
	if (name[0] == '/') {
		// ignore the directory and start from root if we are asked to do that
		directory = gRoot;
		name++;
	}

	char path[B_PATH_NAME_LENGTH];
	if (strlcpy(path, name, sizeof(path)) >= sizeof(path))
		return B_NAME_TOO_LONG;

	Node *node;
	status_t error = get_node_for_path(directory, path, &node);
	if (error != B_OK) {
		if (error != B_ENTRY_NOT_FOUND)
			return error;

		if ((mode & O_CREAT) == 0)
			return B_ENTRY_NOT_FOUND;

		// try to resolve the parent directory
		strlcpy(path, name, sizeof(path));
		if (char* lastSlash = strrchr(path, '/')) {
			if (lastSlash[1] == '\0')
				return B_ENTRY_NOT_FOUND;

			lastSlash = '\0';
			name = lastSlash + 1;

			// resolve the directory
			if (get_node_for_path(directory, path, &node) != B_OK)
				return B_ENTRY_NOT_FOUND;

			if (node->Type() != S_IFDIR) {
				node->Release();
				return B_NOT_A_DIRECTORY;
			}

			directory = static_cast<Directory*>(node);
		} else
			directory->Acquire();

		// create the file
		error = directory->CreateFile(name, permissions, &node);
		directory->Release();

		if (error != B_OK)
			return error;
	} else if ((mode & O_EXCL) != 0) {
		node->Release();
		return B_FILE_EXISTS;
	}

	int fd = open_node(node, mode);

	node->Release();
	return fd;
}


/** Since we don't have directory functions yet, this
 *	function is needed to get the contents of a directory.
 *	It should be removed once readdir() & co. are in place.
 */

Node *
get_node_from(int fd)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return NULL;

	return descriptor->GetNode();
}


int
close(int fd)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	status_t status = descriptor->Release();
	if (!descriptor->RefCount())
		free_descriptor(fd);

	RETURN_AND_SET_ERRNO(status);
}


// ToDo: remove this kludge when possible
int
#if defined(fstat) && !defined(main)
_fstat(int fd, struct stat *stat, size_t /*statSize*/)
#else
fstat(int fd, struct stat *stat)
#endif
{
	if (stat == NULL)
		RETURN_AND_SET_ERRNO(B_BAD_VALUE);

	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		RETURN_AND_SET_ERRNO(B_FILE_ERROR);

	RETURN_AND_SET_ERRNO(descriptor->Stat(*stat));
}
