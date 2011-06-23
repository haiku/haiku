/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H


#include "Node.h"
#include "PackageDirectory.h"
#include "UnpackingNode.h"


struct DirectoryIterator : DoublyLinkedListLinkImpl<DirectoryIterator> {
	Node*	node;

	DirectoryIterator()
		:
		node(NULL)
	{
	}
};

typedef DoublyLinkedList<DirectoryIterator> DirectoryIteratorList;


class Directory : public Node, public UnpackingNode {
public:
								Directory(ino_t id);
	virtual						~Directory();

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	status_t			VFSInit(dev_t deviceID);
	virtual	void				VFSUninit();

	virtual	mode_t				Mode() const;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const;
	virtual	off_t				FileSize() const;

	virtual	Node*				GetNode();

	virtual	status_t			AddPackageNode(PackageNode* packageNode);
	virtual	void				RemovePackageNode(PackageNode* packageNode);

	virtual	PackageNode*		GetPackageNode();

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize);

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

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


class RootDirectory : public Directory {
public:
								RootDirectory(ino_t id,
									const timespec& modifiedTime);

	virtual	timespec			ModifiedTime() const;

private:
			timespec			fModifiedTime;
};


#endif	// DIRECTORY_H
