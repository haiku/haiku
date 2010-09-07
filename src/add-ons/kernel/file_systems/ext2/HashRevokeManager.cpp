/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "HashRevokeManager.h"

#include <new>


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


HashRevokeManager::HashRevokeManager()
	:
	fHash(NULL),
	kInitialHashSize(128)
		// TODO: Benchmark and find an optimal value
{
}


HashRevokeManager::~HashRevokeManager()
{
	if (fHash != NULL) {
		if (fRevokeCount != 0) {
			RevokeElement *element =
				(RevokeElement*)hash_remove_first(fHash, NULL);

			while (element != NULL) {
				delete element;
				element = (RevokeElement*)hash_remove_first(fHash, NULL);
			}
		}

		hash_uninit(fHash);
	}
}


status_t
HashRevokeManager::Init()
{
	RevokeElement dummyElement;
	
	fHash = hash_init(kInitialHashSize, offset_of_member(dummyElement, next),
		&HashRevokeManager::Compare,
		&HashRevokeManager::Hash);

	if (fHash == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
HashRevokeManager::Insert(uint32 block, uint32 commitID)
{
	RevokeElement* element = (RevokeElement*)hash_lookup(fHash, &block);
   
	if (element != NULL) {
		TRACE("HashRevokeManager::Insert(): Already has an element\n");
		if (element->commitID < commitID) {
			TRACE("HashRevokeManager::Insert(): Deleting previous element\n");
			status_t retValue = hash_remove(fHash, element);
			
			if (retValue != B_OK)
				return retValue;

			delete element;
		}
		else {
			return B_OK;
				// We already have a newer version of the block
		}
	}

	return _ForceInsert(block, commitID);
}


status_t
HashRevokeManager::Remove(uint32 block)
{
	RevokeElement* element = (RevokeElement*)hash_lookup(fHash, &block);

	if (element == NULL)
		return B_ERROR; // TODO: Perhaps we should just ignore?

	status_t retValue = hash_remove(fHash, element);
	
	if (retValue == B_OK)
		delete element;

	return retValue;
}


bool
HashRevokeManager::Lookup(uint32 block, uint32 commitID)
{
	RevokeElement* element = (RevokeElement*)hash_lookup(fHash, &block);

	if (element == NULL)
		return false;

	return element->commitID >= commitID;
}


/*static*/ int
HashRevokeManager::Compare(void* _revoked, const void *_block)
{
	RevokeElement* revoked = (RevokeElement*)_revoked;
	uint32 block = *(uint32*)_block;

	if (revoked->block == block)
		return 0;

	return (revoked->block > block) ? 1 : -1;
}


/*static*/ uint32
HashRevokeManager::Hash(void* _revoked, const void* _block, uint32 range)
{
	TRACE("HashRevokeManager::Hash(): revoked: %p, block: %p, range: %lu\n",
		_revoked, _block, range);
	RevokeElement* revoked = (RevokeElement*)_revoked;

	if (revoked != NULL)
		return revoked->block % range;

	uint32 block = *(uint32*)_block;
	return block % range;
}


status_t
HashRevokeManager::_ForceInsert(uint32 block, uint32 commitID)
{
	RevokeElement* element = new(std::nothrow) RevokeElement;

	if (element == NULL)
		return B_NO_MEMORY;

	element->block = block;
	element->commitID = commitID;

	status_t retValue = hash_insert_grow(fHash, element);

	if (retValue == B_OK) {
		fRevokeCount++;
		TRACE("HashRevokeManager::_ForceInsert(): revoke count: %lu\n",
			fRevokeCount);
	}

	return retValue;
}

