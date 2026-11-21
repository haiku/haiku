/*
 * Copyright 2013-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef UTIL_FIXED_SIZE_BITMAP_H
#define UTIL_FIXED_SIZE_BITMAP_H


#include <SupportDefs.h>
#include <string.h>


template<size_t BitsCount>
class FixedSizeBitmap {
	typedef FixedSizeBitmap<BitsCount> Bitmap;

public:
	inline				FixedSizeBitmap() { ClearAll(); }

	inline	void		ClearAll();
	inline	void		SetAll();

	inline	void		SetBit(int32 bit);
	inline	bool		GetBit(int32 bit) const;
	inline	void		ClearBit(int32 bit);

	inline	void		SetBitAtomic(int32 bit);
	inline	bool		GetBitAtomic(int32 bit) const;
	inline	void		ClearBitAtomic(int32 bit);

	inline	bool		Matches(const Bitmap& mask) const;
	inline	Bitmap		And(const Bitmap& mask) const;
	inline	bool		operator==(const Bitmap& other) const;

	inline	bool		IsEmpty() const;

	inline uint32		Bits(uint32 index) const { return fBitmap[index];}

private:
	static	const int	kArrayBits = 32;
	static	const int	kArraySize = (BitsCount + kArrayBits - 1) / kArrayBits;

			uint32		fBitmap[kArraySize];
};


template<size_t BitsCount> inline void
FixedSizeBitmap<BitsCount>::ClearAll()
{
	memset(fBitmap, 0, sizeof(fBitmap));
}


template<size_t BitsCount> inline void
FixedSizeBitmap<BitsCount>::SetAll()
{
	memset(fBitmap, ~uint8(0), sizeof(fBitmap));
}


template<size_t BitsCount>
inline void
FixedSizeBitmap<BitsCount>::SetBit(int32 bit)
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	*element |= 1u << (bit % kArrayBits);
}


template<size_t BitsCount>
inline void
FixedSizeBitmap<BitsCount>::ClearBit(int32 bit)
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	*element &= ~uint32(1u << (bit % kArrayBits));
}


template<size_t BitsCount>
inline void
FixedSizeBitmap<BitsCount>::SetBitAtomic(int32 bit)
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	atomic_or(element, 1u << (bit % kArrayBits));
}


template<size_t BitsCount>
inline void
FixedSizeBitmap<BitsCount>::ClearBitAtomic(int32 bit)
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	atomic_and(element, ~uint32(1u << (bit % kArrayBits)));
}


template<size_t BitsCount>
inline bool
FixedSizeBitmap<BitsCount>::GetBit(int32 bit) const
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	return ((uint32)*element & (1u << (bit % kArrayBits))) != 0;
}


template<size_t BitsCount>
inline bool
FixedSizeBitmap<BitsCount>::GetBitAtomic(int32 bit) const
{
	int32* element = (int32*)&fBitmap[bit / kArrayBits];
	return ((uint32)atomic_get(element) & (1u << (bit % kArrayBits))) != 0;
}


template<size_t BitsCount>
inline FixedSizeBitmap<BitsCount>
FixedSizeBitmap<BitsCount>::And(const FixedSizeBitmap<BitsCount>& mask) const
{
	FixedSizeBitmap<BitsCount> andSet;
	for (int i = 0; i < kArraySize; i++)
		andSet.fBitmap[i] = fBitmap[i] & mask.fBitmap[i];
	return andSet;
}


template<size_t BitsCount>
inline bool
FixedSizeBitmap<BitsCount>::Matches(const FixedSizeBitmap<BitsCount>& mask) const
{
	for (int i = 0; i < kArraySize; i++) {
		if ((fBitmap[i] & mask.fBitmap[i]) != 0)
			return true;
	}

	return false;
}


template<size_t BitsCount>
inline bool
FixedSizeBitmap<BitsCount>::operator==(const FixedSizeBitmap<BitsCount>& other) const
{
	for (int i = 0; i < kArraySize; i++) {
		if (fBitmap[i] != other.fBitmap[i])
			return false;
	}

	return true;
}


template<size_t BitsCount>
inline bool
FixedSizeBitmap<BitsCount>::IsEmpty() const
{
	for (int i = 0; i < kArraySize; i++) {
		if (fBitmap[i] != 0)
			return false;
	}

	return true;
}


#endif	// UTIL_FIXED_SIZE_BITMAP_H
