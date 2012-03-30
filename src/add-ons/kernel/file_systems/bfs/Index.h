/*
 * Copyright 2001-2012, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef INDEX_H
#define INDEX_H


#include "system_dependencies.h"


class Transaction;
class Volume;
class Inode;


class Index {
public:
							Index(Volume* volume);
							~Index();

			status_t		SetTo(const char* name);
			void			Unset();

			Inode*			Node() const { return fNode; };
			uint32			Type();
			size_t			KeySize();

			status_t		Create(Transaction& transaction, const char* name,
								uint32 type);

			status_t		Update(Transaction& transaction, const char* name,
								int32 type, const uint8* oldKey,
								uint16 oldLength, const uint8* newKey,
								uint16 newLength, Inode* inode);

			status_t		InsertName(Transaction& transaction,
								const char* name, Inode* inode);
			status_t		RemoveName(Transaction& transaction,
								const char* name, Inode* inode);
			status_t		UpdateName(Transaction& transaction,
								const char* oldName, const char* newName,
								Inode* inode);

			status_t		InsertSize(Transaction& transaction, Inode* inode);
			status_t		RemoveSize(Transaction& transaction, Inode* inode);
			status_t		UpdateSize(Transaction& transaction, Inode* inode);

			status_t		InsertLastModified(Transaction& transaction,
								Inode* inode);
			status_t		RemoveLastModified(Transaction& transaction,
								Inode* inode);
			status_t		UpdateLastModified(Transaction& transaction,
								Inode* inode, bigtime_t modified = -1);

private:
							Index(const Index& other);
							Index& operator=(const Index& other);
								// no implementation

private:
			Volume*			fVolume;
			Inode*			fNode;
			const char*		fName;
};


#endif	// INDEX_H
