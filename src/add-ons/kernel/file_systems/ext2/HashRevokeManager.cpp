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


HashRevokeManager::HashRevokeManager(bool has64bits)
	: RevokeManager(has64bits),
	fHash(NULL),
	kInitialHashSize(128)
		// TODO: Benchmark and find an optimal value
{
}


HashRevokeManager::~HashRevokeManager()
{
	if (fHash != NULL) {
		if (fRevokeCount != 0) {
			RevokeElement *element = fHash->Clear(true);

			while (element != NULL) {
				RevokeElement* next = element->next;
				delete element;
				element = next;
			}
		}

		delete fHash;
	}
}


status_t
HashRevokeManager::Init()
{
	fHash = new(std::nothrow) RevokeTable();

	if (fHash == NULL || fHash->Init(kInitialHashSize) != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


status_t
HashRevokeManager::Insert(uint32 block, uint32 commitID)
{
	RevokeElement* element = fHash->Lookup(block);

	if (element != NULL) {
		TRACE("HashRevokeManager::Insert(): Already has an element\n");
		if (element->commitID < commitID) {
			TRACE("HashRevokeManager::Insert(): Deleting previous element\n");
			bool retValue = fHash->Remove(element);

			if (!retValue)
				return B_ERROR;

			delete element;
		} else {
			return B_OK;
				// We already have a newer version of the block
		}
	}

	return _ForceInsert(block, commitID);
}


status_t
HashRevokeManager::Remove(uint32 block)
{
	RevokeElement* element = fHash->Lookup(block);

	if (element == NULL)
		return B_ERROR; // TODO: Perhaps we should just ignore?

	fHash->Remove(element);
		// Can't fail as we just did a sucessful Lookup()

	delete element;
	return B_OK;
}


bool
HashRevokeManager::Lookup(uint32 block, uint32 commitID)
{
	RevokeElement* element = fHash->Lookup(block);

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
	TRACE("HashRevokeManager::Hash(): revoked: %p, block: %p, range: %"
		B_PRIu32 "\n", _revoked, _block, range);
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

	status_t retValue = fHash->Insert(element);

	if (retValue == B_OK) {
		fRevokeCount++;
		TRACE("HashRevokeManager::_ForceInsert(): revoke count: %" B_PRIu32
			"\n", fRevokeCount);
	}

	return retValue;
}

