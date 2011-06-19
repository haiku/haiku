/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RootFileSystem.h"

#include <OS.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <fcntl.h>


RootFileSystem::RootFileSystem()
{
}


RootFileSystem::~RootFileSystem()
{
	struct entry *entry = NULL;

	while ((entry = fList.RemoveHead()) != NULL) {
		entry->root->Release();
		delete entry;
	}
}


status_t
RootFileSystem::Open(void **_cookie, int mode)
{
	EntryIterator *iterator = new (std::nothrow) EntryIterator(&fList);
	if (iterator == NULL)
		return B_NO_MEMORY;

	*_cookie = iterator;

	return B_OK;
}


status_t
RootFileSystem::Close(void *cookie)
{
	delete (EntryIterator *)cookie;
	return B_OK;
}


Node*
RootFileSystem::LookupDontTraverse(const char* name)
{
	EntryIterator iterator = fLinks.GetIterator();
	struct entry *entry;

	// first check the links

	while ((entry = iterator.Next()) != NULL) {
		if (!strcmp(name, entry->name)) {
			entry->root->Acquire();
			return entry->root;
		}
	}

	// then all mounted file systems

	iterator = fList.GetIterator();

	while ((entry = iterator.Next()) != NULL) {
		char entryName[B_OS_NAME_LENGTH];
		if (entry->root->GetName(entryName, sizeof(entryName)) != B_OK)
			continue;

		if (!strcmp(entryName, name)) {
			entry->root->Acquire();
			return entry->root;
		}
	}

	return NULL;
}


status_t
RootFileSystem::GetNextEntry(void *_cookie, char *name, size_t size)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;
	struct entry *entry;

	entry = iterator->Next();
	if (entry != NULL)
		return entry->root->GetName(name, size);

	return B_ENTRY_NOT_FOUND;
}


status_t
RootFileSystem::GetNextNode(void *_cookie, Node **_node)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;
	struct entry *entry;

	entry = iterator->Next();
	if (entry != NULL) {
		*_node = entry->root;
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
RootFileSystem::Rewind(void *_cookie)
{
	EntryIterator *iterator = (EntryIterator *)_cookie;

	iterator->Rewind();
	return B_OK;
}


bool
RootFileSystem::IsEmpty()
{
	return fList.IsEmpty();
}


status_t
RootFileSystem::AddVolume(Directory *volume, Partition *partition)
{
	struct entry *entry = new (std::nothrow) RootFileSystem::entry();
	if (entry == NULL)
		return B_NO_MEMORY;

	volume->Acquire();
	entry->name = NULL;
	entry->root = volume;
	entry->partition = partition;

	fList.Add(entry);

	return B_OK;
}


status_t
RootFileSystem::AddLink(const char *name, Directory *target)
{
	struct entry *entry = new (std::nothrow) RootFileSystem::entry();
	if (entry == NULL)
		return B_NO_MEMORY;

	target->Acquire();
	entry->name = name;
	entry->root = target;

	fLinks.Add(entry);

	return B_OK;
}


status_t
RootFileSystem::GetPartitionFor(Directory *volume, Partition **_partition)
{
	EntryIterator iterator = fList.GetIterator();
	struct entry *entry;

	while ((entry = iterator.Next()) != NULL) {
		if (entry->root == volume) {
			*_partition = entry->partition;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}

