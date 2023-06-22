/*
 * Copyright 2006-2007, 2023, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "IconRenderer.h"

#include <algorithm>
#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <List.h>

#include <agg_image_accessors.h>
#include <agg_span_image_filter_rgba.h>
#include <agg_span_gradient.h>
#include <agg_span_interpolator_linear.h>

#include "GradientTransformable.h"
#include "Icon.h"
#include "Shape.h"
#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

class IconRenderer::StyleHandler {
	struct StyleItem {
		Style*			style;
		Transformation	transformation;
	};

 public:
#ifdef ICON_O_MATIC
	StyleHandler(::GammaTable& gammaTable, bool showReferences)
		: fStyles(20),
		  fGammaTable(gammaTable),
		  fShowReferences(showReferences),
		  fTransparent(0, 0, 0, 0),
		  fColor(0, 0, 0, 0)
	{}
#else
	StyleHandler(::GammaTable& gammaTable)
		: fStyles(20),
		  fGammaTable(gammaTable),
		  fTransparent(0, 0, 0, 0),
		  fColor(0, 0, 0, 0)
	{}
#endif // ICON_O_MATIC

	~StyleHandler()
	{
		int32 count = fStyles.CountItems();
		for (int32 i = 0; i < count; i++)
			delete (StyleItem*)fStyles.ItemAtFast(i);
	}

	bool is_solid(unsigned styleIndex) const
	{
		StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
		if (!styleItem)
			return true;

		if (styleItem->style->Gradient())
			return false;

#ifdef ICON_O_MATIC
		if (styleItem->style->Bitmap() && fShowReferences)
			return false;
#endif // ICON_O_MATIC

		return true;
	}

	const agg::rgba8& color(unsigned styleIndex);

	void generate_span(agg::rgba8* span, int x, int y,
					   unsigned len, unsigned styleIndex);

	bool AddStyle(Style* style, const Transformation& transformation)
	{
		if (!style)
			return false;
		StyleItem* item = new (nothrow) StyleItem;
		if (!item)
			return false;
		item->style = style;
		// if the style uses a gradient, the transformation
		// is based on the gradient transformation
		if (Gradient* gradient = style->Gradient()) {
			item->transformation = *gradient;
			item->transformation.multiply(transformation);
		} else {
			item->transformation = transformation;
		}
		item->transformation.invert();
		return fStyles.AddItem((void*)item);
	}

private:
	template<class GradientFunction>
	void _GenerateGradient(agg::rgba8* span, int x, int y, unsigned len,
		GradientFunction function, int32 start, int32 end,
		const agg::rgba8* gradientColors, Transformation& gradientTransform);
#ifdef ICON_O_MATIC
	void _GenerateImage(agg::rgba8* span, int x, int y,
	   unsigned len, Style* style, Transformation& transform);
#endif

	BList				fStyles;
	::GammaTable&		fGammaTable;
#ifdef ICON_O_MATIC
	bool				fShowReferences;
#endif
	agg::rgba8			fTransparent;
	agg::rgba8			fColor;
};


const agg::rgba8&
IconRenderer::StyleHandler::color(unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem) {
		printf("no style at index: %u!\n", styleIndex);
		return fTransparent;
	}

#ifdef ICON_O_MATIC
	if (styleItem->style->Bitmap() && !fShowReferences) {
		fColor = agg::rgba8(0,0,0,0);
		return fColor;
	}
#endif

	const rgb_color& c = styleItem->style->Color();
	fColor = agg::rgba8(fGammaTable.dir(c.red), fGammaTable.dir(c.green),
		fGammaTable.dir(c.blue), c.alpha);
	fColor.premultiply();
    return fColor;
}


void
IconRenderer::StyleHandler::generate_span(agg::rgba8* span, int x, int y,
	unsigned len, unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem
			|| (!styleItem->style->Gradient()
#ifdef ICON_O_MATIC
				&& !styleItem->style->Bitmap()
#endif
		)) {
		printf("no style/gradient at index: %u!\n", styleIndex);
		// TODO: memset() span?
		return;
	}

#ifdef ICON_O_MATIC
	if (styleItem->style->Bitmap()) {
		_GenerateImage(span, x, y, len, styleItem->style, styleItem->transformation);
		return;
	}
#endif // ICON_O_MATIC

	Style* style = styleItem->style;
	Gradient* gradient = style->Gradient();
	const agg::rgba8* colors = style->GammaCorrectedColors(fGammaTable);

	switch (gradient->Type()) {
		case GRADIENT_LINEAR: {
		    agg::gradient_x function;
			_GenerateGradient(span, x, y, len, function, -64, 64, colors,
				styleItem->transformation);
			break;
		}
		case GRADIENT_CIRCULAR: {
		    agg::gradient_radial function;
			_GenerateGradient(span, x, y, len, function, 0, 64, colors,
				styleItem->transformation);
			break;
		}
		case GRADIENT_DIAMOND: {
		    agg::gradient_diamond function;
			_GenerateGradient(span, x, y, len, function, 0, 64, colors,
				styleItem->transformation);
			break;
		}
		case GRADIENT_CONIC: {
		    agg::gradient_conic function;
			_GenerateGradient(span, x, y, len, function, 0, 64, colors,
				styleItem->transformation);
			break;
		}
		case GRADIENT_XY: {
		    agg::gradient_xy function;
			_GenerateGradient(span, x, y, len, function, 0, 64, colors,
				styleItem->transformation);
			break;
		}
		case GRADIENT_SQRT_XY: {
		    agg::gradient_sqrt_xy function;
			_GenerateGradient(span, x, y, len, function, 0, 64, colors,
				styleItem->transformation);
			break;
		}
	}
}


template<class GradientFunction>
void
IconRenderer::StyleHandler::_GenerateGradient(agg::rgba8* span, int x, int y,
	unsigned len, GradientFunction function, int32 start, int32 end,
	const agg::rgba8* gradientColors, Transformation& gradientTransform)
{
	typedef agg::pod_auto_array<agg::rgba8, 256>	ColorArray;
	typedef agg::span_interpolator_linear<>			Interpolator;
	typedef agg::span_gradient<agg::rgba8,
							   Interpolator,
							   GradientFunction,
							   ColorArray>			GradientGenerator;

	Interpolator interpolator(gradientTransform);

	ColorArray array(gradientColors);
	GradientGenerator gradientGenerator(interpolator, function, array,
		start, end);

	gradientGenerator.generate(span, x, y, len);
}


#ifdef ICON_O_MATIC
void
IconRenderer::StyleHandler::_GenerateImage(agg::rgba8* span, int x, int y,
	unsigned len, Style* style, Transformation& transform)
{
	// bitmap
	BBitmap* bbitmap = style->Bitmap();
	agg::rendering_buffer bitmap;
	bitmap.attach(static_cast<unsigned char*>(bbitmap->Bits()), bbitmap->Bounds().Width() + 1,
		bbitmap->Bounds().Height() + 1, bbitmap->BytesPerRow());

	// pixel format attached to bitmap
	PixelFormat pixf_img(bitmap);

	// image interpolator
	typedef agg::span_interpolator_linear<> interpolator_type;
	interpolator_type interpolator(transform);

	// image accessor attached to pixel format of bitmap
	typedef agg::image_accessor_wrap<PixelFormat,
		agg::wrap_mode_repeat, agg::wrap_mode_repeat> source_type;
	source_type source(pixf_img);

	// image filter (nearest neighbor)
	typedef agg::span_image_filter_rgba_nn<
		source_type, interpolator_type> span_gen_type;
	span_gen_type spanGenerator(source, interpolator);

	// generate the requested span
	spanGenerator.generate(span, x, y, len);

	// apply postprocessing
	for (unsigned i = 0; i < len; i++) {
		span[i].apply_gamma_dir(fGammaTable);
		span[i].a = (uint8) ((float) span[i].a * style->Alpha() / 255);
		span[i].premultiply();
	}
}
#endif // ICON_O_MATIC


// #pragma mark -


class HintingTransformer {
 public:

	void transform(double* x, double* y) const
	{
		*x = floor(*x + 0.5);
		*y = floor(*y + 0.5);
	}
};


// #pragma mark -


IconRenderer::IconRenderer(BBitmap* bitmap)
	: fBitmap(bitmap),
	  fBackground(NULL),
	  fBackgroundColor(0, 0, 0, 0),
	  fIcon(NULL),

	  fGammaTable(2.2),

	  fRenderingBuffer(),
	  fPixelFormat(fRenderingBuffer),
	  fPixelFormatPre(fRenderingBuffer),
	  fBaseRenderer(fPixelFormat),
	  fBaseRendererPre(fPixelFormatPre),

	  fScanline(),
	  fBinaryScanline(),
	  fSpanAllocator(),

	  fRasterizer(),

	  fGlobalTransform()
{
	// attach rendering buffer to bitmap
	fRenderingBuffer.attach((uint8*)bitmap->Bits(),
		bitmap->Bounds().IntegerWidth() + 1,
		bitmap->Bounds().IntegerHeight() + 1, bitmap->BytesPerRow());

	fBaseRendererPre.clip_box(0, 0, bitmap->Bounds().IntegerWidth(),
		bitmap->Bounds().IntegerHeight());
}


IconRenderer::~IconRenderer()
{
}


void
IconRenderer::SetIcon(const Icon* icon)
{
	if (fIcon == icon)
		return;

	fIcon = icon;
	// TODO: ... ?
}


void
#ifdef ICON_O_MATIC
IconRenderer::Render(bool showReferences)
#else
IconRenderer::Render()
#endif
{
#ifdef ICON_O_MATIC
	_Render(fBitmap->Bounds(), showReferences);
#else
	_Render(fBitmap->Bounds());
#endif
}


void
#ifdef ICON_O_MATIC
IconRenderer::Render(const BRect& area, bool showReferences)
#else
IconRenderer::Render(const BRect& area)
#endif
{
#ifdef ICON_O_MATIC
	_Render(fBitmap->Bounds() & area, showReferences);
#else
	_Render(fBitmap->Bounds() & area);
#endif
}


void
IconRenderer::SetScale(double scale)
{
	fGlobalTransform.reset();
	fGlobalTransform.multiply(agg::trans_affine_scaling(scale));
}


void
IconRenderer::SetBackground(const BBitmap* background)
{
	fBackground = background;
}


void
IconRenderer::SetBackground(const agg::rgba8& background)
{
	fBackgroundColor.r = fGammaTable.dir(background.r);
	fBackgroundColor.g = fGammaTable.dir(background.g);
	fBackgroundColor.b = fGammaTable.dir(background.b);
	fBackgroundColor.a = background.a;
}


void
IconRenderer::Demultiply()
{
	uint8* bits = (uint8*)fBitmap->Bits();
	uint32 bpr = fBitmap->BytesPerRow();
	uint32 width = fBitmap->Bounds().IntegerWidth() + 1;
	uint32 height = fBitmap->Bounds().IntegerHeight() + 1;

	for (uint32 y = 0; y < height; y++) {
		uint8* b = bits;
		for (uint32 x = 0; x < width; x++) {
			if (b[3] < 255 && b[3] > 0) {
				b[0] = (uint8)((int)b[0] * 255 / b[3]);
				b[1] = (uint8)((int)b[1] * 255 / b[3]);
				b[2] = (uint8)((int)b[2] * 255 / b[3]);
			}
			b += 4;
		}
		bits += bpr;
	}
}


// #pragma mark -


typedef agg::conv_transform<VertexSource, Transformation> ScaledPath;
typedef agg::conv_transform<ScaledPath, HintingTransformer> HintedPath;


void
#ifdef ICON_O_MATIC
IconRenderer::_Render(const BRect& r, bool showReferences)
#else
IconRenderer::_Render(const BRect& r)
#endif
{
	if (!fIcon)
		return;

// TODO: fix clip box for "clear" and "apply_gamma_inv"
//	fBaseRendererPre.clip_box((int)floorf(r.left), (int)floorf(r.top),
//		(int)ceilf(r.right), (int)ceilf(r.bottom));

	if (fBackground)
		memcpy(fBitmap->Bits(), fBackground->Bits(), fBitmap->BitsLength());
	else
		fBaseRendererPre.clear(fBackgroundColor);

//bigtime_t start = system_time();
#ifdef ICON_O_MATIC
	StyleHandler styleHandler(fGammaTable, showReferences);
#else
	StyleHandler styleHandler(fGammaTable);
#endif

	fRasterizer.reset();
	// iterate over the shapes in the icon,
	// add the vector paths to the rasterizer
	// and associate each shapes style
	int32 shapeCount = fIcon->Shapes()->CountItems();
	int32 styleIndex = 0;
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = fIcon->Shapes()->ItemAtFast(i);

		if (!shape->Visible(fGlobalTransform.scale())) {
			continue;
		}

		Transformation transform(*shape);
		transform.multiply(fGlobalTransform);
			// NOTE: this works only because "agg::trans_affine",
			// "Transformable" and "Transformation" are all the
			// same thing

		Style* style = shape->Style();
		if (!style)
			continue;

		// add the style either with global transformation or with
		// the shapes transformation, depending on whether there
		// is a gradient and its settings
		Gradient* gradient = style->Gradient();
		bool styleAdded = false;
		if (gradient && !gradient->InheritTransformation()) {
			styleAdded = styleHandler.AddStyle(shape->Style(),
				fGlobalTransform);
		} else {
			styleAdded = styleHandler.AddStyle(shape->Style(), transform);
		}

		if (!styleAdded) {
			printf("IconRenderer::_Render() - out of memory\n");
			break;
		}

		// if this is not the first shape, and the style contains
		// transparency, commit a render pass of previous shapes
		if (i > 0
				&& (style->HasTransparency()
#ifdef ICON_O_MATIC
					|| style->Bitmap() != NULL
#endif
			))
			_CommitRenderPass(styleHandler);

		fRasterizer.styles(styleIndex, -1);
		styleIndex++;

		// global scale
		shape->SetGlobalScale(max_c(1.0, transform.scale()));
		ScaledPath scaledPath(shape->VertexSource(), transform);
		if (shape->Hinting()) {
			// additional hinting
			HintingTransformer hinter;
			HintedPath hintedPath(scaledPath, hinter);
			fRasterizer.add_path(hintedPath);
		} else {
			fRasterizer.add_path(scaledPath);
		}
	}

	_CommitRenderPass(styleHandler, false);

	if (fGammaTable.gamma() != 1.0)
		fPixelFormat.apply_gamma_inv(fGammaTable);

//if (fRenderingBuffer.width() == 64)
//printf("rendering 64x64: %lld\n", system_time() - start);
}


void
IconRenderer::_CommitRenderPass(StyleHandler& styleHandler, bool reset)
{
	agg::render_scanlines_compound(fRasterizer, fScanline, fBinaryScanline,
		fBaseRendererPre, fSpanAllocator, styleHandler);

	if (reset)
		fRasterizer.reset();
}

