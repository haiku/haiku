/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef ICON_RENDERER_H
#define ICON_RENDERER_H


#include <agg_gamma_lut.h>
#include <agg_pixfmt_rgba.h>
#include <agg_rasterizer_compound_aa.h>
#include <agg_rendering_buffer.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_bin.h>
#include <agg_scanline_u.h>
#include <agg_span_allocator.h>
#include <agg_trans_affine.h>

#include "IconBuild.h"
#include "Transformable.h"


class BBitmap;
class BRect;


_BEGIN_ICON_NAMESPACE


class Icon;

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

			void				SetIcon(const Icon* icon);

#ifdef ICON_O_MATIC
			void				Render(bool showReferences);
			void				Render(const BRect& area, bool showReferences);
#else
			void				Render();
			void				Render(const BRect& area);
#endif // ICON_O_MATIC

			void				SetScale(double scale);
			void				SetBackground(const BBitmap* background);
									// background is not copied,
									// ownership stays with the caller
									// colorspace and size need to
									// be the same as bitmap passed
									// to constructor
			void				SetBackground(const agg::rgba8& color);
									// used when no background bitmap
									// is set

			const _ICON_NAMESPACE GammaTable& GammaTable() const
									{ return fGammaTable; }

			void				Demultiply();

 private:
		class StyleHandler;

#ifdef ICON_O_MATIC
			void				_Render(const BRect& area, bool showReferences);
#else
			void				_Render(const BRect& area);
#endif // ICON_O_MATIC
			void				_CommitRenderPass(StyleHandler& styleHandler,
									bool reset = true);

			BBitmap*			fBitmap;
			const BBitmap*		fBackground;
			agg::rgba8			fBackgroundColor;
			const Icon*			fIcon;

			_ICON_NAMESPACE GammaTable fGammaTable;

			RenderingBuffer		fRenderingBuffer;
			PixelFormat			fPixelFormat;
			PixelFormatPre		fPixelFormatPre;
			BaseRenderer		fBaseRenderer;
			BaseRendererPre		fBaseRendererPre;

			Scanline			fScanline;
			BinaryScanline		fBinaryScanline;
			SpanAllocator		fSpanAllocator;

			CompoundRasterizer	fRasterizer;

			Transformable		fGlobalTransform;
};


_END_ICON_NAMESPACE


#endif // ICON_RENDERER_H
