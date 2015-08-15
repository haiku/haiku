/*
 * Copyright 2005-2007, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * Copyright 2015, Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PAINTER_DATA_H
#define PAINTER_DATA_H


#include "defines.h"

#include <agg_path_storage.h>


struct PainterAggInterface {
	PainterAggInterface(PatternHandler& patternHandler)
		:
		fBuffer(),
		fPixelFormat(fBuffer, &patternHandler),
		fBaseRenderer(fPixelFormat),
		fUnpackedScanline(),
		fPackedScanline(),
		fRasterizer(),
		fRenderer(fBaseRenderer),
		fRendererBin(fBaseRenderer),
		fSubpixPackedScanline(),
		fSubpixUnpackedScanline(),
		fSubpixRasterizer(),
		fSubpixRenderer(fBaseRenderer),
		fMaskedUnpackedScanline(NULL),
		fClippedAlphaMask(NULL),
		fPath(),
		fCurve(fPath)
	{
	}

	agg::rendering_buffer	fBuffer;

	// AGG rendering and rasterization classes
	pixfmt					fPixelFormat;
	renderer_base			fBaseRenderer;

	// Regular drawing mode: pixel-aligned, no alpha masking
	scanline_unpacked_type	fUnpackedScanline;
	scanline_packed_type	fPackedScanline;
	rasterizer_type			fRasterizer;
	renderer_type			fRenderer;

	// Fast mode: no antialiasing needed (horizontal/vertical lines, ...)
	renderer_bin_type		fRendererBin;

	// Subpixel mode
	scanline_packed_subpix_type fSubpixPackedScanline;
	scanline_unpacked_subpix_type fSubpixUnpackedScanline;
	rasterizer_subpix_type	fSubpixRasterizer;
	renderer_subpix_type	fSubpixRenderer;

	// Alpha-Masked mode: for ClipToPicture
	// (this uses the standard rasterizer and renderer)
	scanline_unpacked_masked_type* fMaskedUnpackedScanline;
	agg::clipped_alpha_mask* fClippedAlphaMask;

	agg::path_storage		fPath;
	agg::conv_curve<agg::path_storage> fCurve;
};


#endif // PAINTER_DATA_H
