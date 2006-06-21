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

#include "agg_conv_curve.h"

#include "Icon.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Style.h"
#include "StyleManager.h"
#include "VectorPath.h"

using std::nothrow;

class StyleHandler {
 public:
	StyleHandler(const StyleManager* styles,
				 GammaTable& gammaTable)
		: fStyles(styles),
		  fGammaTable(gammaTable),
		  fTransparent(0, 0, 0, 0),
		  fColor(0, 0, 0, 0)
	{}

	bool is_solid(unsigned styleIndex) const
	{
		Style* style = fStyles->StyleAt(styleIndex);
		if (!style)
			return true;

		return style->Gradient() == NULL;
	}

	const agg::rgba8& color(unsigned styleIndex)
    {
		Style* style = fStyles->StyleAt(styleIndex);
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

	void generate_span(agg::rgba8* span, int x, int y,
					   unsigned len, unsigned styleIndex)
	{
printf("unimplemented: generate_span(%p, %d, %d, %d)\n", span, x, y, len);
		// TODO: render gradient of Style at styleIndex into span
	}

private:
	const StyleManager*	fStyles;
	GammaTable&			fGammaTable;
	agg::rgba8			fTransparent;
	agg::rgba8			fColor;
};


// constructor
IconRenderer::IconRenderer(BBitmap* bitmap)
	: fBitmap(bitmap),
	  fIcon(NULL),
	  fStyles(StyleManager::Default()),

	  fGammaTable(1.0),

	  fRenderingBuffer(),
	  fPixelFormat(fRenderingBuffer),
	  fPixelFormatPre(fRenderingBuffer),
	  fBaseRenderer(fPixelFormat),
	  fBaseRendererPre(fPixelFormatPre),

	  fScanline(),
	  fBinaryScanline(),
	  fSpanAllocator(),

	  fRasterizer()
{
	// attach rendering buffer to bitmap
	fRenderingBuffer.attach((uint8*)bitmap->Bits(),
							bitmap->Bounds().IntegerWidth() + 1,
							bitmap->Bounds().IntegerHeight() + 1,
							bitmap->BytesPerRow());
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

// #pragma mark -

// _Render
void
IconRenderer::_Render(const BRect& r)
{
	fBaseRendererPre.clear(agg::rgba8(0, 0, 0, 0));

	if (!fIcon)
		return;

	fBaseRendererPre.clip_box((int)floorf(r.left),
							  (int)floorf(r.top),
							  (int)ceilf(r.right),
							  (int)ceilf(r.bottom));

	fRasterizer.reset();
	// iterate over shapes in icon and
	// add the vector paths to the rasterizer
	// and associate the shapes style
	int32 shapeCount = fIcon->Shapes()->CountShapes();
	for (int32 s = 0; s < shapeCount; s++) {
		Shape* shape = fIcon->Shapes()->ShapeAtFast(s);
		agg::path_storage aggPath;
		int32 pathCount = shape->Paths()->CountPaths();
		for (int32 p = 0; p < pathCount; p++) {
			VectorPath* path = shape->Paths()->PathAtFast(p);
			agg::path_storage subPath;
			if (path->GetAGGPathStorage(subPath)) {
				aggPath.concat_path(subPath);
			}
		}
		int32 styleIndex = fStyles->IndexOf(shape->Style());
		fRasterizer.styles(styleIndex, -1);
		agg::conv_curve<agg::path_storage> curve(aggPath);
		fRasterizer.add_path(curve);
	}

	StyleHandler styleHandler(fStyles, fGammaTable);

	agg::render_scanlines_compound(fRasterizer,
								   fScanline,
								   fBinaryScanline,
								   fBaseRendererPre,
								   fSpanAllocator,
								   styleHandler);

	fPixelFormat.apply_gamma_dir(fGammaTable);
}

