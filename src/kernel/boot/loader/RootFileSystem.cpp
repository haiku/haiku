/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "RootFileSystem.h"

#include <OS.h>
#include <util/kernel_cpp.h>

#include <string.h>
#include <fcntl.h>


struct file_system_entry {
	list_link	link;
	Directory	*root;
};

struct dir_cookie {
	file_system_entry *current;
};


RootFileSystem::RootFileSystem()
{
	list_init(&fList);
}


RootFileSystem::~RootFileSystem()
{
	file_system_entry *entry = NULL;

	while ((entry = (file_system_entry *)list_remove_head_item(&fList)) != NULL) {
		entry->root->Release();
		delete entry;
	}
}


status_t 
RootFileSystem::Open(void **_cookie, int mode)
{
	dir_cookie *cookie = new dir_cookie();
	if (cookie == NULL)
		return B_NO_MEMORY;
	
	cookie->current = NULL;
	*_cookie = cookie;

	return B_OK;
}


status_t 
RootFileSystem::Close(void *cookie)
{
	delete (dir_cookie *)cookie;
	return B_OK;
}


Node *
RootFileSystem::Lookup(const char *name, bool /*traverseLinks*/)
{
	file_system_entry *entry = NULL;

	while ((entry = (file_system_entry *)list_get_next_item(&fList, entry)) != NULL) {
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
	dir_cookie *cookie = (dir_cookie *)_cookie;
	file_system_entry *entry;

	entry = cookie->current = (file_system_entry *)list_get_next_item(&fList, cookie->current);
	if (entry != NULL)
		return entry->root->GetName(name, size);

	return B_ENTRY_NOT_FOUND;
}


status_t 
RootFileSystem::GetNextNode(void *_cookie, Node **_node)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;
	file_system_entry *entry;

	entry = cookie->current = (file_system_entry *)list_get_next_item(&fList, cookie->current);
	if (entry != NULL) {
		*_node = entry->root;
		return B_OK;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t 
RootFileSystem::Rewind(void *_cookie)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;

	cookie->current = NULL;
	return B_OK;	
}


bool 
RootFileSystem::IsEmpty()
{
	return list_is_empty(&fList);
}


status_t 
RootFileSystem::AddNode(Node *node)
{
	// we don't have RTTI
	if (node->Type() != S_IFDIR)
		return B_BAD_TYPE;

	file_system_entry *entry = new file_system_entry();
	if (entry == NULL)
		return B_NO_MEMORY;

	node->Acquire();
	entry->root = (Directory *)node;

	list_add_item(&fList, entry);

	return B_OK;
}

