/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef HASHREVOKEMANAGER_H
#define HASHREVOKEMANAGER_H

#include <util/OpenHashTable.h>

#include "RevokeManager.h"


struct RevokeElement {
	RevokeElement*	next;	// Next in hash
	uint32			block;
	uint32			commitID;
};


struct RevokeHash {
		typedef uint32			KeyType;
		typedef	RevokeElement	ValueType;

		size_t HashKey(KeyType key) const
		{
			return key;
		}

		size_t Hash(ValueType* value) const
		{
			return HashKey(value->block);
		}

		bool Compare(KeyType key, ValueType* value) const
		{
			return value->block == key;
		}

		ValueType*& GetLink(ValueType* value) const
		{
			return value->next;
		}

};

typedef BOpenHashTable<RevokeHash> RevokeTable;


class HashRevokeManager : public RevokeManager {
public:
						HashRevokeManager(bool has64bits);
	virtual				~HashRevokeManager();

			status_t	Init();

	virtual	status_t	Insert(uint32 block, uint32 commitID);
	virtual	status_t	Remove(uint32 block);
	virtual	bool		Lookup(uint32 block, uint32 commitID);

	static	int			Compare(void* element, const void* key);
	static	uint32		Hash(void* element, const void* key, uint32 range);

protected:
			status_t	_ForceInsert(uint32 block, uint32 commitID);

private:
			RevokeTable*	fHash;

	const	int			kInitialHashSize;
};

#endif	// HASHREVOKEMANAGER_H

