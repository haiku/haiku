/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_RENDERER_H
#define ICON_RENDERER_H

#include "agg_gamma_lut.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_compound_aa.h"
#include "agg_scanline_u.h"
#include "agg_scanline_bin.h"
#include "agg_renderer_scanline.h"
#include "agg_span_allocator.h"
#include "agg_pixfmt_rgba.h"

class BBitmap;
class Icon;
class StyleManager;

typedef agg::gamma_lut
			<agg::int8u, agg::int8u>		GammaTable;

typedef agg::rendering_buffer				RenderingBuffer;
typedef agg::pixfmt_bgra32					PixelFormat;
typedef agg::pixfmt_bgra32_pre				PixelFormatPre;
typedef agg::renderer_base<PixelFormat>		BaseRenderer;
typedef agg::renderer_base<PixelFormatPre>	BaseRendererPre;

typedef agg::scanline_u8					Scanline;
typedef agg::scanline_bin					BinaryScanline;
typedef agg::span_allocator<agg::rgba8>		SpanAllocator;
typedef agg::rasterizer_compound_aa
			<agg::rasterizer_sl_clip_dbl>	CompoundRasterizer;

class IconRenderer {
 public:
								IconRenderer(BBitmap* bitmap);
	virtual						~IconRenderer();

			void				SetIcon(Icon* icon);

			void				Render();
			void				Render(const BRect& area);

 private:
			void				_Render(const BRect& area);

			BBitmap*			fBitmap;
			Icon*				fIcon;
			StyleManager*		fStyles;

			GammaTable			fGammaTable;

			RenderingBuffer		fRenderingBuffer;
			PixelFormat			fPixelFormat;
			PixelFormatPre		fPixelFormatPre;
			BaseRenderer		fBaseRenderer;
			BaseRendererPre		fBaseRendererPre;

			Scanline			fScanline;
			BinaryScanline		fBinaryScanline;
			SpanAllocator		fSpanAllocator;

			CompoundRasterizer	fRasterizer;
};

#endif // ICON_RENDERER_H
