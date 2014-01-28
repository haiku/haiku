/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef ALPHA_MASK_H
#define ALPHA_MASK_H


#include "agg_clipped_alpha_mask.h"
#include "ServerPicture.h"

#include "DrawState.h"
#include "drawing/Painter/defines.h"


class ServerBitmap;
class ServerPicture;


class AlphaMask {
public:
								AlphaMask(ServerPicture* mask, bool inverse,
									BPoint origin, const DrawState& drawState);
								~AlphaMask();

			void				Update(BRect bounds, BPoint offset);

			scanline_unpacked_masked_type* Generate();

private:
			ServerBitmap*		_RenderPicture() const;


private:
			ServerPicture*		fPicture;
			const bool			fInverse;
			BPoint				fOrigin;
			DrawState			fDrawState;

			BRect				fViewBounds;
			BPoint				fViewOffset;

			ServerBitmap*		fCachedBitmap;
			BRect				fCachedBounds;
			BPoint				fCachedOffset;

			agg::rendering_buffer fBuffer;
			agg::clipped_alpha_mask fCachedMask;
			scanline_unpacked_masked_type fScanline;
};


#endif // ALPHA_MASK_H
