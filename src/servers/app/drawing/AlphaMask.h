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


class ServerBitmap;
class ServerPicture;


class AlphaMask : public BReferenceable {
public:
								AlphaMask(ServerPicture* mask, bool inverse,
									BPoint origin, const DrawState& drawState);
								AlphaMask(uint8 backgroundOpacity);
								~AlphaMask();

			void				Update(BRect bounds, BPoint offset);

			void				SetPrevious(AlphaMask* mask);

			scanline_unpacked_masked_type* Generate();

private:
			ServerBitmap*		_RenderPicture() const;
			void				_AttachMaskToBuffer();


private:
			AlphaMask*			fPreviousMask;

			ServerPicture*		fPicture;
			const bool			fInverse;
			BPoint				fOrigin;
									// position of this mask, relative to
									// either its parent mask, or (if there
									// is none) the canvas
			uint8				fBackgroundOpacity;
			DrawState			fDrawState;
									// draw state used for drawing fPicture

			BRect				fViewBounds;
									// determines alpha mask size
			BPoint				fViewOffset;
									// position of alpha mask in screen
									// coordinates

			uint8*				fCachedBitmap;
			BRect				fCachedBounds;
			BPoint				fCachedOffset;

			agg::rendering_buffer fBuffer;
			agg::clipped_alpha_mask fCachedMask;
			scanline_unpacked_masked_type fScanline;
};


#endif // ALPHA_MASK_H
