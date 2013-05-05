/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconRenderer.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <List.h>

#include <agg_span_gradient.h>
#include <agg_span_interpolator_linear.h>

#include "GradientTransformable.h"
#include "Icon.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

class IconRenderer::StyleHandler {
	struct StyleItem {
		Style*			style;
		Transformation	transformation;
	};

 public:
	StyleHandler(::GammaTable& gammaTable)
		: fStyles(20),
		  fGammaTable(gammaTable),
		  fTransparent(0, 0, 0, 0),
		  fColor(0, 0, 0, 0)
	{}

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

		return styleItem->style->Gradient() == NULL;
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

	BList				fStyles;
	::GammaTable&		fGammaTable;
	agg::rgba8			fTransparent;
	agg::rgba8			fColor;
};

// color
const agg::rgba8&
IconRenderer::StyleHandler::color(unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem) {
		printf("no style at index: %u!\n", styleIndex);
		return fTransparent;
	}

	const rgb_color& c = styleItem->style->Color();
	fColor = agg::rgba8(fGammaTable.dir(c.red), fGammaTable.dir(c.green),
		fGammaTable.dir(c.blue), c.alpha);
	fColor.premultiply();
    return fColor;
}

// generate_span
void
IconRenderer::StyleHandler::generate_span(agg::rgba8* span, int x, int y,
	unsigned len, unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem || !styleItem->style->Gradient()) {
		printf("no style/gradient at index: %u!\n", styleIndex);
		// TODO: memset() span?
		return;
	}

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

// _GenerateGradient
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

// constructor
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

// destructor
IconRenderer::~IconRenderer()
{
}

// SetIcon
void
IconRenderer::SetIcon(const Icon* icon)
{
	if (fIcon == icon)
		return;

	fIcon = icon;
	// TODO: ... ?
}

// Render
void
IconRenderer::Render()
{
	_Render(fBitmap->Bounds());
}

// Render
void
IconRenderer::Render(const BRect& area)
{
	_Render(fBitmap->Bounds() & area);
}

//SetScale
void
IconRenderer::SetScale(double scale)
{
	fGlobalTransform.reset();
	fGlobalTransform.multiply(agg::trans_affine_scaling(scale));
}

//SetBackground
void
IconRenderer::SetBackground(const BBitmap* background)
{
	fBackground = background;
}

//SetBackground
void
IconRenderer::SetBackground(const agg::rgba8& background)
{
	fBackgroundColor.r = fGammaTable.dir(background.r);
	fBackgroundColor.g = fGammaTable.dir(background.g);
	fBackgroundColor.b = fGammaTable.dir(background.b);
	fBackgroundColor.a = background.a;
}

// Demultiply
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

// _Render
void
IconRenderer::_Render(const BRect& r)
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
	StyleHandler styleHandler(fGammaTable);

	fRasterizer.reset();
	// iterate over the shapes in the icon,
	// add the vector paths to the rasterizer
	// and associate each shapes style
	int32 shapeCount = fIcon->Shapes()->CountShapes();
	int32 styleIndex = 0;
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = fIcon->Shapes()->ShapeAtFast(i);

		// Don't render shape if the Level Of Detail falls out of range.
		// That's unless the scale is bigger than the maximum
		// MaxVisibilityScale of 4.0f.
		if (fGlobalTransform.scale() < shape->MinVisibilityScale()
			|| (fGlobalTransform.scale() > shape->MaxVisibilityScale()
				&& shape->MaxVisibilityScale() < 4.0f)) {
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
		if (i > 0 && style->HasTransparency())
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

// _CommitRenderPass
void
IconRenderer::_CommitRenderPass(StyleHandler& styleHandler, bool reset)
{
	agg::render_scanlines_compound(fRasterizer, fScanline, fBinaryScanline,
		fBaseRendererPre, fSpanAllocator, styleHandler);

	if (reset)
		fRasterizer.reset();
}




