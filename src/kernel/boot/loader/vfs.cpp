/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <util/kernel_cpp.h>

#include <unistd.h>
#include <fcntl.h>


#if 0
struct vfs_ops {
	int (*open)(void **_cookie);
	int (*close)(void *cookie);
	int (*read)(void *cookie, off_t pos, void *buffer, size_t bytes);
	int (*write)(void *cookie, off_t pos, const void *buffer, size_t bytes);
};

struct vfs_node {
	struct vfs_node	*next;
	off_t			offset;
	off_t			size;
	int32			ref_count;
	struct vfs_ops	*ops;
};

struct vfs_descriptor {
	struct vfs_node	*node;
	struct vfs_ops	*ops;
	void			*cookie;
	off_t			offset;
	int32			ref_count;
};
#endif

#define MAX_VFS_DESCRIPTORS 32

list gBootDevices;
list gPartitions;
Descriptor *gDescriptors[MAX_VFS_DESCRIPTORS];


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
	return B_OK;
}


status_t
Node::Close(void *cookie)
{
	return B_OK;
}


#pragma mark -


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


#pragma mark -


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
Descriptor::Read(off_t pos, void *buffer, size_t bufferSize)
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
Descriptor::Write(off_t pos, const void *buffer, size_t bufferSize)
{
	return fNode->WriteAt(fCookie, pos, buffer, bufferSize);
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
	if (--fRefCount == 1) {
		status_t status = fNode->Close(fCookie);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


#pragma mark -


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
		panic("Could not find any partitions on the boot devices");

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

	return descriptor->Read(offset, buffer, bufferSize);
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

	return descriptor->Write(offset, buffer, bufferSize);
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

