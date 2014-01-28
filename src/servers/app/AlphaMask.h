/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef ALPHA_MASK_H
#define ALPHA_MASK_H


#include "agg_clipped_alpha_mask.h"
#include "ServerBitmap.h"
#include "ServerPicture.h"

#include "drawing/Painter/defines.h"


class ServerBitmap;


class AlphaMask
{
	public:
										AlphaMask(View& view,
											ServerPicture& mask, bool inverse,
											BPoint origin);
										~AlphaMask();
		scanline_unpacked_masked_type*	Generate();

	private:
		ServerPicture&					fPicture;
		const bool						fInverse;
		const BPoint					fOrigin;
		View&							fView;

		ServerBitmap*					fCachedBitmap;
		agg::rendering_buffer			fBuffer;
		agg::clipped_alpha_mask			fCachedMask;
		scanline_unpacked_masked_type	fScanline;
};


#endif
