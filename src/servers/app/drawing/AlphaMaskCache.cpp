/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AlphaMaskCache.h"

#include "AlphaMask.h"
#include "ShapePrivate.h"

#include <AutoLocker.h>


//#define PRINT_ALPHA_MASK_CACHE_STATISTICS
#ifdef PRINT_ALPHA_MASK_CACHE_STATISTICS
static uint32 sAlphaMaskGetCount = 0;
#endif


AlphaMaskCache AlphaMaskCache::sDefaultInstance;


AlphaMaskCache::AlphaMaskCache()
	:
	fLock("AlphaMask cache"),
	fCurrentCacheBytes(0),
	fTooLargeMaskCount(0),
	fMasksReplacedCount(0),
	fHitCount(0),
	fMissCount(0),
	fLowerMaskReferencedCount(0)
{
}


AlphaMaskCache::~AlphaMaskCache()
{
	Clear();
}


/* static */ AlphaMaskCache*
AlphaMaskCache::Default()
{
	return &sDefaultInstance;
}


status_t
AlphaMaskCache::Put(ShapeAlphaMask* mask)
{
	AutoLocker<BLocker> locker(fLock);

	size_t maskStackSize = mask->BitmapSize();
	maskStackSize += _FindUncachedPreviousMasks(mask, true);

	if (maskStackSize > kMaxCacheBytes) {
		_FindUncachedPreviousMasks(mask, false);
		fTooLargeMaskCount++;
		return B_NO_MEMORY;
	}

	if (fCurrentCacheBytes + maskStackSize > kMaxCacheBytes) {
		for (ShapeMaskSet::iterator it = fShapeMasks.begin();
			it != fShapeMasks.end();) {

			if (atomic_get(&it->fMask->fNextMaskCount) > 0) {
				it++;
				continue;
			}

			size_t removedMaskStackSize = it->fMask->BitmapSize();
			removedMaskStackSize += _FindUncachedPreviousMasks(it->fMask,
				false);
			fCurrentCacheBytes -= removedMaskStackSize;

			it->fMask->fInCache = false;
			it->fMask->ReleaseReference();
			fMasksReplacedCount++;
			fShapeMasks.erase(it++);

			if (fCurrentCacheBytes + maskStackSize <= kMaxCacheBytes)
				break;
		}
	}

	if (fCurrentCacheBytes + maskStackSize > kMaxCacheBytes) {
		_FindUncachedPreviousMasks(mask, false);
		fTooLargeMaskCount++;
		return B_NO_MEMORY;
	}

	fCurrentCacheBytes += maskStackSize;

	ShapeMaskElement element(mask->fShape, mask, mask->fPreviousMask.Get(),
		mask->fInverse);
	fShapeMasks.insert(element);
	mask->AcquireReference();
	mask->fInCache = true;
	return B_OK;
}


ShapeAlphaMask*
AlphaMaskCache::Get(const shape_data& shape, AlphaMask* previousMask,
	bool inverse)
{
	AutoLocker<BLocker> locker(fLock);

#ifdef PRINT_ALPHA_MASK_CACHE_STATISTICS
	if (sAlphaMaskGetCount++ > 200) {
		_PrintAndResetStatistics();
		sAlphaMaskGetCount = 0;
	}
#endif

	ShapeMaskElement element(&shape, NULL, previousMask, inverse);
	ShapeMaskSet::iterator it = fShapeMasks.find(element);
 	if (it == fShapeMasks.end()) {
		fMissCount++;
		return NULL;
 	}
	fHitCount++;
	it->fMask->AcquireReference();
	return it->fMask;
}


void
AlphaMaskCache::Clear()
{
	AutoLocker<BLocker> locker(fLock);

	for (ShapeMaskSet::iterator it = fShapeMasks.begin();
		it != fShapeMasks.end(); it++) {
		it->fMask->fInCache = false;
		it->fMask->fIndirectCacheReferences = 0;
		it->fMask->ReleaseReference();
	}
	fShapeMasks.clear();
	fTooLargeMaskCount = 0;
	fMasksReplacedCount = 0;
	fHitCount = 0;
	fMissCount = 0;
	fLowerMaskReferencedCount = 0;
}


size_t
AlphaMaskCache::_FindUncachedPreviousMasks(AlphaMask* mask, bool reference)
{
	const int32 referenceModifier = reference ? 1 : -1;
	size_t addedOrRemovedSize = 0;

	for (AlphaMask* lowerMask = mask->fPreviousMask.Get(); lowerMask != NULL;
		lowerMask = lowerMask->fPreviousMask.Get()) {
		if (lowerMask->fInCache)
			continue;
		uint32 oldReferences = lowerMask->fIndirectCacheReferences;
		lowerMask->fIndirectCacheReferences += referenceModifier;
		if (lowerMask->fIndirectCacheReferences == 0 || oldReferences == 0) {
			// We either newly referenced the mask for the first time, or
			// released the last reference
			addedOrRemovedSize += lowerMask->BitmapSize();
			fLowerMaskReferencedCount += referenceModifier;
		}
	}

	return addedOrRemovedSize;
}


void
AlphaMaskCache::_PrintAndResetStatistics()
{
	debug_printf("AlphaMaskCache statistics: size=%" B_PRIuSIZE " bytes=%"
		B_PRIuSIZE " lower=%4" B_PRIu32 " total=%" B_PRIuSIZE " too_large=%4"
		B_PRIu32 " replaced=%4" B_PRIu32 " hit=%4" B_PRIu32 " miss=%4" B_PRIu32
		"\n", fShapeMasks.size(), fCurrentCacheBytes, fLowerMaskReferencedCount,
		fShapeMasks.size() + fLowerMaskReferencedCount, fTooLargeMaskCount,
		fMasksReplacedCount, fHitCount, fMissCount);
	fTooLargeMaskCount = 0;
	fMasksReplacedCount = 0;
	fHitCount = 0;
	fMissCount = 0;
}
