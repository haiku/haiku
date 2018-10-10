/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef REVOKEMANAGER_H
#define REVOKEMANAGER_H

#include "Journal.h"


struct JournalRevokeHeader;

class RevokeManager {
public:
						RevokeManager(bool has64bits);
	virtual				~RevokeManager() = 0;

	virtual	status_t	Insert(uint32 block, uint32 commitID) = 0;
	virtual	status_t	Remove(uint32 block) = 0;
	virtual	bool		Lookup(uint32 block, uint32 commitID) = 0;
			
			uint32		NumRevokes() { return fRevokeCount; }

			status_t	ScanRevokeBlock(JournalRevokeHeader* revokeBlock,
							uint32 commitID);

protected:
			uint32		fRevokeCount;
			bool		fHas64bits;
};

#endif	// REVOKEMANAGER_H

