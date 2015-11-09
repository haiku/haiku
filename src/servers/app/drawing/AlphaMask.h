/*
 * Copyright 2014-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef ALPHA_MASK_H
#define ALPHA_MASK_H

#include <Referenceable.h>

#include "agg_clipped_alpha_mask.h"
#include "ServerPicture.h"

#include "DrawState.h"
#include "drawing/Painter/defines.h"
#include "IntRect.h"


class BShape;
class ServerBitmap;
class ServerPicture;
class shape_data;


// #pragma mark - AlphaMask


class AlphaMask : public BReferenceable {
public:
								AlphaMask(AlphaMask* previousMask,
									bool inverse);
								AlphaMask(uint8 backgroundOpacity);
	virtual						~AlphaMask();

			IntPoint			SetViewOrigin(IntPoint viewOrigin);

			scanline_unpacked_masked_type* Scanline()
								{ return &fScanline; }

			agg::clipped_alpha_mask* Mask()
								{ return &fMask; }

protected:
			ServerBitmap*		_CreateTemporaryBitmap(BRect bounds) const;
			void				_Generate();
			void				_SetNoClipping();

private:
	virtual	ServerBitmap*		_RenderSource() = 0;
 	virtual	IntPoint			_Offset() = 0;

			void				_AttachMaskToBuffer();

public:
			BReference<AlphaMask> fPreviousMask;
			IntRect				fBounds;

private:
			IntPoint			fViewOrigin;
			const bool			fInverse;
			uint8				fBackgroundOpacity;

			uint8*				fBits;
			agg::rendering_buffer fBuffer;
			agg::clipped_alpha_mask fMask;
			scanline_unpacked_masked_type fScanline;
};


class UniformAlphaMask : public AlphaMask {
public:
								UniformAlphaMask(uint8 opacity);

private:
	virtual	ServerBitmap*		_RenderSource();
	virtual	IntPoint			_Offset();
};


// #pragma mark - VectorAlphaMask


template<class VectorMaskType>
class VectorAlphaMask : public AlphaMask {
public:
								VectorAlphaMask(AlphaMask* previousMask,
									BPoint where, bool inverse);

private:
	virtual	ServerBitmap*		_RenderSource();
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
			BReference<ServerPicture> fPicture;
			DrawState*			fDrawState;
};


// #pragma mark - ShapeAlphaMask


class ShapeAlphaMask : public VectorAlphaMask<ShapeAlphaMask> {
public:
								ShapeAlphaMask(AlphaMask* previousMask,
									const shape_data& shape,
									BPoint where, bool inverse);

			void				DrawVectors(Canvas* canvas);
			BRect				DetermineBoundingBox() const;
			const DrawState&	GetDrawState() const;

private:
			const shape_data&	fShape;
			BRect				fBounds;
			DrawState			fDrawState;
};


#endif // ALPHA_MASK_H
