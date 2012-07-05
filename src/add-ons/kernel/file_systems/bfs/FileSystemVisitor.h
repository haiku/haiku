/*
 * Copyright 2002-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Andreas Henriksson, sausageboy@gmail.com
 * This file may be used under the terms of the MIT License.
 */
#ifndef FILE_SYSTEM_VISITOR_H
#define FILE_SYSTEM_VISITOR_H


#include "system_dependencies.h"

#include "bfs.h"


class Inode;
class TreeIterator;
class Volume;


enum visitor_flags {
	VISIT_REGULAR				= 0x0001,
	VISIT_INDICES				= 0x0002,
	VISIT_REMOVED				= 0x0004,
	VISIT_ATTRIBUTE_DIRECTORIES	= 0x0008
};


class FileSystemVisitor {
public:
								FileSystemVisitor(Volume* volume);
	virtual						~FileSystemVisitor();

			Volume*				GetVolume() const { return fVolume; }

			// traversing the file system
			status_t			Next();
			void				Start(uint32 flags);
			void				Stop();


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
			Volume*				fVolume;

			// state needed when traversing
			block_run			fCurrent;
			Inode*				fParent;
			Stack<block_run>	fStack;
			TreeIterator*		fIterator;

			uint32				fFlags;
};


#endif	// FILE_SYSTEM_VISITOR_H
