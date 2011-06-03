// Block.h

#ifndef BLOCK_H
#define BLOCK_H

class Block;
class BlockHeader;
class BlockReference;
class TFreeBlock;

#include <SupportDefs.h>

// debugging
//#define inline
#define BA_DEFINE_INLINES	1

// BlockHeader
class BlockHeader {
public:
	inline Block *ToBlock()				{ return (Block*)this; }
	inline TFreeBlock *ToFreeBlock()	{ return (TFreeBlock*)this; }

	inline void SetPreviousBlock(Block *block);
	inline Block *GetPreviousBlock();

	inline void SetNextBlock(Block *block);
	inline Block *GetNextBlock();
	inline bool HasNextBlock()			{ return (fSize & HAS_NEXT_FLAG); }

	inline void SetSize(size_t size, bool hasNext = false);
	inline size_t GetSize() const;
	static inline size_t GetUsableSizeFor(size_t size);
	inline size_t GetUsableSize() const;

	inline void *GetData();

	inline void SetFree(bool flag);
	inline bool IsFree() const;

	inline void SetReference(BlockReference *ref);
	inline BlockReference *GetReference() const		{ return fReference; }
	inline void FixReference();

	inline void SetTo(Block *previous, size_t size, bool isFree, bool hasNext,
					  BlockReference *reference = NULL);

private:
	enum {
		FREE_FLAG		= 0x80000000,
		BACK_SKIP_MASK	= 0x7fffffff,
	};

	enum {
		HAS_NEXT_FLAG	= 0x80000000,
		SIZE_MASK		= 0x7fffffff,
	};

protected:
	BlockHeader();
	~BlockHeader();

protected:
	size_t			fBackSkip;
	size_t			fSize;
	BlockReference	*fReference;
};

// Block
class Block : public BlockHeader {
public:
	static inline Block *MakeBlock(void *address, ssize_t offset,
								  Block *previous, size_t size, bool isFree,
								  bool hasNext,
								  BlockReference *reference = NULL);

protected:
	Block();
	~Block();
};

// TFreeBlock
class TFreeBlock : public Block {
public:

	inline void SetPreviousFreeBlock(TFreeBlock *block)	{ fPrevious = block; }
	inline void SetNextFreeBlock(TFreeBlock *block)		{ fNext = block; }
	inline TFreeBlock *GetPreviousFreeBlock()			{ return fPrevious; }
	inline TFreeBlock *GetNextFreeBlock()				{ return fNext; }

	inline void SetTo(Block *previous, size_t size, bool hasNext,
					  TFreeBlock *previousFree, TFreeBlock *nextFree);

//	static inline TFreeBlock *MakeFreeBlock(void *address, ssize_t offset,
//		Block *previous, size_t size, bool hasNext, TFreeBlock *previousFree,
//		TFreeBlock *nextFree);

protected:
	TFreeBlock();
	~TFreeBlock();

private:
	TFreeBlock	*fPrevious;
	TFreeBlock	*fNext;
};

// BlockReference
class BlockReference {
public:
	inline BlockReference()				: fBlock(NULL) {}
	inline BlockReference(Block *block) : fBlock(block) {}

	inline void SetBlock(Block *block)	{ fBlock = block; }
	inline Block *GetBlock() const		{ return fBlock; }

	inline void *GetData() const		{ return fBlock->GetData(); }
	inline void *GetDataAt(ssize_t offset) const;

private:
	Block	*fBlock;
};


// ---------------------------------------------------------------------------
// inline methods

// debugging
#if BA_DEFINE_INLINES

// BlockHeader

// SetPreviousBlock
inline
void
BlockHeader::SetPreviousBlock(Block *block)
{
	size_t offset = (block ? (char*)this - (char*)block : 0);
	fBackSkip = (fBackSkip & FREE_FLAG) | offset;
}

// GetPreviousBlock
inline
Block *
BlockHeader::GetPreviousBlock()
{
	if (fBackSkip & BACK_SKIP_MASK)
		return (Block*)((char*)this - (fBackSkip & BACK_SKIP_MASK));
	return NULL;
}

// SetNextBlock
inline
void
BlockHeader::SetNextBlock(Block *block)
{
	if (block)
		fSize = ((char*)block - (char*)this) | HAS_NEXT_FLAG;
	else
		fSize &= SIZE_MASK;
}

// GetNextBlock
inline
Block *
BlockHeader::GetNextBlock()
{
	if (fSize & HAS_NEXT_FLAG)
		return (Block*)((char*)this + (SIZE_MASK & fSize));
	return NULL;
}

// SetSize
inline
void
BlockHeader::SetSize(size_t size, bool hasNext)
{
	fSize = size;
	if (hasNext)
		fSize |= HAS_NEXT_FLAG;
}

// GetSize
inline
size_t
BlockHeader::GetSize() const
{
	return (fSize & SIZE_MASK);
}

// GetUsableSizeFor
inline
size_t
BlockHeader::GetUsableSizeFor(size_t size)
{
	return (size - sizeof(BlockHeader));
}

// GetUsableSize
inline
size_t
BlockHeader::GetUsableSize() const
{
	return GetUsableSizeFor(GetSize());
}

// GetData
inline
void *
BlockHeader::GetData()
{
	return (char*)this + sizeof(BlockHeader);
}

// SetFree
inline
void
BlockHeader::SetFree(bool flag)
{
	if (flag)
		fBackSkip |= FREE_FLAG;
	else
		fBackSkip &= ~FREE_FLAG;
}

// IsFree
inline
bool
BlockHeader::IsFree() const
{
	return (fBackSkip & FREE_FLAG);
}

// SetTo
inline
void
BlockHeader::SetTo(Block *previous, size_t size, bool isFree, bool hasNext,
				   BlockReference *reference)
{
	SetPreviousBlock(previous);
	SetSize(size, hasNext);
	SetFree(isFree);
	SetReference(reference);
}

// SetReference
inline
void
BlockHeader::SetReference(BlockReference *ref)
{
	fReference = ref;
	FixReference();
}

// FixReference
inline
void
BlockHeader::FixReference()
{
	if (fReference)
		fReference->SetBlock(ToBlock());
}


// Block

// MakeBlock
/*inline
Block *
Block::MakeBlock(void *address, ssize_t offset, Block *previous, size_t size,
				 bool isFree, bool hasNext, BlockReference *reference)
{
	Block *block = (Block*)((char*)address + offset);
	block->SetTo(previous, size, isFree, hasNext, reference);
	return block;
}*/


// TFreeBlock

// SetTo
inline
void
TFreeBlock::SetTo(Block *previous, size_t size, bool hasNext,
				  TFreeBlock *previousFree, TFreeBlock *nextFree)
{
	Block::SetTo(previous, size, true, hasNext, NULL);
	SetPreviousFreeBlock(previousFree);
	SetNextFreeBlock(nextFree);
}

// MakeFreeBlock
/*inline
TFreeBlock *
TFreeBlock::MakeFreeBlock(void *address, ssize_t offset, Block *previous,
						  size_t size, bool hasNext, TFreeBlock *previousFree,
						  TFreeBlock *nextFree)
{
	TFreeBlock *block = (TFreeBlock*)((char*)address + offset);
	block->SetTo(previous, size, hasNext, previousFree, nextFree);
	if (hasNext)
		block->GetNextBlock()->SetPreviousBlock(block);
	return block;
}*/


// BlockReference

// GetDataAt
inline
void *
BlockReference::GetDataAt(ssize_t offset) const
{
	return (char*)fBlock->GetData() + offset;
}

#endif	// BA_DEFINE_INLINES

#endif	// BLOCK_H
