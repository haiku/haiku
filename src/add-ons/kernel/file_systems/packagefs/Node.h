/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
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


class AttributeCookie;
class AttributeDirectoryCookie;
class AttributeIndexer;
class Directory;
class PackageNode;


// node flags
enum {
	NODE_FLAG_KEEP_NAME		= 0x01,
		// Init(): Take over ownership of the given name (i.e. free in
		// destructor).
	NODE_FLAG_CONST_NAME	= 0x02,
		// Init(): The given name is a constant that won't go away during the
		// lifetime of the object. No need to copy.
	NODE_FLAG_OWNS_NAME		= NODE_FLAG_KEEP_NAME
};


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

	virtual	status_t			Init(Directory* parent, const char* name,
									uint32 flags);
									// If specified to keep the name, it does
									// so also on error.

	virtual	status_t			VFSInit(dev_t deviceID);
	virtual	void				VFSUninit();

			void				SetID(ino_t id);
			void				SetParent(Directory* parent);

	virtual	mode_t				Mode() const = 0;
	virtual	uid_t				UserID() const;
	virtual	gid_t				GroupID() const;
	virtual	timespec			ModifiedTime() const = 0;
	virtual	off_t				FileSize() const = 0;

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize) = 0;
	virtual	status_t			Read(io_request* request) = 0;

	virtual	status_t			ReadSymlink(void* buffer,
									size_t* bufferSize) = 0;

	virtual	status_t			OpenAttributeDirectory(
									AttributeDirectoryCookie*& _cookie);
	virtual	status_t			OpenAttribute(const char* name, int openMode,
									AttributeCookie*& _cookie);

	virtual	status_t			IndexAttribute(AttributeIndexer* indexer);
	virtual	void*				IndexCookieForAttribute(const char* name) const;

protected:
			rw_lock				fLock;
			ino_t				fID;
			Directory*			fParent;
			char*				fName;
			Node*				fNameHashTableNext;
			Node*				fIDHashTableNext;
			uint32				fFlags;
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
