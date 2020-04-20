// Iterators.h
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#ifndef ITERATORS_H
#define ITERATORS_H

#include <SupportDefs.h>

#include "DirItem.h"

class Block;
class InternalNode;
class VKey;
class LeafNode;
class Node;
class Tree;

// TreePath
class TreePath {
public:
	class Element;

public:
	TreePath(uint32 maxLength);
	~TreePath();

	status_t InitCheck() const;

	uint32 GetMaxLength() const;
	uint32 GetLength() const;
	const Element *ElementAt(int32 index) const;
	const Element *GetTopElement() const;

	status_t PushElement(uint64 blockNumber, int32 index);
	status_t PopElement();

private:
	uint32	fMaxLength;
	uint32	fLength;
	Element	*fElements;
};

// TreePath::Element
class TreePath::Element {
public:
	Element() {}
	~Element() {}

	status_t SetTo(uint64 blockNumber, int32 index);

	uint64 GetBlockNumber() const;
	int32 GetIndex() const;

private:
	uint64	fBlockNumber;
	uint32	fIndex;
};


// TreeIterator
class TreeIterator {
public:
	TreeIterator(Tree *tree);
	~TreeIterator();

	status_t SetTo(Tree *tree);
	void Unset();
	status_t InitCheck() const;

	Tree *GetTree() const { return fTree; }

	Node *GetNode() const;
	int32 GetLevel() const;

	status_t GoTo(uint32 direction);

	status_t GoToPreviousLeaf(LeafNode **node = NULL);
	status_t GoToNextLeaf(LeafNode **node = NULL);
	status_t FindRightMostLeaf(const VKey *k, LeafNode **node = NULL);

	status_t Suspend();
	status_t Resume();

public:
	enum {
		FORWARD,
		BACKWARDS,
		UP,
		DOWN,
	};

private:
	status_t _PushCurrentNode(Node *newTopNode = NULL, int32 newIndex = 0);
	status_t _PopTopNode();
	status_t _SearchRightMost(InternalNode *node, const VKey *k, int32 *index);

private:
	Tree		*fTree;
	Node		*fCurrentNode;
	int32		fIndex;
	TreePath	*fPath;
};

// ItemIterator
class ItemIterator {
public:
	ItemIterator(Tree *tree);

	status_t SetTo(Tree *tree);
	status_t InitCheck() const;

	Tree *GetTree() const { return fTreeIterator.GetTree(); }

	status_t GetCurrent(Item *item);
	status_t GoToPrevious(Item *item = NULL);
	status_t GoToNext(Item *item = NULL);
	status_t FindRightMostClose(const VKey *k, Item *item = NULL);
	status_t FindRightMost(const VKey *k, Item *item = NULL);

	status_t Suspend();
	status_t Resume();

private:
	status_t _GetLeafNode(LeafNode **node) const;
	status_t _SearchRightMost(LeafNode *node, const VKey *k,
							  int32 *index) const;

private:
	TreeIterator	fTreeIterator;
	int32			fIndex;
};

// ObjectItemIterator
class ObjectItemIterator {
public:
	ObjectItemIterator(Tree *tree, uint32 dirID, uint32 objectID,
					   uint64 startOffset = 0);

	status_t SetTo(Tree *tree, uint32 dirID, uint32 objectID,
				   uint64 startOffset = 0);
	status_t InitCheck() const;

	Tree *GetTree() const { return fItemIterator.GetTree(); }
	uint32 GetDirID() const { return fDirID; }
	uint32 GetObjectID() const { return fObjectID; }
	uint64 GetOffset() const { return fOffset; }

	status_t GetNext(Item *foundItem);
	status_t GetNext(Item *foundItem, uint32 type);
	status_t GetPrevious(Item *foundItem);
	status_t GetPrevious(Item *foundItem, uint32 type);

	status_t Suspend();
	status_t Resume();

private:
	ItemIterator	fItemIterator;
	uint32			fDirID;
	uint32			fObjectID;
	uint64			fOffset;
	bool			fFindFirst;
	bool			fDone;
};

// DirEntryIterator
class DirEntryIterator {
public:
	DirEntryIterator(Tree *tree, uint32 dirID, uint32 objectID,
					 uint64 startOffset = 0, bool fixedHash = false);

	status_t SetTo(Tree *tree, uint32 dirID, uint32 objectID,
				   uint64 startOffset = 0, bool fixedHash = false);
	status_t InitCheck() const;

	status_t Rewind();

	Tree *GetTree() const { return fItemIterator.GetTree(); }
	uint32 GetDirID() const { return fItemIterator.GetDirID(); }
	uint32 GetObjectID() const { return fItemIterator.GetObjectID(); }
	uint64 GetOffset() const { return fItemIterator.GetOffset(); }

	status_t GetNext(DirItem *foundItem, int32 *entryIndex,
					 DirEntry **entry = NULL);
	status_t GetPrevious(DirItem *foundItem, int32 *entryIndex,
						 DirEntry **entry = NULL);

	status_t Suspend();
	status_t Resume();

private:
	ObjectItemIterator	fItemIterator;
	DirItem				fDirItem;
	int32				fIndex;
	bool				fFixedHash;
	bool				fDone;
};

// StreamReader
class StreamReader {
public:
	StreamReader(Tree *tree, uint32 dirID, uint32 objectID);

	status_t SetTo(Tree *tree, uint32 dirID, uint32 objectID);
	status_t InitCheck() const;

	Tree *GetTree() const { return fItemIterator.GetTree(); }
	uint32 GetDirID() const { return fItemIterator.GetDirID(); }
	uint32 GetObjectID() const { return fItemIterator.GetObjectID(); }
	uint64 GetOffset() const { return fItemIterator.GetOffset(); }

	off_t StreamSize() const { return fStreamSize; }

	status_t ReadAt(off_t position, void *buffer, size_t bufferSize,
					size_t *bytesRead);

	status_t Suspend();
	status_t Resume();

private:
	status_t _GetStreamSize();
	status_t _SeekTo(off_t position);
	status_t _ReadIndirectItem(off_t offset, void *buffer, size_t bufferSize);
	status_t _ReadDirectItem(off_t offset, void *buffer, size_t bufferSize);

private:
	ObjectItemIterator	fItemIterator;
	Item				fItem;
	off_t				fStreamSize;
	off_t				fItemOffset;
	off_t				fItemSize;
	uint32				fBlockSize;
};

#endif	// ITERATORS_H
