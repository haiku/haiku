/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "RevokeManager.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


RevokeManager::RevokeManager(bool has64bits)
	:
	fRevokeCount(0),
	fHas64bits(has64bits)
{
}


RevokeManager::~RevokeManager()
{
}


status_t
RevokeManager::ScanRevokeBlock(JournalRevokeHeader* revokeBlock,
	uint32 commitID)
{
	TRACE("RevokeManager::ScanRevokeBlock(): Commit ID: %" B_PRIu32 "\n",
		commitID);
	int count = revokeBlock->NumBytes() / 4;
	
	for (int i = 0; i < count; ++i) {
		TRACE("RevokeManager::ScanRevokeBlock(): Found a revoked block: %"
			B_PRIu32 "\n", revokeBlock->RevokeBlock(i));
		status_t status = Insert(revokeBlock->RevokeBlock(i), commitID);
		
		if (status != B_OK) {
			TRACE("RevokeManager::ScanRevokeBlock(): Error inserting\n");
			return status;
		}
	}

	return B_OK;
}

