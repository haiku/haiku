/*
 * Copyright 2002-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com
 * This file may be used under the terms of the MIT License.
 */
#ifndef CHECK_VISITOR_H
#define CHECK_VISITOR_H


#include "system_dependencies.h"


#include "bfs_control.h"
#include "FileSystemVisitor.h"


class BlockAllocator;
class BPlusTree;
struct check_index;

typedef Stack<check_index*> IndexStack;


class CheckVisitor : public FileSystemVisitor {
public:
								CheckVisitor(Volume* volume);
	virtual						~CheckVisitor();

			check_control&		Control() { return control; }
			IndexStack&			Indices() { return indices; }
			uint32				Pass() { return control.pass; }

			status_t			StartBitmapPass();
			status_t			WriteBackCheckBitmap();
			status_t			StartIndexPass();
			status_t			StopChecking();

	virtual status_t			VisitDirectoryEntry(Inode* inode,
									Inode* parent, const char* treeName);
	virtual status_t			VisitInode(Inode* inode, const char* treeName);

	virtual status_t			OpenInodeFailed(status_t reason, ino_t id,
									Inode* parent, char* treeName,
									TreeIterator* iterator);
	virtual status_t			OpenBPlusTreeFailed(Inode* inode);
	virtual status_t			TreeIterationFailed(status_t reason,
									Inode* parent);

private:
			status_t			_RemoveInvalidNode(Inode* parent,
									BPlusTree* tree, Inode* inode,
									const char* name);

			bool				_ControlValid();
			bool				_CheckBitmapIsUsedAt(off_t block) const;
			void				_SetCheckBitmapAt(off_t block);
			status_t			_CheckInodeBlocks(Inode* inode,
									const char* name);
			status_t			_CheckAllocated(block_run run,
									const char* type);

			size_t				_BitmapSize() const;

			status_t			_PrepareIndices();
			void				_FreeIndices();
			status_t			_AddInodeToIndex(Inode* inode);

private:
			check_control		control;
			IndexStack			indices;

			uint32*				fCheckBitmap;
};


#endif	// CHECK_VISITOR_H
