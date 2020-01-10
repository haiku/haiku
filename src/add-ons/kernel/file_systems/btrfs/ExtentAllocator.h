/*
 * Copyright 2017, Chế Vũ Gia Hy, cvghy116@gmail.com.
 * This file may be used under the terms of the MIT License.
 */
#ifndef EXTENT_ALLOCATOR_H
#define EXTENT_ALLOCATOR_H


#include "BTree.h"
#include "Volume.h"

#include "system_dependencies.h"


#ifdef FS_SHELL
#define ERROR(x...) TRACE(x)
#else
#include <DebugSupport.h>
#endif


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


struct CachedExtent : AVLTreeNode {
	uint64		offset;
	uint64		length;
	int			refCount;
	uint64		flags;
	btrfs_extent* item;

	static	CachedExtent*		Create(uint64 offset, uint64 length,
									uint64 flags);
			void				Delete();

			uint64				End() const { return offset + length; }
			bool				IsAllocated() const;
			bool				IsData() const;

			void				Info() const;

private:
								CachedExtent() {}
								CachedExtent(const CachedExtent& other);
};


struct TreeDefinition {
	typedef uint64			Key;
	typedef CachedExtent	Value;

	AVLTreeNode* GetAVLTreeNode(Value* value) const
	{
		return value;
	}

	Value* GetValue(AVLTreeNode* node) const
	{
		return static_cast<Value*>(node);
	}

	int Compare(const Key& a, const Value* b) const
	{
		if (a < b->offset)
			return -1;
		if (a >= b->End())
			return 1;
		return 0;
	}

	int Compare(const Value* a, const Value* b) const
	{
		int comp = Compare(a->offset, b);
		// TODO: check more conditions here if necessary
		return comp;
	}
};


struct CachedExtentTree : AVLTree<TreeDefinition> {
public:
								CachedExtentTree(
									const TreeDefinition& definition);
								~CachedExtentTree();

			status_t			FindNext(CachedExtent** chosen, uint64 offset,
									uint64 size, uint64 type);
			status_t			AddExtent(CachedExtent* extent);
			status_t			FillFreeExtents(uint64 lowerBound,
									uint64 upperBound);
			void				DumpInOrder() const;
			void				Delete();
private:
			status_t			_AddAllocatedExtent(CachedExtent* node);
			status_t			_AddFreeExtent(CachedExtent* node);
			void				_CombineFreeExtent(CachedExtent* node);
			void				_RemoveExtent(CachedExtent* node);
			void				_DumpInOrder(CachedExtent* node) const;
			void				_Delete(CachedExtent* node);
};


class BlockGroup {
public:
								BlockGroup(BTree* extentTree);
								~BlockGroup();

			status_t			SetExtentTree(off_t rootAddress);
			status_t			Initialize(uint64 flag);
			status_t			LoadExtent(CachedExtentTree* tree,
									bool inverse = false);

			uint64				Flags() const { return fItem->Flags(); }
			uint64				Start() const { return fKey.ObjectID(); }
			uint64				End() const
									{ return fKey.ObjectID() + fKey.Offset(); }

private:
								BlockGroup(const BlockGroup&);
								BlockGroup& operator=(const BlockGroup&);
			status_t			_InsertExtent(CachedExtentTree* tree,
									uint64 size, uint64 length, uint64 flags);
			status_t			_InsertExtent(CachedExtentTree* tree,
									CachedExtent* extent);

private:
			btrfs_key			fKey;
			btrfs_block_group*	fItem;
			BTree*				fCurrentExtentTree;
};


class ExtentAllocator {
public:
								ExtentAllocator(Volume* volume);
								~ExtentAllocator();

			status_t			Initialize();
			status_t			AllocateTreeBlock(uint64& found,
									uint64 start = (uint64)-1,
									uint64 flags = BTRFS_BLOCKGROUP_FLAG_METADATA);
			status_t			AllocateDataBlock(uint64& found, uint64 size,
									uint64 start = (uint64)-1,
									uint64 flags = BTRFS_BLOCKGROUP_FLAG_DATA);
private:
								ExtentAllocator(const ExtentAllocator&);
								ExtentAllocator& operator=(const ExtentAllocator&);
			status_t			_LoadExtentTree(uint64 flags);
			status_t			_Allocate(uint64& found, uint64 start,
									uint64 size, uint64 type);
private:
			Volume*				fVolume;
			CachedExtentTree*	fTree;
			uint64				fStart;
			uint64				fEnd;
};


#endif	// EXTENT_ALLOCATOR_H
