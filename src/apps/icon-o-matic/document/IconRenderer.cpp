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
 public:
	StyleHandler(GammaTable& gammaTable)
		: fStyles(20),
		  fGammaTable(gammaTable),
		  fTransparent(0, 0, 0, 0),
		  fColor(0, 0, 0, 0)
	{}

	bool is_solid(unsigned styleIndex) const
	{
		Style* style = (Style*)fStyles.ItemAt(styleIndex);
		if (!style)
			return true;

		return style->Gradient() == NULL;
	}

	const agg::rgba8& color(unsigned styleIndex);

	void generate_span(agg::rgba8* span, int x, int y,
					   unsigned len, unsigned styleIndex);

	bool AddStyle(Style* style)
	{
		return fStyles.AddItem((void*)style);
	}

private:
	template<class GradientFunction>
	void _GenerateGradient(agg::rgba8* span, int x, int y, unsigned len,
						   GradientFunction function,
						   const agg::rgba8* gradientColors,
						   agg::trans_affine& gradientTransform);

	BList				fStyles;
	GammaTable&			fGammaTable;
	agg::rgba8			fTransparent;
	agg::rgba8			fColor;
};

// color
const agg::rgba8&
StyleHandler::color(unsigned styleIndex)
{
	Style* style = (Style*)fStyles.ItemAt(styleIndex);
	if (!style) {
		printf("no style at index: %d!\n", styleIndex);
		return fTransparent;
	}

	const rgb_color& c = style->Color();
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
	Style* style = (Style*)fStyles.ItemAt(styleIndex);
	if (!style || !style->Gradient()) {
		printf("no style/gradient at index: %d!\n", styleIndex);
		// TODO: memset() span?
		return;
	}

	Gradient* gradient = style->Gradient();		

	// TODO: move filling of color array elsewhere and cache result
	// maybe in Style?
	agg::rgba8 colors[256];
	gradient->MakeGradient((uint32*)colors, 256);

	agg::trans_affine transformation;
		// TODO: construct the gradient transformation here
		// remember to invert() it!

	switch (gradient->Type()) {
		case GRADIENT_LINEAR: {
		    agg::gradient_x function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
			break;
		}
		case GRADIENT_CIRCULAR: {
		    agg::gradient_radial function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
			break;
		}
		case GRADIENT_DIAMONT: {
		    agg::gradient_diamond function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
			break;
		}
		case GRADIENT_CONIC: {
		    agg::gradient_conic function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
			break;
		}
		case GRADIENT_XY: {
		    agg::gradient_xy function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
			break;
		}
		case GRADIENT_SQRT_XY: {
		    agg::gradient_sqrt_xy function;
			_GenerateGradient(span, x, y, len, function, colors,
							  transformation);
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
								agg::trans_affine& gradientTransform)

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

	StyleHandler styleHandler(fGammaTable);

	fRasterizer.reset();
	// iterate over the shapes in the icon,
	// add the vector paths to the rasterizer
	// and associate each shapes style
	int32 shapeCount = fIcon->Shapes()->CountShapes();
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = fIcon->Shapes()->ShapeAtFast(i);

		if (!styleHandler.AddStyle(shape->Style())) {
			printf("IconRenderer::_Render() - out of memory\n");
			break;
		}
		fRasterizer.styles(i, -1);

		if (fGlobalTransform.is_identity()) {
			fRasterizer.add_path(shape->VertexSource());
		} else {
			agg::conv_transform<VertexSource, Transformation>
				scaledPath(shape->VertexSource(), fGlobalTransform);
			fRasterizer.add_path(scaledPath);
		}
	}

	agg::render_scanlines_compound(fRasterizer,
								   fScanline,
								   fBinaryScanline,
								   fBaseRendererPre,
								   fSpanAllocator,
								   styleHandler);

	if (fGammaTable.gamma() != 1.0)
		fPixelFormat.apply_gamma_inv(fGammaTable);
}

