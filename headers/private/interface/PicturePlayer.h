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


class BAffineTransform;
class BList;
class BPicture;
class BShape;
class Layer;


namespace BPrivate {


struct picture_player_callbacks {
	void (*move_pen_by)(void* userData, const BPoint& where);
	void (*stroke_line)(void* userData, const BPoint& start, const BPoint& end);
	void (*draw_rect)(void* userData, const BRect& rect, bool fill);
	void (*draw_round_rect)(void* userData, const BRect& rect,
		const BPoint& radii, bool fill);
	void (*draw_bezier)(void* userData, size_t numControlPoints,
		const BPoint controlPoints[], bool fill);
	void (*draw_arc)(void* userData, const BPoint& center, const BPoint& radii,
		float startTheta, float arcTheta, bool fill);
	void (*draw_ellipse)(void* userData, const BRect& rect, bool fill);
	void (*draw_polygon)(void* userData, size_t numPoints,
		const BPoint points[], bool isClosed, bool fill);
	void (*draw_shape)(void* userData, const BShape& shape, bool fill);
	void (*draw_string)(void* userData, const char* string, size_t length,
		float spaceEscapement, float nonSpaceEscapement);
	void (*draw_pixels)(void* userData, const BRect& source,
		const BRect& destination, uint32 width, uint32 height,
		size_t bytesPerRow, color_space pixelFormat, uint32 flags,
		const void* data, size_t length);
	void (*draw_picture)(void* userData, const BPoint& where, int32 token);
	void (*set_clipping_rects)(void* userData, size_t numRects,
		const BRect rects[]);
	void (*clip_to_picture)(void* userData, int32 token,
		const BPoint& where, bool clipToInverse);
	void (*push_state)(void* userData);
	void (*pop_state)(void* userData);
	void (*enter_state_change)(void* userData);
	void (*exit_state_change)(void* userData);
	void (*enter_font_state)(void* userData);
	void (*exit_font_state)(void* userData);
	void (*set_origin)(void* userData, const BPoint& origin);
	void (*set_pen_location)(void* userData, const BPoint& location);
	void (*set_drawing_mode)(void* userData, drawing_mode mode);
	void (*set_line_mode)(void* userData, cap_mode capMode, join_mode joinMode,
		float miterLimit);
	void (*set_pen_size)(void* userData, float size);
	void (*set_fore_color)(void* userData, const rgb_color& color);
	void (*set_back_color)(void* userData, const rgb_color& color);
	void (*set_stipple_pattern)(void* userData, const pattern& patter);
	void (*set_scale)(void* userData, float scale);
	void (*set_font_family)(void* userData, const char* familyName,
		size_t length);
	void (*set_font_style)(void* userData, const char* styleName,
		size_t length);
	void (*set_font_spacing)(void* userData, uint8 spacing);
	void (*set_font_size)(void* userData, float size);
	void (*set_font_rotation)(void* userData, float rotation);
	void (*set_font_encoding)(void* userData, uint8 encoding);
	void (*set_font_flags)(void* userData, uint32 flags);
	void (*set_font_shear)(void* userData, float shear);
	void (*set_font_face)(void* userData, uint16 face);
	void (*set_blending_mode)(void* userData, source_alpha alphaSourceMode,
		alpha_function alphaFunctionMode);
	void (*set_transform)(void* userData, const BAffineTransform& transform);
	void (*translate_by)(void* userData, double x, double y);
	void (*scale_by)(void* userData, double x, double y);
	void (*rotate_by)(void* userData, double angleRadians);
	void (*blend_layer)(void* userData, Layer* layer);
	void (*clip_to_rect)(void* userData, const BRect& rect, bool inverse);
	void (*clip_to_shape)(void* userData, int32 opCount, const uint32 opList[],
		int32 ptCount, const BPoint ptList[], bool inverse);
	void (*draw_string_locations)(void* userData, const char* string,
		size_t length, const BPoint locations[], size_t locationCount);
};


class PicturePlayer {
public:
	PicturePlayer();
	PicturePlayer(const void* data, size_t size, BList* pictures);
	virtual	~PicturePlayer();

	status_t	Play(void** callbacks, int32 tableEntries,
					void* userData);
	status_t	Play(const picture_player_callbacks& callbacks,
					size_t callbacksSize, void* userData);

private:
	status_t	_Play(const picture_player_callbacks& callbacks, void* userData,
					const void* data, size_t length, uint16 parentOp);

	const void*	fData;
	size_t		fSize;
	BList*		fPictures;
};

} // namespace BPrivate

#endif // _PICTURE_PLAYER_H
