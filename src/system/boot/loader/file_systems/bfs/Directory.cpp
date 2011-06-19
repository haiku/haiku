/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"
#include "File.h"
#include "Link.h"

#include <StorageDefs.h>
#include <KernelExport.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>


// temp. private VFS API
extern Node *get_node_from(int fd);


namespace BFS {


Directory::Directory(Volume &volume, block_run run)
	:
	fStream(volume, run),
	fTree(&fStream)
{
}


Directory::Directory(Volume &volume, off_t id)
	:
	fStream(volume, id),
	fTree(&fStream)
{
}


Directory::Directory(const Stream &stream)
	:
	fStream(stream),
	fTree(&fStream)
{
}


Directory::~Directory()
{
}


status_t
Directory::InitCheck()
{
	return fStream.InitCheck();
}


status_t
Directory::Open(void **_cookie, int mode)
{
	_inherited::Open(_cookie, mode);

	*_cookie = (void *)new(nothrow) TreeIterator(&fTree);
	if (*_cookie == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
Directory::Close(void *cookie)
{
	_inherited::Close(cookie);

	delete (TreeIterator *)cookie;
	return B_OK;
}


Node*
Directory::LookupDontTraverse(const char* name)
{
	off_t id;
	if (fTree.Find((uint8 *)name, strlen(name), &id) < B_OK)
		return NULL;

	return Stream::NodeFactory(fStream.GetVolume(), id);
}


status_t
Directory::GetNextEntry(void *cookie, char *name, size_t size)
{
	TreeIterator *iterator = (TreeIterator *)cookie;
	uint16 length;
	off_t id;

	return iterator->GetNextEntry(name, &length, size, &id);
}


status_t
Directory::GetNextNode(void *cookie, Node **_node)
{
	TreeIterator *iterator = (TreeIterator *)cookie;
	char name[B_FILE_NAME_LENGTH];
	uint16 length;
	off_t id;

	status_t status = iterator->GetNextEntry(name, &length, sizeof(name), &id);
	if (status != B_OK)
		return status;

	*_node = Stream::NodeFactory(fStream.GetVolume(), id);
	if (*_node == NULL)
		return B_ERROR;

	return B_OK;
}


status_t
Directory::Rewind(void *cookie)
{
	TreeIterator *iterator = (TreeIterator *)cookie;

	return iterator->Rewind();
}


bool
Directory::IsEmpty()
{
	TreeIterator iterator(&fTree);

	// index and attribute directories are really empty when they are
	// empty - directories for standard files always contain ".", and
	// "..", so we need to ignore those two

	uint32 count = 0;
	char name[BPLUSTREE_MAX_KEY_LENGTH];
	uint16 length;
	off_t id;
	while (iterator.GetNextEntry(name, &length, B_FILE_NAME_LENGTH, &id)
			== B_OK) {
		if (fStream.Mode() & (S_ATTR_DIR | S_INDEX_DIR))
			return false;

		if (++count > 2 || (strcmp(".", name) && strcmp("..", name)))
			return false;
	}
	return true;
}


status_t
Directory::GetName(char *name, size_t size) const
{
	if (fStream.inode_num == fStream.GetVolume().Root()) {
		strlcpy(name, fStream.GetVolume().SuperBlock().name, size);
		return B_OK;
	}

	return fStream.GetName(name, size);
}


ino_t
Directory::Inode() const
{
	return fStream.ID();
}


}	// namespace BFS
