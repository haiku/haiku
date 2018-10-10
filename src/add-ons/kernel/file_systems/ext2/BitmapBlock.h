/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef BITMAPBLOCK_H
#define BITMAPBLOCK_H


#include "CachedBlock.h"


class BitmapBlock : public CachedBlock {
public:
							BitmapBlock(Volume* volume, uint32 numBits);
							~BitmapBlock();

			bool			SetTo(off_t block);
			bool			SetToWritable(Transaction& transaction,
								off_t block, bool empty = false);

			bool			CheckMarked(uint32 start, uint32 length);
			bool			CheckUnmarked(uint32 start, uint32 length);

			bool			Mark(uint32 start, uint32 length,
								bool force = false);
			bool			Unmark(uint32 start, uint32 length,
								bool force = false);

			void			FindNextMarked(uint32& pos);
			void			FindNextUnmarked(uint32& pos);

			void			FindPreviousMarked(uint32& pos);

			void			FindLargestUnmarkedRange(uint32& start,
								uint32& length);

			uint32			NumBits() const { return fNumBits; }
			uint32			Checksum(uint32 unitsPerGroup) const;

private:
			bool			_Check(uint32 start, uint32 length, bool marked);
			void			_FindNext(uint32& pos, bool marked);
			bool			_Update(uint32 start, uint32 length, bool mark,
								bool force);

			Volume*			fVolume;
			uint32*			fData;
			const uint32*	fReadOnlyData;

			uint32			fNumBits;
			uint32			fMaxIndex;
};


inline bool
BitmapBlock::CheckUnmarked(uint32 start, uint32 length)
{
	return _Check(start, length, false);
}


inline bool
BitmapBlock::CheckMarked(uint32 start, uint32 length)
{
	return _Check(start, length, true);
}


inline bool
BitmapBlock::Mark(uint32 start, uint32 length, bool force)
{
	return _Update(start, length, true, force);
}


inline bool
BitmapBlock::Unmark(uint32 start, uint32 length, bool force)
{
	return _Update(start, length, false, force);
}


inline void
BitmapBlock::FindNextMarked(uint32& pos)
{
	_FindNext(pos, true);
}


inline void
BitmapBlock::FindNextUnmarked(uint32& pos)
{
	_FindNext(pos, false);
}


#endif	// BITMAPBLOCK_H
