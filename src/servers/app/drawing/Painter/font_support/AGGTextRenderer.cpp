// AGGTextRenderer.cpp

#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <FontServer.h>
#include <Message.h>
#include <ServerFont.h>
#include <UTF8.h>

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <agg_conv_segmentator.h>
#include <agg_conv_transform.h>
//#include <agg_rendering_buffer.h>
//#include <agg_scanline_bin.h>
//#include <agg_renderer_mclip.h>
//#include <agg_renderer_scanline.h>
//#include <agg_renderer_primitives.h>
//#include <agg_rasterizer_scanline_aa.h>
//#include <agg_pixfmt_gray8.h>
#include <agg_trans_affine.h>

//#include "support.h"

#include "FontManager.h"

#include "AGGTextRenderer.h"

#define FLIP_Y false

// rect_to_int
inline void
rect_to_int(BRect r,
			int32& left, int32& top, int32& right, int32& bottom)
{
	left = (int32)floorf(r.left);
	top = (int32)floorf(r.top);
	right = (int32)ceilf(r.right);
	bottom = (int32)ceilf(r.bottom);
}

#define DEFAULT_UNI_CODE_BUFFER_SIZE	2048

// constructor
AGGTextRenderer::AGGTextRenderer()
	: TextRenderer(),
	  fFontEngine(ftlib),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves),
	  fUnicodeBuffer((char*)malloc(DEFAULT_UNI_CODE_BUFFER_SIZE)),
	  fUnicodeBufferSize(DEFAULT_UNI_CODE_BUFFER_SIZE)
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(FLIP_Y);
}

AGGTextRenderer::AGGTextRenderer(BMessage* archive)
	: TextRenderer(archive),
	  fFontEngine(ftlib),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves),
	  fUnicodeBuffer((char*)malloc(DEFAULT_UNI_CODE_BUFFER_SIZE)),
	  fUnicodeBufferSize(DEFAULT_UNI_CODE_BUFFER_SIZE)
{
//printf("AGGTextRenderer::AGGTextRenderer(BMessage*)\n");
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(FLIP_Y);
}

// constructor
AGGTextRenderer::AGGTextRenderer(const AGGTextRenderer& from)
	: TextRenderer(from),
	  fFontEngine(ftlib),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves),
	  fUnicodeBuffer((char*)malloc(DEFAULT_UNI_CODE_BUFFER_SIZE)),
	  fUnicodeBufferSize(DEFAULT_UNI_CODE_BUFFER_SIZE)
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(FLIP_Y);
}

// destructor
AGGTextRenderer::~AGGTextRenderer()
{
	Unset();
	free(fUnicodeBuffer);
}

// SetTo
void
AGGTextRenderer::SetTo(const TextRenderer* other)
{
	const AGGTextRenderer* casted = dynamic_cast<const AGGTextRenderer*>(other);
	if (casted) {
		TextRenderer::SetTo(other);
	}
}

// Archive
status_t
AGGTextRenderer::Archive(BMessage* into, bool deep) const
{
	status_t status = TextRenderer::Archive(into, deep);
	if (status >= B_OK) {
		status = into->AddString("class", "AGGTextRenderer");
	}
	return status;
}

// SetFont
bool
AGGTextRenderer::SetFont(const ServerFont &font)
{
//printf("AGGTextRenderer::SetFont(%s, %s)\n", font.GetFamily(), font.GetStyle());
	if (fFontEngine.load_font(font, agg::glyph_ren_native_gray8)) {			
//	if (fFontEngine.load_font(font, agg::glyph_ren_outline)) {
		return TextRenderer::SetFont(font);
	} else {
		fprintf(stderr, "font could not be loaded\n");
	}
	return false;
}

// Unset
void
AGGTextRenderer::Unset()
{
}

// Family
const char*
AGGTextRenderer::Family() const
{
	const char* family = NULL;
	if (fFontFilePath) {
		entry_ref ref;
		if (get_ref_for_path(fFontFilePath, &ref) >= B_OK) {
			FontManager* fm = FontManager::Default();
			if (fm->Lock()) {
				family = fm->FamilyFor(&ref);
				fm->Unlock();
			}
		}
	}
	return family;
}

// Style
const char*
AGGTextRenderer::Style() const
{
	const char* style = NULL;
	if (fFontFilePath) {
		entry_ref ref;
		if (get_ref_for_path(fFontFilePath, &ref) >= B_OK) {
			FontManager* fm = FontManager::Default();
			if (fm->Lock()) {
				style = fm->StyleFor(&ref);
				fm->Unlock();
			}
		}
	}
	return style;
}

// PostScriptName
const char*
AGGTextRenderer::PostScriptName() const
{
	const char* name = NULL;
	if (fFontFilePath) {
		entry_ref ref;
		if (get_ref_for_path(fFontFilePath, &ref) >= B_OK) {
			FontManager* fm = FontManager::Default();
			if (fm->Lock()) {
				name = fm->PostScriptNameFor(&ref);
				fm->Unlock();
			}
		}
	}
	return name;
}

// RenderString
BRect
AGGTextRenderer::RenderString(const char* string,
							  uint32 length,
							  font_renderer_solid_type* solidRenderer,
							  font_renderer_bin_type* binRenderer,
							  const Transformable& transform,
							  BRect clippingFrame,
							  bool dryRun,
							  BPoint* nextCharPos)
{
//printf("RenderString(\"%s\", length: %ld, dry: %d)\n", string, length, dryRun);

	fFontEngine.hinting(fHinted);
	fFontEngine.height((int32)(fPtSize));
	fFontEngine.width((int32)(fPtSize));

	BRect bounds(0.0, 0.0, -1.0, -1.0);

	fCurves.approximation_scale(transform.scale());

	// use a transformation behind the curves
	// (only if glyph->data_type == agg::glyph_data_outline)
	// in the pipeline for the rasterizer
	typedef agg::conv_transform<conv_font_curve_type, agg::trans_affine>	conv_font_trans_type;
	conv_font_trans_type ftrans(fCurves, transform);

	int32 srcLength = min_c(length, strlen(string));
	int32 dstLength = srcLength * 4;

	if (dstLength > fUnicodeBufferSize) {
		fUnicodeBufferSize = dstLength;
		fUnicodeBuffer = (char*)realloc((void*)fUnicodeBuffer, fUnicodeBufferSize);
	}

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 string, &srcLength,
								 fUnicodeBuffer, &dstLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, fUnicodeBuffer, dstLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* p = (uint16*)fUnicodeBuffer;

		double x  = 0.0;
		double y0 = 0.0;
		double y  = y0;

		double advanceX = 0.0;
		double advanceY = 0.0;

		// for when we bypass the transformation pipeline
		BPoint transformOffset(0.0, 0.0);
		transform.Transform(&transformOffset);

		for (int32 i = 0; i < dstLength / 2; i++) {

/*			// line break (not supported by R5)
			if (*p == '\n') {
				y0 += LineOffset();
				x = 0.0;
				y = y0;
				advanceX = 0.0;
				advanceY = 0.0;
				++p;
				continue;
			}*/

			const agg::glyph_cache* glyph = fFontManager.glyph(*p);

			if (glyph) {

				if (i > 0 && fKerning) {
					fFontManager.add_kerning(&advanceX, &advanceY);
				}

				x += advanceX;
				y += advanceY;

				// calculate bounds
				const agg::rect& r = glyph->bounds;
				BRect glyphBounds(r.x1 + x, r.y1 + y, r.x2 + x, r.y2 + y);

				// init the fontmanager and transform glyph bounds
				if (glyph->data_type != agg::glyph_data_outline) {
					// we cannot use the transformation pipeline
					double transformedX = x + transformOffset.x;
					double transformedY = y + transformOffset.y;
					fFontManager.init_embedded_adaptors(glyph,
														transformedX,
														transformedY);
					glyphBounds.OffsetBy(transformOffset);
				} else {
					fFontManager.init_embedded_adaptors(glyph, x, y);
					glyphBounds = transform.TransformBounds(glyphBounds);
				}

				// render glyph and update touched area
				if (!dryRun && clippingFrame.Intersects(glyphBounds)) {
					switch (glyph->data_type) {
						case agg::glyph_data_mono:
							agg::render_scanlines(fFontManager.mono_adaptor(), 
												  fFontManager.mono_scanline(), 
												  *binRenderer);
							break;
		
						case agg::glyph_data_gray8:
							agg::render_scanlines(fFontManager.gray8_adaptor(), 
												  fFontManager.gray8_scanline(), 
												  *solidRenderer);
							break;
		
						case agg::glyph_data_outline:
							fRasterizer.reset();
		// NOTE: this function can be easily extended to handle
		// conversion to contours, so that's why there is a lot of
		// commented out code, I leave here because I think it
		// will be needed again.
		
	//						if(fabs(0.0) <= 0.01) {
							// For the sake of efficiency skip the
							// contour converter if the weight is about zero.
							//-----------------------
	//							fRasterizer.add_path(fCurves);
								fRasterizer.add_path(ftrans);
	/*						} else {
	//							fRasterizer.add_path(fContour);
								fRasterizer.add_path(ftrans);
							}*/
							if (fAntialias) {
								agg::render_scanlines(fRasterizer, fScanline, *solidRenderer);
							} else {
								agg::render_scanlines(fRasterizer, fScanline, *binRenderer);
							}
							break;
						default:
							break;
					}
				}
				if (glyphBounds.IsValid())
					bounds = bounds.IsValid() ? bounds | glyphBounds : glyphBounds;

				// increment pen position
				advanceX = glyph->advance_x;
				advanceY = glyph->advance_y;
			}
			++p;
		}
		// put pen location behind rendered text
		// (at the baseline of the virtual next glyph)
		if (nextCharPos) {
			nextCharPos->x = x + advanceX;
			nextCharPos->y = y + advanceY;
		}
	} else {
		fprintf(stderr, "UTF8 -> Unicode conversion failed: %s\n", strerror(ret));
	}

//	return transform.TransformBounds(bounds);
	return bounds;
}

