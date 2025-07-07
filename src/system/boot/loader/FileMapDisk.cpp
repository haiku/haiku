/*
 * Copyright 2005, ?.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

// Include necessary headers for the FileMapDisk class
#include <boot/FileMapDisk.h>
#include <boot_item.h>
#include <new>
#include <endian.h>
#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <SupportDefs.h>

// Uncomment to enable tracing for debugging purposes
//#define TRACE_FILEMAPDISK

// Define a macro for tracing, which can be enabled or disabled
#ifdef TRACE_FILEMAPDISK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// Use the nothrow version of new to avoid exceptions on memory allocation failures
using std::nothrow;

// Constructor for the FileMapDisk class
FileMapDisk::FileMapDisk()
	:
	fNode(NULL) // Initialize the node pointer to NULL
{
}

// Destructor for the FileMapDisk class
FileMapDisk::~FileMapDisk()
{
}

// Initialize the FileMapDisk with a given node
status_t
FileMapDisk::Init(Node *node/*, Partition *partition, FileMap *map, off_t imageSize*/)
{
	TRACE(("FileMapDisk::FileMapDisk(%p)\n", node)); // Trace the initialization
	fNode = node; // Assign the provided node to the member variable

	/*
	// Commented out code for socket initialization
	fPartition = partition;
	fMap = map;
	fImageSize = imageSize;
	// Create and bind a UDP socket
	fSocket = new(nothrow) UDPSocket;
	if (!fSocket)
		return B_NO_MEMORY; // Return error if socket creation fails
	status_t error = fSocket->Bind(INADDR_ANY, 6666); // Bind to any address on port 6666
	if (error != B_OK)
		return error; // Return error if binding fails
	*/

	return B_OK; // Return success
}

// Open the file map disk with the given mode
status_t
FileMapDisk::Open(void **_cookie, int mode)
{
	TRACE(("FileMapDisk::Open(, 0x%08x)\n", mode)); // Trace the open operation
	if (fNode == NULL)
		return B_NO_INIT; // Return error if not initialized
	return fNode->Open(_cookie, mode); // Delegate the open operation to the node
}

// Close the file map disk
status_t
FileMapDisk::Close(void *cookie)
{
	TRACE(("FileMapDisk::Close(%p)\n", cookie)); // Trace the close operation
	if (fNode == NULL)
		return B_NO_INIT; // Return error if not initialized
	return fNode->Close(cookie); // Delegate the close operation to the node
}

// Read data from the file map disk at a specific position
ssize_t
FileMapDisk::ReadAt(void *cookie, off_t pos, void *_buffer, size_t bufferSize)
{
	TRACE(("FileMapDisk::ReadAt(%p, %lld, , %ld)\n", cookie, pos, bufferSize)); // Trace the read operation
	if (fNode == NULL)
		return B_NO_INIT; // Return error if not initialized
	return fNode->ReadAt(cookie, pos, _buffer, bufferSize); // Delegate the read operation to the node
}

// Write data to the file map disk at a specific position
ssize_t
FileMapDisk::WriteAt(void */*cookie*/, off_t pos, const void *buffer, size_t bufferSize)
{
	// Not needed in the boot loader, so return permission denied
	return B_PERMISSION_DENIED;
}

// Get the name of the file map disk
status_t
FileMapDisk::GetName(char *nameBuffer, size_t bufferSize) const
{
	const char *prefix = "FileMapDisk:"; // Prefix for the name
	if (nameBuffer == NULL)
		return B_BAD_VALUE; // Return error if buffer is NULL
	strlcpy(nameBuffer, prefix, bufferSize); // Copy the prefix to the buffer
	if (bufferSize > strlen(prefix) && fNode)
		return fNode->GetName(nameBuffer + strlen(prefix), bufferSize - strlen(prefix)); // Get the rest of the name from the node
	return B_OK; // Return success
}

// Get the file map for the disk
status_t
FileMapDisk::GetFileMap(struct file_map_run *runs, int32 *count)
{
	return fNode->GetFileMap(runs, count); // Delegate the operation to the node
}

// Get the size of the file map disk
off_t
FileMapDisk::Size() const
{
	if (fNode == NULL)
		return B_NO_INIT; // Return error if not initialized
	return fNode->Size(); // Delegate the operation to the node
}

// Find any file map disk in the given volume
FileMapDisk *
FileMapDisk::FindAnyFileMapDisk(Directory *volume)
{
	TRACE(("FileMapDisk::FindAnyFileMapDisk(%p)\n", volume)); // Trace the operation
	Node *node;
	status_t error;
	if (volume == NULL)
		return NULL; // Return NULL if volume is NULL

	// Lookup the file map folder in the volume
	// XXX: check lower/mixed case as well
	Node *dirnode;
	Directory *dir;
	dirnode = volume->Lookup(FMAP_FOLDER_NAME, true);
	if (dirnode == NULL || !S_ISDIR(dirnode->Type()))
		return NULL; // Return NULL if the folder is not found or not a directory

	dir = (Directory *)dirnode;
	// Lookup the file map image in the directory
	node = dir->Lookup(FMAP_IMAGE_NAME, true);
	if (node == NULL)
		return NULL; // Return NULL if the image is not found

	// Create a FileMapDisk object
	FileMapDisk *disk = new(nothrow) FileMapDisk;
	if (disk != NULL) {
		error = disk->Init(node); // Initialize the disk with the node
		if (error != B_OK) {
			delete disk; // Delete the disk if initialization fails
			disk = NULL;
		}
	}
	return disk; // Return the disk
}

// Register the file map boot item
status_t
FileMapDisk::RegisterFileMapBootItem()
{
	return B_ERROR; // Return error
#if 0
	// Commented out code for registering the boot item
	struct file_map_boot_item *item;
	item = (struct file_map_boot_item *)malloc(sizeof(struct file_map_boot_item));
	item->num_runs = FMAP_MAX_RUNS;
	status_t err;
	err = GetFileMap(item->runs, &item->num_runs);
	if (err < B_OK)
		return err;
	// err = add_boot_item("file_map_disk", item, sizeof(struct file_map_boot_item));
	err = B_ERROR;
	return err;
#endif
}

