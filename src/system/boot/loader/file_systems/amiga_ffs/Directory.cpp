/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Directory.h"
#include "Volume.h"
#include "File.h"

#include <StorageDefs.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>


namespace FFS {

Directory::Directory(Volume &volume, int32 block)
	:
	fVolume(volume)
{
	void *data = malloc(volume.BlockSize());
	if (data == NULL)
		return;

	if (read_pos(volume.Device(), block * volume.BlockSize(), data, volume.BlockSize()) == volume.BlockSize())
		fNode.SetTo(data, volume.BlockSize());
}


Directory::Directory(Volume &volume, RootBlock &root)
	:
	fVolume(volume)
{
	fNode.SetTo(root.BlockData(), root.BlockSize());
}


Directory::~Directory()
{
	free(fNode.BlockData());
}


status_t
Directory::InitCheck()
{
	return fNode.ValidateCheckSum();
}


status_t
Directory::Open(void **_cookie, int mode)
{
	_inherited::Open(_cookie, mode);

	HashIterator *iterator = new(nothrow) HashIterator(fVolume.Device(), fNode);
	if (iterator == NULL)
		return B_NO_MEMORY;

	if (iterator->InitCheck() != B_OK) {
		delete iterator;
		return B_NO_MEMORY;
	}

	*_cookie = (void *)iterator;
	return B_OK;
}


status_t
Directory::Close(void *cookie)
{
	_inherited::Close(cookie);

	delete (HashIterator *)cookie;
	return B_OK;
}


Node*
Directory::LookupDontTraverse(const char* name)
{
	if (!strcmp(name, ".")) {
		Acquire();
		return this;
	}

	HashIterator iterator(fVolume.Device(), fNode);
	if (iterator.InitCheck() != B_OK)
		return NULL;

	iterator.Goto(fNode.HashIndexFor(fVolume.Type(), name));

	NodeBlock *node;
	int32 block;
	while ((node = iterator.GetNext(block)) != NULL) {
		char fileName[FFS_NAME_LENGTH];
		if (node->GetName(fileName, sizeof(fileName)) == B_OK
			&& !strcmp(name, fileName)) {
			if (node->IsFile())
				return new(nothrow) File(fVolume, block);
			if (node->IsDirectory())
				return new(nothrow) Directory(fVolume, block);

			return NULL;
		}
	}
	return NULL;
}


status_t
Directory::GetNextEntry(void *cookie, char *name, size_t size)
{
	HashIterator *iterator = (HashIterator *)cookie;
	int32 block;

	NodeBlock *node = iterator->GetNext(block);
	if (node == NULL)
		return B_ENTRY_NOT_FOUND;

	return node->GetName(name, size);
}


status_t
Directory::GetNextNode(void *cookie, Node **_node)
{
	return B_ERROR;
}


status_t
Directory::Rewind(void *cookie)
{
	HashIterator *iterator = (HashIterator *)cookie;
	iterator->Rewind();

	return B_OK;
}


bool
Directory::IsEmpty()
{
	int32 index;
	return fNode.FirstHashValue(index) == -1;
}


status_t
Directory::GetName(char *name, size_t size) const
{
	return fNode.GetName(name, size);
}


ino_t
Directory::Inode() const
{
	return fNode.HeaderKey();
}

}	// namespace FFS
