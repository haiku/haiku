/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Partition.h"
#include "RootFileSystem.h"

#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>


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

	private:
		Node	*fNode;
		void	*fCookie;
		off_t	fOffset;
		int32	fRefCount;
};

#define MAX_VFS_DESCRIPTORS 32

list gBootDevices;
list gPartitions;
Descriptor *gDescriptors[MAX_VFS_DESCRIPTORS];
Directory *gRoot;


Node::Node()
	:
	fRefCount(0)
{
	fLink.next = fLink.prev = NULL;
}


Node::~Node()
{
}


status_t
Node::Open(void **_cookie, int mode)
{
	return Acquire();
}


status_t
Node::Close(void *cookie)
{
	return Release();
}


status_t 
Node::GetName(char *nameBuffer, size_t bufferSize) const
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


status_t 
Node::Acquire()
{
	fRefCount++;
	return B_OK;
}

status_t 
Node::Release()
{
	if (--fRefCount == 0)
		delete this;

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
Directory::AddNode(Node */*node*/)
{
	// just don't do anything
	return B_ERROR;
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


static Descriptor *
get_descriptor(int fd)
{
	if (fd >= MAX_VFS_DESCRIPTORS)
		return NULL;

	return gDescriptors[fd];
}


static void
free_descriptor(int fd)
{
	if (fd >= MAX_VFS_DESCRIPTORS)
		return;

	delete gDescriptors[fd];
	gDescriptors[fd] = NULL;
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
		if (gDescriptors[fd] == NULL)
			break;
	}
	if (fd == MAX_VFS_DESCRIPTORS)
		return B_ERROR;

	printf("got descriptor %d\n", fd);

	// we got a free descriptor entry, now try to open the node
	
	void *cookie;
	status_t status = node->Open(&cookie, mode);
	if (status < B_OK)
		return status;

	printf("could open node at %p\n", node);

	Descriptor *descriptor = new Descriptor(node, cookie);
	if (descriptor == NULL)
		return B_NO_MEMORY;

	gDescriptors[fd] = descriptor;

	return fd;
}


status_t
vfs_init(stage2_args *args)
{
	list_init(&gBootDevices);
	list_init(&gPartitions);

	status_t status = platform_get_boot_devices(args, &gBootDevices);
	if (status < B_OK)
		return status;

	return B_OK;
}


status_t
mount_boot_file_systems()
{
	list_init_etc(&gPartitions, Partition::LinkOffset());

	gRoot = new RootFileSystem();
	if (gRoot == NULL)
		return B_NO_MEMORY;

	Node *device = NULL;
	while ((device = (Node *)list_get_next_item(&gBootDevices, (void *)device)) != NULL) {
		int fd = open_node(device, O_RDONLY);
		if (fd < B_OK)
			continue;

		puts("add partitions");
		add_partitions_for(fd);

		close(fd);
	}

	if (list_is_empty(&gPartitions))
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


/*
void *
mount(void *from, const char *name)
{
	ops *op = (ops *)handle;

	if (op->mount != NULL)
		return op->mount(from, name);

	return NULL;
}
*/


int
dup(int fd)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	descriptor->Acquire();
	return fd;
}


ssize_t
read_pos(int fd, off_t offset, void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	return descriptor->ReadAt(offset, buffer, bufferSize);
}


ssize_t
read(int fd, void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	return descriptor->Read(buffer, bufferSize);
}


ssize_t
write_pos(int fd, off_t offset, const void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	return descriptor->WriteAt(offset, buffer, bufferSize);
}


ssize_t
write(int fd, const void *buffer, size_t bufferSize)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	return descriptor->Write(buffer, bufferSize);
}


int
open(const char *name, int mode)
{
	return B_ERROR;
}


int
close(int fd)
{
	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = descriptor->Release();
	if (!descriptor->RefCount())
		free_descriptor(fd);

	return status;
}


int
fstat(int fd, struct stat *stat)
{
	if (stat == NULL)
		return B_BAD_VALUE;

	Descriptor *descriptor = get_descriptor(fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	return descriptor->Stat(*stat);
}
