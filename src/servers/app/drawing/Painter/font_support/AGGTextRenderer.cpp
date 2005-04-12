// AGGTextRenderer.cpp

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <FontServer.h>
#include <Bitmap.h>
#include <ByteOrder.h>
#include <Entry.h>
#include <Message.h>
#include <UTF8.h>

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <agg_conv_segmentator.h>
#include <agg_conv_transform.h>
//#include <agg_rendering_buffer.h>
#include <agg_scanline_u.h>
#include <agg_scanline_bin.h>
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


// constructor
AGGTextRenderer::AGGTextRenderer()
	: TextRenderer(),
	  fFontEngine(ftlib),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves)
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
	  fContour(fCurves)
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
	  fContour(fCurves)
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(FLIP_Y);
}

// destructor
AGGTextRenderer::~AGGTextRenderer()
{
	Unset();
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
	fFontEngine.hinting(fHinted);
	fFontEngine.height((int32)(fPtSize));
	fFontEngine.width((int32)(fPtSize));

	BRect bounds(0.0, 0.0, -1.0, -1.0);

	typedef agg::conv_curve<font_manager_type::path_adaptor_type>			conv_font_curve_type;
//	typedef agg::conv_segmentator<conv_font_curve_type>						conv_font_segm_type;
//	typedef agg::conv_transform<conv_font_segm_type, agg::trans_affine>		conv_font_trans_type;
	typedef agg::conv_transform<conv_font_curve_type, agg::trans_affine>	conv_font_trans_type;
	
	agg::scanline_u8 sl;
	agg::rasterizer_scanline_aa<> ras;


	conv_font_curve_type fcurves(fFontManager.path_adaptor());
	fcurves.approximation_scale(transform.scale());
//	conv_font_segm_type  fsegm(fcurves);
//	conv_font_trans_type ftrans(fsegm, transform);
	conv_font_trans_type ftrans(fcurves, transform);


	int32 srcLength = min_c(length, strlen(string));
	int32 dstLength = srcLength * 4;

	char* buffer = new char[dstLength];

	int32 state = 0;
	status_t ret;
	if ((ret = convert_from_utf8(B_UNICODE_CONVERSION, 
								 string, &srcLength,
								 buffer, &dstLength,
								 &state, B_SUBSTITUTE)) >= B_OK
		&& (ret = swap_data(B_INT16_TYPE, buffer, dstLength,
							B_SWAP_BENDIAN_TO_HOST)) >= B_OK) {

		uint16* p = (uint16*)buffer;

		double x  = 0.0;
		double y0 = 0.0;
		double y  = y0;

		double advanceX = 0.0;
		double advanceY = 0.0;

		// for when we bypass the transformation pipeline
		BPoint transformOffset(0.0, 0.0);
		transform.Transform(&transformOffset);

		for (int32 i = 0; i < dstLength / 2; i++) {

			// line break
			if (*p == '\n') {
				y0 += LineOffset();
				x = 0.0;
				y = y0;
				advanceX = 0.0;
				advanceY = 0.0;
				++p;
				continue;
			}


			const agg::glyph_cache* glyph = fFontManager.glyph(*p);

			if (glyph) {

				if (fKerning) {
					fFontManager.add_kerning(&advanceX, &advanceY);
				}

				x += fAdvanceScale * advanceX;
				if (advanceX > 0.0 && fAdvanceScale > 1.0)
					x += (fAdvanceScale - 1.0) * fFontEngine.height();
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
					switch(glyph->data_type) {
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
							ras.reset();
		// NOTE: this function can be easily extended to handle
		// conversion to contours, to that's why there is a lot of
		// commented out code, I leave here because I think it
		// will be needed again.
		
	//						if(fabs(0.0) <= 0.01) {
							// For the sake of efficiency skip the
							// contour converter if the weight is about zero.
							//-----------------------
	//							ras.add_path(fCurves);
								ras.add_path(ftrans);
	/*						} else {
	//							ras.add_path(fContour);
								ras.add_path(ftrans);
							}*/
							if (fAntialias) {
								agg::render_scanlines(ras, sl, *solidRenderer);
							} else {
								agg::render_scanlines(ras, sl, *binRenderer);
							}
							break;
						default:
							break;
					}
				}
				if (glyphBounds.IsValid())
					bounds = bounds.IsValid() ? bounds | glyphBounds : glyphBounds;

				// increment pen position
				advanceX = fHinted ? floorf(glyph->advance_x + 0.5) : glyph->advance_x;
				advanceY = fHinted ? floorf(glyph->advance_y + 0.5) : glyph->advance_y;
			}
			++p;
		}
		if (nextCharPos) {
			x += fAdvanceScale * advanceX;
			if (advanceX > 0.0 && fAdvanceScale > 1.0)
				x += (fAdvanceScale - 1.0) * fFontEngine.height();
			y += advanceY;
			nextCharPos->x = x;
			nextCharPos->y = y;
		}
	} else {
		fprintf(stderr, "UTF8 -> Unicode conversion failed: %s\n", strerror(ret));
	}
	delete[] buffer;

//	return transform.TransformBounds(bounds);
	return bounds;
}

