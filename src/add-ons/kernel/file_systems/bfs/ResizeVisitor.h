/*
 * ResizeVisitor.h
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com
 *
 * Distributed under terms of the MIT license.
 */

#ifndef RESIZE_VISITOR_H
#define RESIZE_VISITOR_H


#include "system_dependencies.h"

#include "bfs_control.h"
#include "FileSystemVisitor.h"


class Inode;
class Transaction;


class ResizeVisitor : public FileSystemVisitor {
public:
								ResizeVisitor(Volume* volume);
	virtual						~ResizeVisitor();

			status_t			Resize(off_t size, disk_job_id job);

	virtual status_t			VisitInode(Inode* inode, const char* treeName);

	virtual status_t			OpenInodeFailed(status_t reason, ino_t id,
									Inode* parent, char* treeName,
									TreeIterator* iterator);
	virtual status_t			OpenBPlusTreeFailed(Inode* inode);
	virtual status_t			TreeIterationFailed(status_t reason,
									Inode* parent);

private:
			void				_CalculateNewSizes(off_t size);
			status_t			_IsResizePossible(off_t size);
			status_t			_ResizeVolume();

			// moving the inode
			status_t			_UpdateParent(Transaction& transaction,
									Inode* inode, off_t newInodeID,
									const char* treeName);
			status_t			_UpdateAttributeDirectory(
									Transaction& transaction, Inode* inode,
									block_run newInodeRun);
			status_t			_UpdateIndexReferences(Transaction& transaction,
									Inode* inode, off_t newInodeID,
									bool rootOrIndexDir);
			status_t			_UpdateTree(Transaction& transaction,
									Inode* inode, off_t newInodeID);
			status_t			_UpdateChildren(Transaction& transaction,
									Inode* inode, off_t newInodeID);
			status_t			_UpdateSuperBlock(Inode* inode,
									off_t newInodeID);
			status_t			_MoveInode(Inode* inode, off_t& newInodeID,
									const char* treeName);

private:
			bool				fError;

			off_t				fNumBlocks;
			off_t				fBitmapBlocks;
			off_t				fReservedLength;

			block_run			fNewLog;

			off_t				fBeginBlock;
			off_t				fEndBlock;

			bool				fShrinking;
};


#endif	// RESIZE_VISITOR_H
