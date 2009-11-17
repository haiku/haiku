/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"

#include "DebugSupport.h"
#include "PackageDirectory.h"


Directory::Directory(ino_t id)
	:
	Node(id)
{
	fMode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
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
Directory::Init(Directory* parent, const char* name)
{
	status_t error = fChildTable.Init();
	if (error != B_OK)
		return error;

	return Node::Init(parent, name);
}


status_t
Directory::AddPackageNode(PackageNode* packageNode)
{
	if (!S_ISDIR(packageNode->Mode()))
		return B_BAD_VALUE;

// TODO:...

	return B_OK;
}


void
Directory::AddChild(Node* node)
{
	fChildTable.Insert(node);
	fChildList.Add(node);
}


void
Directory::RemoveChild(Node* node)
{
	Node* nextNode = fChildList.GetNext(node);

	fChildTable.Remove(node);
	fChildList.Remove(node);

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
