/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DIRECTORY_H
#define PACKAGE_DIRECTORY_H


#include <util/DoublyLinkedList.h>

#include "PackageNode.h"


class PackageDirectory : public PackageNode,
	public DoublyLinkedListLinkImpl<PackageDirectory> {
public:
	static	void*				operator new(size_t size);
	static	void				operator delete(void* block);

								PackageDirectory(Package* package, mode_t mode);
	virtual						~PackageDirectory();

			void				AddChild(PackageNode* node);
			void				RemoveChild(PackageNode* node);

	inline	PackageNode*		FirstChild() const;
	inline	PackageNode*		NextChild(PackageNode* node) const;

			const PackageNodeList& Children() const
									{ return fChildren; }

			bool				HasPrecedenceOver(const PackageDirectory* other)
									const;

private:
			PackageNodeList		fChildren;
};


PackageNode*
PackageDirectory::FirstChild() const
{
	return fChildren.First();
}


PackageNode*
PackageDirectory::NextChild(PackageNode* node) const
{
	return fChildren.GetNext(node);
}


typedef DoublyLinkedList<PackageDirectory> PackageDirectoryList;


#endif	// PACKAGE_DIRECTORY_H
