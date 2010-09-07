/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef HASHREVOKEMANAGER_H
#define HASHREVOKEMANAGER_H

#include <util/khash.h>

#include "RevokeManager.h"


struct RevokeElement {
	RevokeElement*	next;	// Next in hash
	uint32			block;
	uint32			commitID;
};


class HashRevokeManager : public RevokeManager {
public:
						HashRevokeManager();
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
			hash_table*	fHash;

	const	int			kInitialHashSize;
};

#endif	// HASHREVOKEMANAGER_H

