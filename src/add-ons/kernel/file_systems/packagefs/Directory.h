/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"
#include "PackageDirectory.h"



struct DirectoryIterator : DoublyLinkedListLinkImpl<DirectoryIterator> {
	Node*	node;

	DirectoryIterator()
		:
		node(NULL)
	{
	}
};

typedef DoublyLinkedList<DirectoryIterator> DirectoryIteratorList;


class Directory : public Node {
public:
								Directory(ino_t id);
	virtual						~Directory();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;

	virtual	status_t			AddPackageNode(PackageNode* packageNode);

			void				AddChild(Node* node);
			void				RemoveChild(Node* node);
			Node*				FindChild(const char* name);

	inline	Node*				FirstChild() const;
	inline	Node*				NextChild(Node* node) const;

			void				AddDirectoryIterator(
									DirectoryIterator* iterator);
			void				RemoveDirectoryIterator(
									DirectoryIterator* iterator);

private:
			NodeNameHashTable	fChildTable;
			NodeList			fChildList;
			PackageDirectoryList fPackageDirectories;
			DirectoryIteratorList fIterators;
};


Node*
Directory::FirstChild() const
{
	return fChildList.First();
}


Node*
Directory::NextChild(Node* node) const
{
	return fChildList.GetNext(node);
}


#endif	// DIRECTORY_H
