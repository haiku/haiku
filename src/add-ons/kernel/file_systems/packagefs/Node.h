/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <fs_interface.h>

#include <AutoLocker.h>
#include <Referenceable.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <util/OpenHashTable.h>


class Directory;
class PackageNode;


class Node : public BReferenceable, public DoublyLinkedListLinkImpl<Node> {
public:
								Node(ino_t id);
	virtual						~Node();

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

			ino_t				ID() const		{ return fID; }
			Directory*			Parent() const	{ return fParent; }
			const char*			Name() const	{ return fName; }

			Node*&				NameHashTableNext()
									{ return fNameHashTableNext; }
			Node*&				IDHashTableNext()
									{ return fIDHashTableNext; }

			mode_t				Mode() const	{ return fMode; }
			uid_t				UserID() const	{ return fUserID; }
			gid_t				GroupID() const	{ return fGroupID; }

	virtual	status_t			Init(Directory* parent, const char* name);

	virtual	status_t			AddPackageNode(PackageNode* packageNode) = 0;

protected:
			rw_lock				fLock;
			ino_t				fID;
			Directory*			fParent;
			char*				fName;
			Node*				fNameHashTableNext;
			Node*				fIDHashTableNext;
			mode_t				fMode;
			uid_t				fUserID;
			gid_t				fGroupID;
};


bool
Node::ReadLock()
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Node::ReadUnlock()
{
	rw_lock_read_unlock(&fLock);
}


bool
Node::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
Node::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


struct NodeNameHashDefinition {
	typedef const char*		KeyType;
	typedef	Node			ValueType;

	size_t HashKey(const char* key) const
	{
		return hash_hash_string(key);
	}

	size_t Hash(const Node* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const char* key, const Node* value) const
	{
		return strcmp(value->Name(), key) == 0;
	}

	Node*& GetLink(Node* value) const
	{
		return value->NameHashTableNext();
	}
};


struct NodeIDHashDefinition {
	typedef ino_t	KeyType;
	typedef	Node	ValueType;

	size_t HashKey(ino_t key) const
	{
		return (uint64)(key >> 32) ^ (uint32)key;
	}

	size_t Hash(const Node* value) const
	{
		return HashKey(value->ID());
	}

	bool Compare(ino_t key, const Node* value) const
	{
		return value->ID() == key;
	}

	Node*& GetLink(Node* value) const
	{
		return value->IDHashTableNext();
	}
};

typedef DoublyLinkedList<Node> NodeList;

typedef BOpenHashTable<NodeNameHashDefinition> NodeNameHashTable;
typedef BOpenHashTable<NodeIDHashDefinition> NodeIDHashTable;

typedef AutoLocker<Node, AutoLockerReadLocking<Node> > NodeReadLocker;
typedef AutoLocker<Node, AutoLockerWriteLocking<Node> > NodeWriteLocker;


#endif	// NODE_H
