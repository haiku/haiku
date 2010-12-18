/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef INODEJOURNAL_H
#define INODEJOURNAL_H


#include "Inode.h"
#include "Journal.h"


class InodeJournal : public Journal {
public:
						InodeJournal(Inode* inode);
						~InodeJournal();

			status_t	InitCheck();
			
			status_t	MapBlock(off_t logical, fsblock_t& physical);
private:
			Inode*		fInode;
			status_t	fInitStatus;
};

#endif	// INODEJOURNAL_H
