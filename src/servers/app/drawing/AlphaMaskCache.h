/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ALPHA_MASK_CACHE_H
#define ALPHA_MASK_CACHE_H

#include <set>

#include "ShapePrivate.h"
#include <Locker.h>
#include <kernel/OS.h>


class AlphaMask;
class ShapeAlphaMask;


class AlphaMaskCache {
private:
	enum {
		kMaxCacheBytes = 8 * 1024 * 1024 // 8 MiB
	};

public:
								AlphaMaskCache();
								~AlphaMaskCache();

	static	AlphaMaskCache*		Default();

			status_t			Put(ShapeAlphaMask* mask);
			ShapeAlphaMask*		Get(const shape_data& shape,
									AlphaMask* previousMask,
									bool inverse);

			void				Clear();

private:
			size_t				_FindUncachedPreviousMasks(AlphaMask* mask,
									bool reference);
			void				_PrintAndResetStatistics();

private:
	struct ShapeMaskElement {
		ShapeMaskElement(const shape_data* shape,
			ShapeAlphaMask* mask, AlphaMask* previousMask,
			bool inverse)
			:
			fShape(shape),
			fInverse(inverse),
			fMask(mask),
			fPreviousMask(previousMask)
		{
		}

		bool operator<(const ShapeMaskElement& other) const
		{
			if (fInverse != other.fInverse)
				return fInverse < other.fInverse;
			if (fPreviousMask != other.fPreviousMask)
				return fPreviousMask < other.fPreviousMask;

			// compare shapes
			if (fShape->ptCount != other.fShape->ptCount)
				return fShape->ptCount < other.fShape->ptCount;
			if (fShape->opCount != other.fShape->opCount)
				return fShape->opCount < other.fShape->opCount;
			int diff = memcmp(fShape->ptList, other.fShape->ptList,
				fShape->ptSize);
			if (diff != 0)
				return diff < 0;
			diff = memcmp(fShape->opList, other.fShape->opList,
				fShape->opSize);
			if (diff != 0)
				return diff < 0;

			// equal
			return false;
		}

		const shape_data*	fShape;
		bool				fInverse;
		ShapeAlphaMask*		fMask;
		AlphaMask*			fPreviousMask;
	};

private:
	typedef std::set<ShapeMaskElement> ShapeMaskSet;

	static	AlphaMaskCache		sDefaultInstance;

			BLocker				fLock;

			size_t				fCurrentCacheBytes;
			ShapeMaskSet		fShapeMasks;

			// Statistics counters
			uint32				fTooLargeMaskCount;
			uint32				fMasksReplacedCount;
			uint32				fHitCount;
			uint32				fMissCount;
			uint32				fLowerMaskReferencedCount;
};


#endif // ALPHA_MASK_CACHE_H
