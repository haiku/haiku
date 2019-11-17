/*
 * Copyright 2009, Christian Packmann.
 * Copyright 2008, Andrej Spielmann <andrej.spielmann@seh.ox.ac.uk>.
 * Copyright 2005-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2015, Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DRAW_BITMAP_GENERIC_H
#define DRAW_BITMAP_GENERIC_H

#include "Painter.h"


struct Fill {};
struct Tile {};

template<typename PixFmt, typename Mode>
struct ImageAccessor {};

template<typename PixFmt>
struct ImageAccessor<PixFmt, Fill> {
	typedef agg::image_accessor_clone<PixFmt> type;
};

template<typename PixFmt>
struct ImageAccessor<PixFmt, Tile> {
	typedef agg::image_accessor_wrap<PixFmt,
		agg::wrap_mode_repeat, agg::wrap_mode_repeat> type;
};


template<typename FillMode>
struct DrawBitmapGeneric {
	static void
	Draw(const Painter* painter, PainterAggInterface& aggInterface,
		agg::rendering_buffer& bitmap, BPoint offset,
		double scaleX, double scaleY, BRect destinationRect, uint32 options)
	{
		// pixel format attached to bitmap
		typedef agg::pixfmt_bgra32 pixfmt_image;
		pixfmt_image pixf_img(bitmap);

		agg::trans_affine srcMatrix;
		// NOTE: R5 seems to ignore this offset when drawing bitmaps
		//	srcMatrix *= agg::trans_affine_translation(-actualBitmapRect.left,
		//		-actualBitmapRect.top);
		srcMatrix *= painter->Transform();

		agg::trans_affine imgMatrix;
		imgMatrix *= agg::trans_affine_translation(
			offset.x - destinationRect.left, offset.y - destinationRect.top);
		imgMatrix *= agg::trans_affine_scaling(scaleX, scaleY);
		imgMatrix *= agg::trans_affine_translation(destinationRect.left,
			destinationRect.top);
		imgMatrix *= painter->Transform();
		imgMatrix.invert();

		// image interpolator
		typedef agg::span_interpolator_linear<> interpolator_type;
		interpolator_type interpolator(imgMatrix);

		// scanline allocator
		agg::span_allocator<pixfmt_image::color_type> spanAllocator;

		// image accessor attached to pixel format of bitmap
		typedef
			typename ImageAccessor<pixfmt_image, FillMode>::type source_type;
		source_type source(pixf_img);

		// clip to the current clipping region's frame
		if (painter->IsIdentityTransform()) {
			destinationRect = destinationRect
				& painter->ClippingRegion()->Frame();
		}
		// convert to pixel coords (versus pixel indices)
		destinationRect.right++;
		destinationRect.bottom++;

		// path enclosing the bitmap
		agg::path_storage& path = aggInterface.fPath;
		rasterizer_type& rasterizer = aggInterface.fRasterizer;

		path.remove_all();
		path.move_to(destinationRect.left, destinationRect.top);
		path.line_to(destinationRect.right, destinationRect.top);
		path.line_to(destinationRect.right, destinationRect.bottom);
		path.line_to(destinationRect.left, destinationRect.bottom);
		path.close_polygon();

		agg::conv_transform<agg::path_storage> transformedPath(path,
			srcMatrix);
		rasterizer.reset();
		rasterizer.add_path(transformedPath);

		if ((options & B_FILTER_BITMAP_BILINEAR) != 0) {
			// image filter (bilinear)
			typedef agg::span_image_filter_rgba_bilinear<
				source_type, interpolator_type> span_gen_type;
			span_gen_type spanGenerator(source, interpolator);

			// render the path with the bitmap as scanline fill
			if (aggInterface.fMaskedUnpackedScanline != NULL) {
				agg::render_scanlines_aa(rasterizer,
					*aggInterface.fMaskedUnpackedScanline,
					aggInterface.fBaseRenderer, spanAllocator, spanGenerator);
			} else {
				agg::render_scanlines_aa(rasterizer,
					aggInterface.fUnpackedScanline,
					aggInterface.fBaseRenderer, spanAllocator, spanGenerator);
			}
		} else {
			// image filter (nearest neighbor)
			typedef agg::span_image_filter_rgba_nn<
				source_type, interpolator_type> span_gen_type;
			span_gen_type spanGenerator(source, interpolator);

			// render the path with the bitmap as scanline fill
			if (aggInterface.fMaskedUnpackedScanline != NULL) {
				agg::render_scanlines_aa(rasterizer,
					*aggInterface.fMaskedUnpackedScanline,
					aggInterface.fBaseRenderer, spanAllocator, spanGenerator);
			} else {
				agg::render_scanlines_aa(rasterizer,
					aggInterface.fUnpackedScanline,
					aggInterface.fBaseRenderer, spanAllocator, spanGenerator);
			}
		}
	}
};


#endif // DRAW_BITMAP_GENERIC_H
