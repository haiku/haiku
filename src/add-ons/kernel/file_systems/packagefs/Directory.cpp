/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"

#include "DebugSupport.h"
#include "UnpackingAttributeCookie.h"
#include "UnpackingAttributeDirectoryCookie.h"
#include "Utils.h"


Directory::Directory(ino_t id)
	:
	Node(id)
{
}


Directory::~Directory()
{
	Node* child = fChildTable.Clear(true);
	while (child != NULL) {
		Node* next = child->NameHashTableNext();
		child->ReleaseReference();
		child = next;
	}
}


status_t
Directory::Init(Directory* parent, const char* name, uint32 flags)
{
	status_t error = Node::Init(parent, name, flags);
	if (error != B_OK)
		return error;

	return fChildTable.Init();
}


status_t
Directory::Read(off_t offset, void* buffer, size_t* bufferSize)
{
	return B_IS_A_DIRECTORY;
}


status_t
Directory::Read(io_request* request)
{
	return B_IS_A_DIRECTORY;
}


status_t
Directory::ReadSymlink(void* buffer, size_t* bufferSize)
{
	return B_IS_A_DIRECTORY;
}


void
Directory::AddChild(Node* node)
{
	fChildTable.Insert(node);
	fChildList.Add(node);
	node->AcquireReference();
}


void
Directory::RemoveChild(Node* node)
{
	Node* nextNode = fChildList.GetNext(node);

	fChildTable.Remove(node);
	fChildList.Remove(node);
	node->ReleaseReference();

	// adjust directory iterators pointing to the removed child
	for (DirectoryIteratorList::Iterator it = fIterators.GetIterator();
			DirectoryIterator* iterator = it.Next();) {
		if (iterator->node == node)
			iterator->node = nextNode;
	}
}


Node*
Directory::FindChild(const char* name)
{
	return fChildTable.Lookup(name);
}


void
Directory::AddDirectoryIterator(DirectoryIterator* iterator)
{
	fIterators.Add(iterator);
}


void
Directory::RemoveDirectoryIterator(DirectoryIterator* iterator)
{
	fIterators.Remove(iterator);
}
