/*
 * Copyright 2006, Haiku.
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

#include "agg_span_gradient.h"
#include "agg_span_interpolator_linear.h"

#include "Gradient.h"
#include "Icon.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

class StyleHandler {
	struct StyleItem {
		Style*			style;
		Transformation	transformation;
	};

 public:
	StyleHandler(GammaTable& gammaTable)
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
		item->transformation = transformation;
		// TODO: if the style uses a gradient and the gradient
		// should be transformed along with the shape, then
		// we should multiply the item->transformation with the
		// gradient transformation
		item->transformation.invert();
		return fStyles.AddItem((void*)item);
	}

private:
	template<class GradientFunction>
	void _GenerateGradient(agg::rgba8* span, int x, int y, unsigned len,
						   GradientFunction function,
						   const agg::rgba8* gradientColors,
						   Transformation& gradientTransform);

	BList				fStyles;
	GammaTable&			fGammaTable;
	agg::rgba8			fTransparent;
	agg::rgba8			fColor;
};

// color
const agg::rgba8&
StyleHandler::color(unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem) {
		printf("no style at index: %d!\n", styleIndex);
		return fTransparent;
	}

	const rgb_color& c = styleItem->style->Color();
	fColor = agg::rgba8(fGammaTable.dir(c.red),
						fGammaTable.dir(c.green),
						fGammaTable.dir(c.blue),
						c.alpha);
	fColor.premultiply();
    return fColor;
}

// generate_span
void
StyleHandler::generate_span(agg::rgba8* span, int x, int y,
							unsigned len, unsigned styleIndex)
{
	StyleItem* styleItem = (StyleItem*)fStyles.ItemAt(styleIndex);
	if (!styleItem || !styleItem->style->Gradient()) {
		printf("no style/gradient at index: %d!\n", styleIndex);
		// TODO: memset() span?
		return;
	}

	Style* style = styleItem->style;
	Gradient* gradient = style->Gradient();		
	const agg::rgba8* colors = style->Colors();

	switch (gradient->Type()) {
		case GRADIENT_LINEAR: {
		    agg::gradient_x function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
		case GRADIENT_CIRCULAR: {
		    agg::gradient_radial function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
		case GRADIENT_DIAMONT: {
		    agg::gradient_diamond function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
		case GRADIENT_CONIC: {
		    agg::gradient_conic function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
		case GRADIENT_XY: {
		    agg::gradient_xy function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
		case GRADIENT_SQRT_XY: {
		    agg::gradient_sqrt_xy function;
			_GenerateGradient(span, x, y, len, function, colors,
							  styleItem->transformation);
			break;
		}
	}
}

// _GenerateGradient
template<class GradientFunction>
void
StyleHandler::_GenerateGradient(agg::rgba8* span, int x, int y, unsigned len,
								GradientFunction function,
								const agg::rgba8* gradientColors,
								Transformation& gradientTransform)

{
	typedef agg::pod_auto_array<agg::rgba8, 256>	ColorArray;
	typedef agg::span_interpolator_linear<>			Interpolator;
	typedef agg::span_gradient<agg::rgba8, 
							   Interpolator, 
							   GradientFunction,
							   ColorArray>			GradientGenerator;

	Interpolator interpolator(gradientTransform);

	ColorArray array(gradientColors);
	GradientGenerator gradientGenerator(interpolator,
										function,
										array,
										0, 100);

	gradientGenerator.generate(span, x, y, len);
}

// #pragma mark -

// constructor
IconRenderer::IconRenderer(BBitmap* bitmap)
	: fBitmap(bitmap),
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
							bitmap->Bounds().IntegerHeight() + 1,
							bitmap->BytesPerRow());

	fBaseRendererPre.clip_box(0,
							  0,
							  fBitmap->Bounds().IntegerWidth(),
							  fBitmap->Bounds().IntegerHeight());
}

// destructor
IconRenderer::~IconRenderer()
{
}

// SetIcon
void
IconRenderer::SetIcon(Icon* icon)
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

// #pragma mark -

// _Render
void
IconRenderer::_Render(const BRect& r)
{
	if (!fIcon)
		return;

// TODO: fix clip box for "clear" and "apply_gamma_inv"
//	fBaseRendererPre.clip_box((int)floorf(r.left),
//							  (int)floorf(r.top),
//							  (int)ceilf(r.right),
//							  (int)ceilf(r.bottom));

	fBaseRendererPre.clear(agg::rgba8(0, 0, 0, 0));

//bigtime_t start = system_time();
	StyleHandler styleHandler(fGammaTable);

	fRasterizer.reset();
	// iterate over the shapes in the icon,
	// add the vector paths to the rasterizer
	// and associate each shapes style
	int32 shapeCount = fIcon->Shapes()->CountShapes();
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = fIcon->Shapes()->ShapeAtFast(i);

		Transformation transform(*shape);
		transform.multiply(fGlobalTransform);
			// NOTE: this works only because "agg::trans_affine",
			// "Transformable" and "Transformation" are all the
			// same thing

		if (!styleHandler.AddStyle(shape->Style(), transform)) {
			printf("IconRenderer::_Render() - out of memory\n");
			break;
		}
		fRasterizer.styles(i, -1);

		agg::conv_transform<VertexSource, Transformation>
			scaledPath(shape->VertexSource(), transform);
		fRasterizer.add_path(scaledPath);
	}

	agg::render_scanlines_compound(fRasterizer,
								   fScanline,
								   fBinaryScanline,
								   fBaseRendererPre,
								   fSpanAllocator,
								   styleHandler);

	if (fGammaTable.gamma() != 1.0)
		fPixelFormat.apply_gamma_inv(fGammaTable);

//if (fRenderingBuffer.width() == 64)
//printf("rendering 64x64: %lld\n", system_time() - start);
}

