/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconRenderer.h"

#include <new>

#include <Bitmap.h>

#include "Icon.h"
#include "Style.h"
#include "StyleManager.h"

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

		return style->Gradient() != NULL;
	}

	const agg::rgba8& color(unsigned styleIndex)
    {
		Style* style = fStyles->StyleAt(styleIndex);
		if (!style)
			return fTransparent;

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

	  fGammaTable(2.2),

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

	// attach bitmap
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

// #pragma mark -

// _RenderIcon
void
IconRenderer::_RenderIcon()
{
	fBaseRenderer.clear(agg::rgba8(0, 0, 0, 0));

	if (!fIcon)
		return;

	// TODO: iterate over shapes in icon and
	// add the vector paths to the rasterizer

	StyleHandler styleHandler(fStyles, fGammaTable);

	agg::render_scanlines_compound(fRasterizer,
								   fScanline,
								   fBinaryScanline,
								   fBaseRendererPre,
								   fSpanAllocator,
								   styleHandler);

	fPixelFormat.apply_gamma_dir(fGammaTable);
}

