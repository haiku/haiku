/*
 * Copyright 2005, ?.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <boot/FileMapDisk.h>
#include <boot_item.h>

#include <new>

#include <endian.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>
#include <SupportDefs.h>


//#define TRACE_FILEMAPDISK
#ifdef TRACE_FILEMAPDISK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


using std::nothrow;


// constructor
FileMapDisk::FileMapDisk()
	:
	fNode(NULL)
{
}

// destructor
FileMapDisk::~FileMapDisk()
{
}

// Init
status_t
FileMapDisk::Init(Node *node/*, Partition *partition, FileMap *map, off_t imageSize*/)
{
	TRACE(("FileMapDisk::FileMapDisk(%p)\n", node));
	fNode = node;
	/*
	fPartition = partition;
	fMap = map;
	fImageSize = imageSize;

	// create and bind socket
	fSocket = new(nothrow) UDPSocket;
	if (!fSocket)
		return B_NO_MEMORY;

	status_t error = fSocket->Bind(INADDR_ANY, 6666);
	if (error != B_OK)
		return error;
	*/

	return B_OK;
}


status_t
FileMapDisk::Open(void **_cookie, int mode)
{
	TRACE(("FileMapDisk::Open(, 0x%08x)\n", mode));
	if (fNode == NULL)
		return B_NO_INIT;

	return fNode->Open(_cookie, mode);
}


status_t
FileMapDisk::Close(void *cookie)
{
	TRACE(("FileMapDisk::Close(%p)\n", cookie));
	if (fNode == NULL)
		return B_NO_INIT;

	return fNode->Close(cookie);
}


// ReadAt
ssize_t
FileMapDisk::ReadAt(void *cookie, off_t pos, void *_buffer,
	size_t bufferSize)
{
	TRACE(("FileMapDisk::ReadAt(%p, %lld, , %ld)\n", cookie, pos, bufferSize));
	if (fNode == NULL)
		return B_NO_INIT;

	return fNode->ReadAt(cookie, pos, _buffer, bufferSize);
}


// WriteAt
ssize_t
FileMapDisk::WriteAt(void */*cookie*/, off_t pos, const void *buffer,
	size_t bufferSize)
{
	// Not needed in the boot loader.
	return B_PERMISSION_DENIED;
}

// GetName
status_t
FileMapDisk::GetName(char *nameBuffer, size_t bufferSize) const
{
	const char *prefix = "FileMapDisk:";
	if (nameBuffer == NULL)
		return B_BAD_VALUE;

	strlcpy(nameBuffer, prefix, bufferSize);
	if (bufferSize > strlen(prefix) && fNode)
		return fNode->GetName(nameBuffer + strlen(prefix),
			bufferSize - strlen(prefix));

	return B_OK;
}


status_t
FileMapDisk::GetFileMap(struct file_map_run *runs, int32 *count)
{
	return fNode->GetFileMap(runs, count);
}


off_t
FileMapDisk::Size() const
{
	if (fNode == NULL)
		return B_NO_INIT;
	return fNode->Size();
}


FileMapDisk *
FileMapDisk::FindAnyFileMapDisk(Directory *volume)
{
	TRACE(("FileMapDisk::FindAnyFileMapDisk(%p)\n", volume));
	Node *node;
	status_t error;

	if (volume == NULL)
		return NULL;

	//XXX: check lower/mixed case as well
	Node *dirnode;
	Directory *dir;
	dirnode = volume->Lookup(FMAP_FOLDER_NAME, true);
	if (dirnode == NULL || !S_ISDIR(dirnode->Type()))
		return NULL;
	dir = (Directory *)dirnode;
	node = dir->Lookup(FMAP_IMAGE_NAME, true);
	if (node == NULL)
		return NULL;

	// create a FileMapDisk object
	FileMapDisk *disk = new(nothrow) FileMapDisk;
	if (disk != NULL) {
		error = disk->Init(node);
		if (error != B_OK) {
			delete disk;
			disk = NULL;
		}
	}

	return disk;
}


status_t
FileMapDisk::RegisterFileMapBootItem()
{
	return B_ERROR;
#if 0
	struct file_map_boot_item *item;
	item = (struct file_map_boot_item *)malloc(sizeof(struct file_map_boot_item));
	item->num_runs = FMAP_MAX_RUNS;
	status_t err;
	err = GetFileMap(item->runs, &item->num_runs);
	if (err < B_OK)
		return err;
//	err = add_boot_item("file_map_disk", item, sizeof(struct file_map_boot_item));
	err = B_ERROR;
	return err;
#endif
}
