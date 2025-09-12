/*
 * Copyright 2001-2018, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen (marcus@overhagen.de)
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef	_PICTURE_PLAYER_H
#define	_PICTURE_PLAYER_H

/*!	PicturePlayer is used to play picture data. */


#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>


class BAffineTransform;
class BGradient;
class BList;
class BPicture;
class BShape;
class Layer;


namespace BPrivate {


struct picture_player_callbacks_compat {
	/*  0 */ void (*nop)(void* user);
	/*  1 */ void (*move_pen_by)(void* user, BPoint delta);
	/*  2 */ void (*stroke_line)(void* user, BPoint start, BPoint end);
	/*  3 */ void (*stroke_rect)(void* user, BRect rect);
	/*  4 */ void (*fill_rect)(void* user, BRect rect);
	/*  5 */ void (*stroke_round_rect)(void* user, BRect rect, BPoint radii);
	/*  6 */ void (*fill_round_rect)(void* user, BRect rect, BPoint radii);
	/*  7 */ void (*stroke_bezier)(void* user, BPoint* control);
	/*  8 */ void (*fill_bezier)(void* user, BPoint* control);
	/*  9 */ void (*stroke_arc)(void* user, BPoint center, BPoint radii, float startTheta,
		float arcTheta);
	/* 10 */ void (*fill_arc)(void* user, BPoint center, BPoint radii, float startTheta,
		float arcTheta);
	/* 11 */ void (*stroke_ellipse)(void* user, BPoint center, BPoint radii);
	/* 12 */ void (*fill_ellipse)(void* user, BPoint center, BPoint radii);
	/* 13 */ void (*stroke_polygon)(void* user, int32 numPoints, const BPoint* points,
		bool isClosed);
	/* 14 */ void (*fill_polygon)(void* user, int32 numPoints, const BPoint* points,
		bool isClosed);
	// Called "Reserved" in BeBook
	/* 15 */ void (*stroke_shape)(void* user, const BShape *shape);
	// Called "Reserved" in BeBook
	/* 16 */ void (*fill_shape)(void* user, const BShape *shape);
	/* 17 */ void (*draw_string)(void* user, const char* string, float deltax, float deltay);
	/* 18 */ void (*draw_pixels)(void* user, BRect src, BRect dest, int32 width, int32 height,
		int32 bytesPerRow, int32 pixelFormat, int32 flags, const void* data);
	// Called "Reserved" in BeBook
	/* 19 */ void (*draw_picture)(void* user, BPoint where, int32 token);
	/* 20 */ void (*set_clipping_rects)(void* user, const BRect* rects, uint32 numRects);
	// Called "Reserved" in BeBook
	/* 21 */ void (*clip_to_picture)(void* user, int32 token, BPoint point,
		bool clip_to_inverse_picture);
	/* 22 */ void (*push_state)(void* user);
	/* 23 */ void (*pop_state)(void* user);
	/* 24 */ void (*enter_state_change)(void* user);
	/* 25 */ void (*exit_state_change)(void* user);
	/* 26 */ void (*enter_font_state)(void* user);
	/* 27 */ void (*exit_font_state)(void* user);
	/* 28 */ void (*set_origin)(void* user, BPoint pt);
	/* 29 */ void (*set_pen_location)(void* user, BPoint pt);
	/* 30 */ void (*set_drawing_mode)(void* user, drawing_mode mode);
	/* 31 */ void (*set_line_mode)(void* user, cap_mode capMode, join_mode joinMode,
		float miterLimit);
	/* 32 */ void (*set_pen_size)(void* user, float size);
	/* 33 */ void (*set_fore_color)(void* user, rgb_color color);
	/* 34 */ void (*set_back_color)(void* user, rgb_color color);
	/* 35 */ void (*set_stipple_pattern)(void* user, pattern p);
	/* 36 */ void (*set_scale)(void* user, float scale);
	/* 37 */ void (*set_font_family)(void* user, const char* family);
	/* 38 */ void (*set_font_style)(void* user, const char* style);
	/* 39 */ void (*set_font_spacing)(void* user, int32 spacing);
	/* 40 */ void (*set_font_size)(void* user, float size);
	/* 41 */ void (*set_font_rotate)(void* user, float rotation);
	/* 42 */ void (*set_font_encoding)(void* user, int32 encoding);
	/* 43 */ void (*set_font_flags)(void* user, int32 flags);
	/* 44 */ void (*set_font_shear)(void* user, float shear);
	// Called "Reserved" in BeBook
	/* 45 */ void (*set_font_bpp)(void* user, int32 bpp);
	/* 46 */ void (*set_font_face)(void* user, int32 flags);

	// New in Haiku
	/* 47 */ void (*set_blending_mode)(void* user, source_alpha alphaSrcMode,
		alpha_function alphaFncMode);
	/* 48 */ void (*set_transform)(void* user, const BAffineTransform& transform);
	/* 49 */ void (*translate_by)(void* user, double x, double y);
	/* 50 */ void (*scale_by)(void* user, double x, double y);
	/* 51 */ void (*rotate_by)(void* user, double angleRadians);
	// Broken when saved to file
	/* 52 */ void (*blend_layer)(void* user, class Layer* layer);
	/* 53 */ void (*clip_to_rect)(void* user, const BRect& rect, bool inverse);
	// Why not BShape?
	/* 54 */ void (*clip_to_shape)(void* user, int32 opCount, const uint32 opList[], int32 ptCount,
		const BPoint ptList[], bool inverse);
	/* 55 */ void (*draw_string_locations)(void* user, const char* string, const BPoint* locations,
		size_t locationCount);

	/* 56 */ void (*fill_rect_gradient)(void* user, BRect rect, const BGradient& gradient);
	/* 57 */ void (*stroke_rect_gradient)(void* user, BRect rect, const BGradient& gradient);
	/* 58 */ void (*fill_round_rect_gradient)(void* user, BRect rect, BPoint radii,
		const BGradient& gradient);
	/* 59 */ void (*stroke_round_rect_gradient)(void* user, BRect rect, BPoint radii,
		const BGradient& gradient);
	/* 60 */ void (*fill_bezier_gradient)(void* user, const BPoint* points,
		const BGradient& gradient);
	/* 61 */ void (*stroke_bezier_gradient)(void* user, const BPoint* points,
		const BGradient& gradient);
	/* 62 */ void (*fill_arc_gradient)(void* user, BPoint center, BPoint radii, float startTheta,
		float arcTheta, const BGradient& gradient);
	/* 63 */ void (*stroke_arc_gradient)(void* user, BPoint center, BPoint radii, float startTheta,
		float arcTheta, const BGradient& gradient);
	/* 64 */ void (*fill_ellipse_gradient)(void* user, BPoint center, BPoint radii,
		const BGradient& gradient);
	/* 65 */ void (*stroke_ellipse_gradient)(void* user, BPoint center, BPoint radii,
		const BGradient& gradient);
	/* 66 */ void (*fill_polygon_gradient)(void* user, int32 numPoints, const BPoint* points,
		bool isClosed, const BGradient& gradient);
	/* 67 */ void (*stroke_polygon_gradient)(void* user, int32 numPoints, const BPoint* points,
		bool isClosed, const BGradient& gradient);
	/* 68 */ void (*fill_shape_gradient)(void* user, BShape shape, const BGradient& gradient);
	/* 69 */ void (*stroke_shape_gradient)(void* user, BShape shape, const BGradient& gradient);

	/* 70 */ void (*set_fill_rule)(void* user, int32 fillRule);

	/* 71 */ void (*stroke_line_gradient)(void* user, BPoint start, BPoint end,
		const BGradient& gradient);
};


class PicturePlayerCallbacks {
public:
	virtual void MovePenBy(const BPoint& where) {}
	virtual void StrokeLine(const BPoint& start, const BPoint& end) {}
	virtual void DrawRect(const BRect& rect, bool fill) {}
	virtual void DrawRoundRect(const BRect& rect, const BPoint& radii, bool fill) {}
	virtual void DrawBezier(const BPoint controlPoints[4], bool fill) {}
	virtual void DrawArc(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, bool fill) {}
	virtual void DrawEllipse(const BRect& rect, bool fill) {}
	virtual void DrawPolygon(size_t numPoints, const BPoint points[], bool isClosed, bool fill) {}
	virtual void DrawShape(const BShape& shape, bool fill) {}
	virtual void DrawString(const char* string, size_t length, float spaceEscapement,
		float nonSpaceEscapement) {}
	virtual void DrawPixels(const BRect& source, const BRect& destination, uint32 width,
		uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 flags, const void* data,
		size_t length) {}
	virtual void DrawPicture(const BPoint& where, int32 token) {}
	virtual void SetClippingRects(size_t numRects, const clipping_rect rects[]) {}
	virtual void ClipToPicture(int32 token, const BPoint& where, bool clipToInverse) {}
	virtual void PushState() {}
	virtual void PopState() {}
	virtual void EnterStateChange() {}
	virtual void ExitStateChange() {}
	virtual void EnterFontState() {}
	virtual void ExitFontState() {}
	virtual void SetOrigin(const BPoint& origin) {}
	virtual void SetPenLocation(const BPoint& location) {}
	virtual void SetDrawingMode(drawing_mode mode) {}
	virtual void SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit) {}
	virtual void SetPenSize(float size) {}
	virtual void SetForeColor(const rgb_color& color) {}
	virtual void SetBackColor(const rgb_color& color) {}
	virtual void SetStipplePattern(const pattern& patter) {}
	virtual void SetScale(float scale) {}
	virtual void SetFontFamily(const char* familyName, size_t length) {}
	virtual void SetFontStyle(const char* styleName, size_t length) {}
	virtual void SetFontSpacing(uint8 spacing) {}
	virtual void SetFontSize(float size) {}
	virtual void SetFontRotation(float rotation) {}
	virtual void SetFontEncoding(uint8 encoding) {}
	virtual void SetFontFlags(uint32 flags) {}
	virtual void SetFontShear(float shear) {}
	virtual void SetFontFace(uint16 face) {}
	virtual void SetBlendingMode(source_alpha alphaSourceMode, alpha_function alphaFunctionMode) {}
	virtual void SetTransform(const BAffineTransform& transform) {}
	virtual void TranslateBy(double x, double y) {}
	virtual void ScaleBy(double x, double y) {}
	virtual void RotateBy(double angleRadians) {}
	virtual void BlendLayer(Layer* layer) {}
	virtual void ClipToRect(const BRect& rect, bool inverse) {}
	virtual void ClipToShape(int32 opCount, const uint32 opList[], int32 ptCount,
		const BPoint ptList[], bool inverse) {}
	virtual void DrawStringLocations(const char* string, size_t length, const BPoint locations[],
		size_t locationCount) {}
	virtual void DrawRectGradient(const BRect& rect, BGradient& gradient, bool fill) {}
	virtual void DrawRoundRectGradient(const BRect& rect, const BPoint& radii, BGradient& gradient,
		bool fill) {}
	virtual void DrawBezierGradient(const BPoint controlPoints[4], BGradient& gradient, bool fill)
		{}
	virtual void DrawArcGradient(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, BGradient& gradient, bool fill) {}
	virtual void DrawEllipseGradient(const BRect& rect, BGradient& gradient, bool fill) {}
	virtual void DrawPolygonGradient(size_t numPoints, const BPoint points[], bool isClosed,
		BGradient& gradient, bool fill) {}
	virtual void DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill) {}
	virtual void SetFillRule(int32 fillRule) {}
	virtual void StrokeLineGradient(const BPoint& start, const BPoint& end,
		const BGradient& gradient) {}
};


class PicturePlayer {
public:
	PicturePlayer();
	PicturePlayer(const void* data, size_t size, BList* pictures);
	virtual	~PicturePlayer();

	status_t	Play(void** callbacks, int32 tableEntries,
					void* userData);
	status_t	Play(PicturePlayerCallbacks& callbacks);

private:
	status_t	_Play(PicturePlayerCallbacks& callbacks,
					const void* data, size_t length, uint16 parentOp);

	const void*	fData;
	size_t		fSize;
	BList*		fPictures;
};

} // namespace BPrivate

#endif // _PICTURE_PLAYER_H
