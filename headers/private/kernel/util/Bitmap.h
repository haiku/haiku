/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef KERNEL_UTIL_BITMAP_H
#define KERNEL_UTIL_BITMAP_H


#include <debug.h>

#include <SupportDefs.h>


class Bitmap {
public:
						Bitmap(int bitCount);
						~Bitmap();

	inline	status_t	GetInitStatus();

	inline	bool		Get(int index) const;
	inline	void		Set(int index);
	inline	void		Clear(int index);

			int			GetHighestSet() const;

private:
			status_t	fInitStatus;

			int			fElementsCount;
			int			fSize;
			addr_t*		fBits;

	static	const int	kBitsPerElement;
};


status_t
Bitmap::GetInitStatus()
{
	return fInitStatus;
}


bool
Bitmap::Get(int index) const
{
	ASSERT(index < fSize);

	const int kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	return fBits[kArrayElement] & kBitMask;
}


void
Bitmap::Set(int index)
{
	ASSERT(index < fSize);

	const int kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	fBits[kArrayElement] |= kBitMask;
}


void
Bitmap::Clear(int index)
{
	ASSERT(index < fSize);

	const int kArrayElement = index / kBitsPerElement;
	const addr_t kBitMask = addr_t(1) << (index % kBitsPerElement);
	fBits[kArrayElement] &= ~addr_t(kBitMask);
}


#endif	// KERNEL_UTIL_BITMAP_H

