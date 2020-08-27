/*
 * Copyright 2014-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef ALPHA_MASK_H
#define ALPHA_MASK_H

#include <Referenceable.h>
#include <locks.h>

#include "agg_clipped_alpha_mask.h"
#include "ServerPicture.h"

#include "DrawState.h"
#include "drawing/Painter/defines.h"
#include "IntRect.h"


class BShape;
class ServerBitmap;
class ServerPicture;
class shape_data;
class UtilityBitmap;


// #pragma mark - AlphaMask


class AlphaMask : public BReferenceable {
public:
								AlphaMask(AlphaMask* previousMask,
									bool inverse);
								AlphaMask(AlphaMask* previousMask,
									AlphaMask* other);
								AlphaMask(uint8 backgroundOpacity);
	virtual						~AlphaMask();

			IntPoint			SetCanvasGeometry(IntPoint origin,
									IntRect bounds);

			scanline_unpacked_masked_type* Scanline()
								{ return &fScanline; }

			agg::clipped_alpha_mask* Mask()
								{ return &fMask; }

			size_t				BitmapSize() const;

protected:
			ServerBitmap*		_CreateTemporaryBitmap(BRect bounds) const;
			void				_Generate();
			void				_SetNoClipping();
			const IntRect&		_PreviousMaskBounds() const;
	virtual	void				_AddToCache() = 0;

private:
	virtual	ServerBitmap*		_RenderSource(const IntRect& canvasBounds) = 0;
 	virtual	IntPoint			_Offset() = 0;

			void				_AttachMaskToBuffer();

protected:
			BReference<AlphaMask> fPreviousMask;
			IntRect				fBounds;
			bool				fClippedToCanvas;
			recursive_lock		fLock;

private:
	friend class AlphaMaskCache;

			IntPoint			fCanvasOrigin;
			IntRect				fCanvasBounds;
			const bool			fInverse;
			uint8				fBackgroundOpacity;

			int32				fNextMaskCount;
			bool				fInCache;
			uint32				fIndirectCacheReferences;
									// number of times this mask has been
									// seen as "previous mask" of another
									// one in the cache, without being
									// in the cache itself

			BReference<UtilityBitmap> fBits;
			agg::rendering_buffer fBuffer;
			agg::clipped_alpha_mask fMask;
			scanline_unpacked_masked_type fScanline;
};


class UniformAlphaMask : public AlphaMask {
public:
								UniformAlphaMask(uint8 opacity);

private:
	virtual	ServerBitmap*		_RenderSource(const IntRect& canvasBounds);
	virtual	IntPoint			_Offset();
	virtual void				_AddToCache();
};


// #pragma mark - VectorAlphaMask


template<class VectorMaskType>
class VectorAlphaMask : public AlphaMask {
public:
								VectorAlphaMask(AlphaMask* previousMask,
									BPoint where, bool inverse);
								VectorAlphaMask(AlphaMask* previousMask,
									VectorAlphaMask* other);

private:
	virtual	ServerBitmap*		_RenderSource(const IntRect& canvasBounds);
	virtual	IntPoint			_Offset();

protected:
			BPoint				fWhere;
};


// #pragma mark - PictureAlphaMask


class PictureAlphaMask : public VectorAlphaMask<PictureAlphaMask> {
public:
								PictureAlphaMask(AlphaMask* previousMask,
									ServerPicture* picture,
									const DrawState& drawState, BPoint where,
									bool inverse);
	virtual						~PictureAlphaMask();

			void				DrawVectors(Canvas* canvas);
			BRect				DetermineBoundingBox() const;
			const DrawState&	GetDrawState() const;

private:
	virtual void				_AddToCache();

private:
			BReference<ServerPicture> fPicture;
			ObjectDeleter<DrawState> fDrawState;
};


// #pragma mark - ShapeAlphaMask


class ShapeAlphaMask : public VectorAlphaMask<ShapeAlphaMask> {
private:
								ShapeAlphaMask(AlphaMask* previousMask,
									const shape_data& shape,
									BPoint where, bool inverse);
								ShapeAlphaMask(AlphaMask* previousMask,
									ShapeAlphaMask* other);

public:
	virtual						~ShapeAlphaMask();

	static	ShapeAlphaMask*		Create(AlphaMask* previousMask,
									const shape_data& shape,
									BPoint where, bool inverse);

			void				DrawVectors(Canvas* canvas);
			BRect				DetermineBoundingBox() const;
			const DrawState&	GetDrawState() const;

private:
	virtual void				_AddToCache();

private:
	friend class AlphaMaskCache;

			BReference<shape_data> fShape;
			BRect				fShapeBounds;
	static	DrawState*			fDrawState;
};


#endif // ALPHA_MASK_H
