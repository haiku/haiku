// AGGTextRenderer.cpp

#include <math.h>
#include <stdio.h>
#include <string.h>

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
	  fFontEngine(),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves)
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(true);
}

AGGTextRenderer::AGGTextRenderer(BMessage* archive)
	: TextRenderer(archive),
	  fFontEngine(),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves)
{
//printf("AGGTextRenderer::AGGTextRenderer(BMessage*)\n");
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(true);

	if (fFontFilePath)
		SetFont(fFontFilePath);
}

// constructor
AGGTextRenderer::AGGTextRenderer(const AGGTextRenderer& from)
	: TextRenderer(from),
	  fFontEngine(),
	  fFontManager(fFontEngine),
	  fCurves(fFontManager.path_adaptor()),
	  fContour(fCurves)
{
	fCurves.approximation_scale(2.0);
	fContour.auto_detect_orientation(false);
	fFontEngine.flip_y(true);

	if (fFontFilePath)
		SetFont(fFontFilePath);
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
AGGTextRenderer::SetFont(const char* pathToFontFile)
{
	if (pathToFontFile) {
		if (fFontEngine.load_font(pathToFontFile, 0, agg::glyph_ren_outline)) {
//		if (fFontEngine.load_font(pathToFontFile, 0, agg::glyph_ren_native_gray8)) {			

			return TextRenderer::SetFont(pathToFontFile);
		} else {
			fprintf(stderr, "%s : is not a font file or could not be opened\n",
					pathToFontFile);
		}
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
void
AGGTextRenderer::RenderString(const char* string,
							  uint32 length,
							  font_renderer_solid_type* solidRenderer,
							  font_renderer_bin_type* binRenderer,
							  const Transformable& transform,
							  BPoint* nextCharPos)
{
	fFontEngine.hinting(false);
	fFontEngine.height((int32)(fPtSize));
	fFontEngine.width((int32)(fPtSize));

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

				fFontManager.init_embedded_adaptors(glyph, x, y);
	
				switch(glyph->data_type) {
					case agg::glyph_data_mono:
//						binRenderer->color(agg::gray8(fOpacity));
						agg::render_scanlines(fFontManager.mono_adaptor(), 
											  fFontManager.mono_scanline(), 
											  *binRenderer);
						break;
		
					case agg::glyph_data_gray8:
//						solidRenderer->color(agg::gray8(fOpacity));
						agg::render_scanlines(fFontManager.gray8_adaptor(), 
											  fFontManager.gray8_scanline(), 
											  *solidRenderer);
						break;
		
					case agg::glyph_data_outline:
						ras.reset();
						if(fabs(0.0) <= 0.01) {
							// For the sake of efficiency skip the
							// contour converter if the weight is about zero.
							//-----------------------
//							ras.add_path(fCurves);
							ras.add_path(ftrans);
						} else {
//							ras.add_path(fContour);
							ras.add_path(ftrans);
						}
						if (fAntialias) {
//							solidRenderer->color(agg::gray8(fOpacity));
							agg::render_scanlines(ras, sl, *solidRenderer);
						} else {
//							binRenderer->color(agg::gray8(fOpacity));
							agg::render_scanlines(ras, sl, *binRenderer);
						}
						break;
					default:
						break;
				}
	
				// increment pen position
//				advanceX = fHinted ? floorf(glyph->advance_x + 0.5) : glyph->advance_x;
//				advanceY = fHinted ? floorf(glyph->advance_y + 0.5) : glyph->advance_y;
				advanceX = glyph->advance_x;
				advanceY = glyph->advance_y;
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
}

// Bounds
BRect
AGGTextRenderer::Bounds(const char* string, uint32 length,
						const Transformable& transform)
{
	fFontEngine.hinting(false);
	fFontEngine.height((int32)(fPtSize));
	fFontEngine.width((int32)(fPtSize));

	BRect bounds(0.0, 0.0, -1.0, -1.0);

	// do a UTF8 -> Unicode conversion
	if (string) {
	
		int32 srcLength = min_c(strlen(string), length);
		if (srcLength > 0) {

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
	
				for (int32 i = 0; i < dstLength / 2; i++) {
	
					// line break
					if (*p == '\n') {
						y0 +=LineOffset();
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
	
						double left = 0.0;
						double top = 0.0;
						double right = -1.0;
						double bottom = -1.0;
						uint32 pathID[1];
						pathID[0] = 0;
			
						fFontManager.init_embedded_adaptors(glyph, x, y);
			
						switch(glyph->data_type) {
							case agg::glyph_data_mono:
								break;
				
							case agg::glyph_data_gray8:
								break;
				
							case agg::glyph_data_outline:
								if (fabs(0.0) <= 0.01) {
									// For the sake of efficiency skip the
									// contour converter if the weight is about zero.
									//-----------------------
									agg::bounding_rect(fContour, pathID, 0, 1, &left, &top, &right, &bottom);
								} else {
									agg::bounding_rect(fCurves, pathID, 0, 1, &left, &top, &right, &bottom);
								}
								break;
							default:
								break;
						}
						BRect t(left, top, right, bottom);
						if (t.IsValid())
							bounds = bounds.IsValid() ? bounds | t : t;
		
						// increment pen position
						advanceX = glyph->advance_x;
						advanceY = glyph->advance_y;
					}
					++p;
				}
				delete[] buffer;
			} else {
				fprintf(stderr, "UTF8 -> Unicode conversion failed: %s\n", strerror(ret));
			}
		}
	}

	return transform.TransformBounds(bounds);
}

