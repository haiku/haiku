// AGGTextRenderer.h

#ifndef AGG_TEXT_RENDERER_H
#define AGG_TEXT_RENDERER_H

#include <agg_conv_curve.h>
#include <agg_conv_contour.h>

#include "agg_font_freetype.h"
#include "defines.h"

#include "TextRenderer.h"

class AGGTextRenderer : public TextRenderer {
 public:
								AGGTextRenderer();
								AGGTextRenderer(BMessage* archive);
								AGGTextRenderer(const AGGTextRenderer& from);
	virtual						~AGGTextRenderer();

	virtual	void				SetTo(const TextRenderer* other);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

	virtual	bool				SetFont(const char* pathToFontFile);
	virtual	void				Unset();

	virtual	const char*			Family() const;
	virtual	const char*			Style() const;
	virtual	const char*			PostScriptName() const;

	virtual	void				RenderString(const char* utf8String,
											 font_renderer_solid_type* solidRenderer,
											 font_renderer_bin_type* binRenderer,
											 const Transformable& transform);

	virtual	BRect				Bounds(const char* utf8String,
									   const Transformable& transform);

 private:

	typedef agg::font_engine_freetype_int32				font_engine_type;
	typedef agg::font_cache_manager<font_engine_type>	font_manager_type;

			font_engine_type	fFontEngine;
			font_manager_type	fFontManager;

	// Pipeline to process the vectors glyph paths (curves + contour)
	agg::conv_curve<font_manager_type::path_adaptor_type>
								fCurves;
	agg::conv_contour<agg::conv_curve<font_manager_type::path_adaptor_type> >
								fContour;

			BRect				fBounds;
};

#endif // AGG_TEXT_RENDERER_H
