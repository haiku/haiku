/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGES_DIRECTORY_H
#define PACKAGES_DIRECTORY_H


#include <sys/stat.h>

#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "NodeRef.h"
#include "String.h"


class PackagesDirectory : public BReferenceable,
	public DoublyLinkedListLinkImpl<PackagesDirectory> {
public:
								PackagesDirectory();
								~PackagesDirectory();

			const String&		StateName() const
									{ return fStateName; }
									// empty for the latest state
			const char*			Path() const
									{ return fPath; }
			int					DirectoryFD() const
									{ return fDirFD; }
			const node_ref&		NodeRef() const
									{ return fNodeRef; }
			dev_t				DeviceID() const
									{ return fNodeRef.device; }
			ino_t				NodeID() const
									{ return fNodeRef.node; }

			status_t			Init(const char* path, dev_t mountPointDeviceID,
									ino_t mountPointNodeID, struct stat& _st);
			status_t			InitOldState(dev_t adminDirDeviceID,
									ino_t adminDirNodeID,
									const char* stateName);

	static	bool				IsNewer(const PackagesDirectory* a,
									const PackagesDirectory* b);

			PackagesDirectory*&	HashTableNext()
									{ return fHashNext; }

private:
			status_t			_Init(struct vnode* vnode, struct stat& _st);

private:
			String				fStateName;
			char*				fPath;
			int					fDirFD;
			node_ref			fNodeRef;
			PackagesDirectory*	fHashNext;
};


struct PackagesDirectoryHashDefinition {
	typedef const node_ref&		KeyType;
	typedef	PackagesDirectory	ValueType;

	size_t HashKey(const node_ref& key) const
	{
		return (size_t)key.device ^ (size_t)key.node;
	}

	size_t Hash(const PackagesDirectory* value) const
	{
		return HashKey(value->NodeRef());
	}

	bool Compare(const node_ref& key, const PackagesDirectory* value) const
	{
		return key == value->NodeRef();
	}

	PackagesDirectory*& GetLink(PackagesDirectory* value) const
	{
		return value->HashTableNext();
	}
};


typedef BOpenHashTable<PackagesDirectoryHashDefinition>
	PackagesDirectoryHashTable;
typedef DoublyLinkedList<PackagesDirectory> PackagesDirectoryList;


#endif	// PACKAGES_DIRECTORY_H
