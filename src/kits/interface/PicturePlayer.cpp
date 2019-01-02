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

/**	PicturePlayer is used to play picture data. */

#include <PicturePlayer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AffineTransform.h>
#include <PictureProtocol.h>
#include <Shape.h>


using BPrivate::PicturePlayer;


struct adapter_context {
	void* user_data;
	void** function_table;
};


static void
nop()
{
}


static void
move_pen_by(void* _context, const BPoint& delta)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint))context->function_table[1])(context->user_data,
		delta);
}


static void
stroke_line(void* _context, const BPoint& start, const BPoint& end)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint, BPoint))context->function_table[2])(
		context->user_data, start, end);
}


static void
draw_rect(void* _context, const BRect& rect, bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BRect))context->function_table[fill ? 4 : 3])(
		context->user_data, rect);
}


static void
draw_round_rect(void* _context, const BRect& rect, const BPoint& radii,
	bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BRect, BPoint))context->function_table[fill ? 6 : 5])(
		context->user_data, rect, radii);
}


static void
draw_bezier(void* _context, size_t numPoints, const BPoint _points[], bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	if (numPoints != 4)
		return;

	BPoint points[4] = { _points[0], _points[1], _points[2], _points[3] };
	((void (*)(void*, BPoint*))context->function_table[fill ? 8 : 7])(
		context->user_data, points);
}


static void
draw_arc(void* _context, const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint, BPoint, float, float))
		context->function_table[fill ? 10 : 9])(context->user_data, center,
			radii, startTheta, arcTheta);
}


static void
draw_ellipse(void* _context, const BRect& rect, bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
	BPoint center = rect.LeftTop() + radii;
	((void (*)(void*, BPoint, BPoint))
		context->function_table[fill ? 12 : 11])(context->user_data, center,
			radii);
}


static void
draw_polygon(void* _context, size_t numPoints, const BPoint _points[],
	bool isClosed, bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);

	// This is rather ugly but works for such a trivial class.
	const size_t kMaxStackCount = 200;
	char stackData[kMaxStackCount * sizeof(BPoint)];
	BPoint* points = (BPoint*)stackData;
	if (numPoints > kMaxStackCount) {
		points = (BPoint*)malloc(numPoints * sizeof(BPoint));
		if (points == NULL)
			return;
	}

	memcpy(points, _points, numPoints * sizeof(BPoint));

	((void (*)(void*, int32, BPoint*, bool))
		context->function_table[fill ? 14 : 13])(context->user_data, numPoints,
			points, isClosed);

	if (numPoints > kMaxStackCount)
		free(points);
}


static void
draw_shape(void* _context, const BShape& shape, bool fill)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BShape))context->function_table[fill ? 16 : 15])(
		context->user_data, shape);
}


static void
draw_string(void* _context, const char* _string, size_t length,
	float deltaSpace, float deltaNonSpace)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	char* string = strndup(_string, length);

	((void (*)(void*, char*, float, float))
		context->function_table[17])(context->user_data, string, deltaSpace,
			deltaNonSpace);

	free(string);
}


static void
draw_pixels(void* _context, const BRect& src, const BRect& dest, uint32 width,
	uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 options,
	const void* _data, size_t length)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	void* data = malloc(length);
	if (data == NULL)
		return;

	memcpy(data, _data, length);

	((void (*)(void*, BRect, BRect, int32, int32, int32, int32, int32, void*))
		context->function_table[18])(context->user_data, src, dest, width,
			height, bytesPerRow, pixelFormat, options, data);

	free(data);
}


static void
draw_picture(void* _context, const BPoint& where, int32 token)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint, int32))context->function_table[19])(
		context->user_data, where, token);
}


static void
set_clipping_rects(void* _context, size_t numRects, const BRect _rects[])
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);

	// This is rather ugly but works for such a trivial class.
	const size_t kMaxStackCount = 100;
	char stackData[kMaxStackCount * sizeof(BRect)];
	BRect* rects = (BRect*)stackData;
	if (numRects > kMaxStackCount) {
		rects = (BRect*)malloc(numRects * sizeof(BRect));
		if (rects == NULL)
			return;
	}

	memcpy(rects, _rects, numRects * sizeof(BRect));

	((void (*)(void*, BRect*, uint32))context->function_table[20])(
		context->user_data, rects, numRects);

	if (numRects > kMaxStackCount)
		free(rects);
}


static void
clip_to_picture(void* _context, int32 token, const BPoint& origin,
	bool clipToInverse)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32, BPoint, bool))context->function_table[21])(
			context->user_data, token, origin, clipToInverse);
}


static void
push_state(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[22])(context->user_data);
}


static void
pop_state(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[23])(context->user_data);
}


static void
enter_state_change(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[24])(context->user_data);
}


static void
exit_state_change(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[25])(context->user_data);
}


static void
enter_font_state(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[26])(context->user_data);
}


static void
exit_font_state(void* _context)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*))context->function_table[27])(context->user_data);
}


static void
set_origin(void* _context, const BPoint& origin)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint))context->function_table[28])(context->user_data,
		origin);
}


static void
set_pen_location(void* _context, const BPoint& penLocation)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, BPoint))context->function_table[29])(context->user_data,
		penLocation);
}


static void
set_drawing_mode(void* _context, drawing_mode mode)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, drawing_mode))context->function_table[30])(
		context->user_data, mode);
}


static void
set_line_mode(void* _context, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, cap_mode, join_mode, float))context->function_table[31])(
		context->user_data, capMode, joinMode, miterLimit);
}


static void
set_pen_size(void* _context, float size)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, float))context->function_table[32])(context->user_data,
		size);
}


static void
set_fore_color(void* _context, const rgb_color& color)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, rgb_color))context->function_table[33])(
		context->user_data, color);
}


static void
set_back_color(void* _context, const rgb_color& color)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, rgb_color))context->function_table[34])(
		context->user_data, color);
}


static void
set_stipple_pattern(void* _context, const pattern& stipplePattern)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, pattern))context->function_table[35])(context->user_data,
		stipplePattern);
}


static void
set_scale(void* _context, float scale)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, float))context->function_table[36])(context->user_data,
		scale);
}


static void
set_font_family(void* _context, const char* _family, size_t length)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	char* family = strndup(_family, length);

	((void (*)(void*, char*))context->function_table[37])(context->user_data,
		family);

	free(family);
}


static void
set_font_style(void* _context, const char* _style, size_t length)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	char* style = strndup(_style, length);

	((void (*)(void*, char*))context->function_table[38])(context->user_data,
		style);

	free(style);
}


static void
set_font_spacing(void* _context, uint8 spacing)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32))context->function_table[39])(context->user_data,
		spacing);
}


static void
set_font_size(void* _context, float size)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, float))context->function_table[40])(context->user_data,
		size);
}


static void
set_font_rotation(void* _context, float rotation)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, float))context->function_table[41])(context->user_data,
		rotation);
}


static void
set_font_encoding(void* _context, uint8 encoding)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32))context->function_table[42])(context->user_data,
		encoding);
}


static void
set_font_flags(void* _context, uint32 flags)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32))context->function_table[43])(context->user_data,
		flags);
}


static void
set_font_shear(void* _context, float shear)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, float))context->function_table[44])(context->user_data,
		shear);
}


static void
set_font_face(void* _context, uint16 face)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32))context->function_table[46])(context->user_data,
		face);
}


static void
set_blending_mode(void* _context, source_alpha alphaSrcMode,
	alpha_function alphaFncMode)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, source_alpha, alpha_function))
		context->function_table[47])(context->user_data, alphaSrcMode,
			alphaFncMode);
}


static void
set_transform(void* _context, const BAffineTransform& transform)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, const BAffineTransform&))
		context->function_table[48])(context->user_data, transform);
}


static void
translate_by(void* _context, double x, double y)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, double, double))
		context->function_table[49])(context->user_data, x, y);
}


static void
scale_by(void* _context, double x, double y)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, double, double))
		context->function_table[50])(context->user_data, x, y);
}


static void
rotate_by(void* _context, double angleRadians)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, double))
		context->function_table[51])(context->user_data, angleRadians);
}


static void
blend_layer(void* _context, Layer* layer)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, Layer*))
		context->function_table[52])(context->user_data, layer);
}


static void
clip_to_rect(void* _context, const BRect& rect, bool inverse)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, const BRect&, bool))
		context->function_table[53])(context->user_data, rect, inverse);
}


static void
clip_to_shape(void* _context, int32 opCount, const uint32 opList[],
	int32 ptCount, const BPoint ptList[], bool inverse)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	((void (*)(void*, int32, const uint32*, int32, const BPoint*, bool))
		context->function_table[54])(context->user_data, opCount, opList,
			ptCount, ptList, inverse);
}


static void
draw_string_locations(void* _context, const char* _string, size_t length,
	const BPoint* locations, size_t locationCount)
{
	adapter_context* context = reinterpret_cast<adapter_context*>(_context);
	char* string = strndup(_string, length);

	((void (*)(void*, char*, const BPoint*, size_t))
		context->function_table[55])(context->user_data, string, locations,
			locationCount);

	free(string);
}



#if DEBUG > 1
static const char *
PictureOpToString(int op)
{
	#define RETURN_STRING(x) case x: return #x

	switch(op) {
		RETURN_STRING(B_PIC_MOVE_PEN_BY);
		RETURN_STRING(B_PIC_STROKE_LINE);
		RETURN_STRING(B_PIC_STROKE_RECT);
		RETURN_STRING(B_PIC_FILL_RECT);
		RETURN_STRING(B_PIC_STROKE_ROUND_RECT);
		RETURN_STRING(B_PIC_FILL_ROUND_RECT);
		RETURN_STRING(B_PIC_STROKE_BEZIER);
		RETURN_STRING(B_PIC_FILL_BEZIER);
		RETURN_STRING(B_PIC_STROKE_POLYGON);
		RETURN_STRING(B_PIC_FILL_POLYGON);
		RETURN_STRING(B_PIC_STROKE_SHAPE);
		RETURN_STRING(B_PIC_FILL_SHAPE);
		RETURN_STRING(B_PIC_DRAW_STRING);
		RETURN_STRING(B_PIC_DRAW_STRING_LOCATIONS);
		RETURN_STRING(B_PIC_DRAW_PIXELS);
		RETURN_STRING(B_PIC_DRAW_PICTURE);
		RETURN_STRING(B_PIC_STROKE_ARC);
		RETURN_STRING(B_PIC_FILL_ARC);
		RETURN_STRING(B_PIC_STROKE_ELLIPSE);
		RETURN_STRING(B_PIC_FILL_ELLIPSE);

		RETURN_STRING(B_PIC_ENTER_STATE_CHANGE);
		RETURN_STRING(B_PIC_SET_CLIPPING_RECTS);
		RETURN_STRING(B_PIC_CLIP_TO_PICTURE);
		RETURN_STRING(B_PIC_PUSH_STATE);
		RETURN_STRING(B_PIC_POP_STATE);
		RETURN_STRING(B_PIC_CLEAR_CLIPPING_RECTS);

		RETURN_STRING(B_PIC_SET_ORIGIN);
		RETURN_STRING(B_PIC_SET_PEN_LOCATION);
		RETURN_STRING(B_PIC_SET_DRAWING_MODE);
		RETURN_STRING(B_PIC_SET_LINE_MODE);
		RETURN_STRING(B_PIC_SET_PEN_SIZE);
		RETURN_STRING(B_PIC_SET_SCALE);
		RETURN_STRING(B_PIC_SET_TRANSFORM);
		RETURN_STRING(B_PIC_SET_FORE_COLOR);
		RETURN_STRING(B_PIC_SET_BACK_COLOR);
		RETURN_STRING(B_PIC_SET_STIPLE_PATTERN);
		RETURN_STRING(B_PIC_ENTER_FONT_STATE);
		RETURN_STRING(B_PIC_SET_BLENDING_MODE);
		RETURN_STRING(B_PIC_SET_FONT_FAMILY);
		RETURN_STRING(B_PIC_SET_FONT_STYLE);
		RETURN_STRING(B_PIC_SET_FONT_SPACING);
		RETURN_STRING(B_PIC_SET_FONT_ENCODING);
		RETURN_STRING(B_PIC_SET_FONT_FLAGS);
		RETURN_STRING(B_PIC_SET_FONT_SIZE);
		RETURN_STRING(B_PIC_SET_FONT_ROTATE);
		RETURN_STRING(B_PIC_SET_FONT_SHEAR);
		RETURN_STRING(B_PIC_SET_FONT_BPP);
		RETURN_STRING(B_PIC_SET_FONT_FACE);

		RETURN_STRING(B_PIC_AFFINE_TRANSLATE);
		RETURN_STRING(B_PIC_AFFINE_SCALE);
		RETURN_STRING(B_PIC_AFFINE_ROTATE);

		RETURN_STRING(B_PIC_BLEND_LAYER);

		default: return "Unknown op";
	}
	#undef RETURN_STRING
}
#endif


PicturePlayer::PicturePlayer(const void *data, size_t size, BList *pictures)
	:	fData(data),
		fSize(size),
		fPictures(pictures)
{
}


PicturePlayer::~PicturePlayer()
{
}


status_t
PicturePlayer::Play(void** callBackTable, int32 tableEntries, void* userData)
{
	const BPrivate::picture_player_callbacks kAdapterCallbacks = {
		move_pen_by,
		stroke_line,
		draw_rect,
		draw_round_rect,
		draw_bezier,
		draw_arc,
		draw_ellipse,
		draw_polygon,
		draw_shape,
		draw_string,
		draw_pixels,
		draw_picture,
		set_clipping_rects,
		clip_to_picture,
		push_state,
		pop_state,
		enter_state_change,
		exit_state_change,
		enter_font_state,
		exit_font_state,
		set_origin,
		set_pen_location,
		set_drawing_mode,
		set_line_mode,
		set_pen_size,
		set_fore_color,
		set_back_color,
		set_stipple_pattern,
		set_scale,
		set_font_family,
		set_font_style,
		set_font_spacing,
		set_font_size,
		set_font_rotation,
		set_font_encoding,
		set_font_flags,
		set_font_shear,
		set_font_face,
		set_blending_mode,
		set_transform,
		translate_by,
		scale_by,
		rotate_by,
		blend_layer,
		clip_to_rect,
		clip_to_shape,
		draw_string_locations
	};

	// We don't check if the functions in the table are NULL, but we
	// check the tableEntries to see if the table is big enough.
	// If an application supplies the wrong size or an invalid pointer,
	// it's its own fault.

	// If the caller supplied a function table smaller than needed,
	// we use our dummy table, and copy the supported ops from the supplied one.
	void *dummyTable[kOpsTableSize];

	adapter_context adapterContext;
	adapterContext.user_data = userData;
	adapterContext.function_table = callBackTable;

	if ((size_t)tableEntries < kOpsTableSize) {
		memcpy(dummyTable, callBackTable, tableEntries * sizeof(void*));
		for (size_t i = (size_t)tableEntries; i < kOpsTableSize; i++)
			dummyTable[i] = (void*)nop;

		adapterContext.function_table = dummyTable;
	}

	return _Play(kAdapterCallbacks, &adapterContext, fData, fSize, 0);
}


status_t
PicturePlayer::Play(const picture_player_callbacks& callbacks,
	size_t callbacksSize, void* userData)
{
	return _Play(callbacks, userData, fData, fSize, 0);
}


class DataReader {
public:
		DataReader(const void* buffer, size_t length)
			:
			fBuffer((const uint8*)buffer),
			fRemaining(length)
		{
		}

		size_t
		Remaining() const
		{
			return fRemaining;
		}

		template<typename T>
		bool
		Get(const T*& typed, size_t count = 1)
		{
			if (fRemaining < sizeof(T) * count)
				return false;

			typed = reinterpret_cast<const T *>(fBuffer);
			fRemaining -= sizeof(T) * count;
			fBuffer += sizeof(T) * count;
			return true;
		}

		template<typename T>
		bool
		GetRemaining(const T*& buffer, size_t& size)
		{
			if (fRemaining == 0)
				return false;

			buffer = reinterpret_cast<const T*>(fBuffer);
			size = fRemaining;
			fRemaining = 0;
			return true;
		}

private:
		const uint8*	fBuffer;
		size_t			fRemaining;
};


struct picture_data_entry_header {
	uint16 op;
	uint32 size;
} _PACKED;


status_t
PicturePlayer::_Play(const picture_player_callbacks& callbacks, void* userData,
	const void* buffer, size_t length, uint16 parentOp)
{
#if DEBUG
	printf("Start rendering %sBPicture...\n", parentOp != 0 ? "sub " : "");
	bigtime_t startTime = system_time();
	int32 numOps = 0;
#endif

	DataReader pictureReader(buffer, length);

	while (pictureReader.Remaining() > 0) {
		const picture_data_entry_header* header;
		const uint8* opData = NULL;
		if (!pictureReader.Get(header)
			|| !pictureReader.Get(opData, header->size)) {
			return B_BAD_DATA;
		}

		DataReader reader(opData, header->size);

		// Disallow ops that don't fit the parent.
		switch (parentOp) {
			case 0:
				// No parent op, no restrictions.
				break;

			case B_PIC_ENTER_STATE_CHANGE:
				if (header->op <= B_PIC_ENTER_STATE_CHANGE
					|| header->op > B_PIC_SET_TRANSFORM) {
					return B_BAD_DATA;
				}
				break;

			case B_PIC_ENTER_FONT_STATE:
				if (header->op < B_PIC_SET_FONT_FAMILY
					|| header->op > B_PIC_SET_FONT_FACE) {
					return B_BAD_DATA;
					}
				break;

			default:
				return B_BAD_DATA;
		}

#if DEBUG > 1
		bigtime_t startOpTime = system_time();
		printf("Op %s ", PictureOpToString(header->op));
#endif
		switch (header->op) {
			case B_PIC_MOVE_PEN_BY:
			{
				const BPoint* where;
				if (callbacks.move_pen_by == NULL || !reader.Get(where))
					break;

				callbacks.move_pen_by(userData, *where);
				break;
			}

			case B_PIC_STROKE_LINE:
			{
				const BPoint* start;
				const BPoint* end;
				if (callbacks.stroke_line == NULL || !reader.Get(start)
					|| !reader.Get(end)) {
					break;
				}

				callbacks.stroke_line(userData, *start, *end);
				break;
			}

			case B_PIC_STROKE_RECT:
			case B_PIC_FILL_RECT:
			{
				const BRect* rect;
				if (callbacks.draw_rect == NULL || !reader.Get(rect))
					break;

				callbacks.draw_rect(userData, *rect,
					header->op == B_PIC_FILL_RECT);
				break;
			}

			case B_PIC_STROKE_ROUND_RECT:
			case B_PIC_FILL_ROUND_RECT:
			{
				const BRect* rect;
				const BPoint* radii;
				if (callbacks.draw_round_rect == NULL || !reader.Get(rect)
					|| !reader.Get(radii)) {
					break;
				}

				callbacks.draw_round_rect(userData, *rect, *radii,
					header->op == B_PIC_FILL_ROUND_RECT);
				break;
			}

			case B_PIC_STROKE_BEZIER:
			case B_PIC_FILL_BEZIER:
			{
				const size_t kNumControlPoints = 4;
				const BPoint* controlPoints;
				if (callbacks.draw_bezier == NULL
					|| !reader.Get(controlPoints, kNumControlPoints)) {
					break;
				}

				callbacks.draw_bezier(userData, kNumControlPoints,
					controlPoints, header->op == B_PIC_FILL_BEZIER);
				break;
			}

			case B_PIC_STROKE_ARC:
			case B_PIC_FILL_ARC:
			{
				const BPoint* center;
				const BPoint* radii;
				const float* startTheta;
				const float* arcTheta;
				if (callbacks.draw_arc == NULL || !reader.Get(center)
					|| !reader.Get(radii) || !reader.Get(startTheta)
					|| !reader.Get(arcTheta)) {
					break;
				}

				callbacks.draw_arc(userData, *center, *radii, *startTheta,
					*arcTheta, header->op == B_PIC_FILL_ARC);
				break;
			}

			case B_PIC_STROKE_ELLIPSE:
			case B_PIC_FILL_ELLIPSE:
			{
				const BRect* rect;
				if (callbacks.draw_ellipse == NULL || !reader.Get(rect))
					break;

				callbacks.draw_ellipse(userData, *rect,
					header->op == B_PIC_FILL_ELLIPSE);
				break;
			}

			case B_PIC_STROKE_POLYGON:
			case B_PIC_FILL_POLYGON:
			{
				const uint32* numPoints;
				const BPoint* points;
				if (callbacks.draw_polygon == NULL || !reader.Get(numPoints)
					|| !reader.Get(points, *numPoints)) {
					break;
				}

				bool isClosed = true;
				const bool* closedPointer;
				if (header->op != B_PIC_FILL_POLYGON) {
					if (!reader.Get(closedPointer))
						break;

					isClosed = *closedPointer;
				}

				callbacks.draw_polygon(userData, *numPoints, points, isClosed,
					header->op == B_PIC_FILL_POLYGON);
				break;
			}

			case B_PIC_STROKE_SHAPE:
			case B_PIC_FILL_SHAPE:
			{
				const uint32* opCount;
				const uint32* pointCount;
				const uint32* opList;
				const BPoint* pointList;
				if (callbacks.draw_shape == NULL || !reader.Get(opCount)
					|| !reader.Get(pointCount) || !reader.Get(opList, *opCount)
					|| !reader.Get(pointList, *pointCount)) {
					break;
				}

				// TODO: remove BShape data copying
				BShape shape;
				shape.SetData(*opCount, *pointCount, opList, pointList);

				callbacks.draw_shape(userData, shape,
					header->op == B_PIC_FILL_SHAPE);
				break;
			}

			case B_PIC_DRAW_STRING:
			{
				const float* escapementSpace;
				const float* escapementNonSpace;
				const char* string;
				size_t length;
				if (callbacks.draw_string == NULL
					|| !reader.Get(escapementSpace)
					|| !reader.Get(escapementNonSpace)
					|| !reader.GetRemaining(string, length)) {
					break;
				}

				callbacks.draw_string(userData, string, length,
					*escapementSpace, *escapementNonSpace);
				break;
			}

			case B_PIC_DRAW_STRING_LOCATIONS:
			{
				const uint32* pointCount;
				const BPoint* pointList;
				const char* string;
				size_t length;
				if (callbacks.draw_string_locations == NULL
					|| !reader.Get(pointCount)
					|| !reader.Get(pointList, *pointCount)
					|| !reader.GetRemaining(string, length)) {
					break;
				}

				callbacks.draw_string_locations(userData, string, length,
					pointList, *pointCount);
				break;
			}

			case B_PIC_DRAW_PIXELS:
			{
				const BRect* sourceRect;
				const BRect* destinationRect;
				const uint32* width;
				const uint32* height;
				const uint32* bytesPerRow;
				const uint32* colorSpace;
				const uint32* flags;
				const void* data;
				size_t length;
				if (callbacks.draw_pixels == NULL || !reader.Get(sourceRect)
					|| !reader.Get(destinationRect) || !reader.Get(width)
					|| !reader.Get(height) || !reader.Get(bytesPerRow)
					|| !reader.Get(colorSpace) || !reader.Get(flags)
					|| !reader.GetRemaining(data, length)) {
					break;
				}

				callbacks.draw_pixels(userData, *sourceRect, *destinationRect,
					*width, *height, *bytesPerRow, (color_space)*colorSpace,
					*flags, data, length);
				break;
			}

			case B_PIC_DRAW_PICTURE:
			{
				const BPoint* where;
				const int32* token;
				if (callbacks.draw_picture == NULL || !reader.Get(where)
					|| !reader.Get(token)) {
					break;
				}

				callbacks.draw_picture(userData, *where, *token);
				break;
			}

			case B_PIC_SET_CLIPPING_RECTS:
			{
				const uint32* numRects;
				const BRect* rects;
				if (callbacks.set_clipping_rects == NULL
					|| !reader.Get(numRects) || !reader.Get(rects, *numRects)) {
					break;
				}

				callbacks.set_clipping_rects(userData, *numRects, rects);
				break;
			}

			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				if (callbacks.set_clipping_rects == NULL)
					break;

				callbacks.set_clipping_rects(userData, 0, NULL);
				break;
			}

			case B_PIC_CLIP_TO_PICTURE:
			{
				const int32* token;
				const BPoint* where;
				const bool* inverse;
				if (callbacks.clip_to_picture == NULL || !reader.Get(token)
					|| !reader.Get(where) || !reader.Get(inverse))
					break;

				callbacks.clip_to_picture(userData, *token, *where, *inverse);
				break;
			}

			case B_PIC_PUSH_STATE:
			{
				if (callbacks.push_state == NULL)
					break;

				callbacks.push_state(userData);
				break;
			}

			case B_PIC_POP_STATE:
			{
				if (callbacks.pop_state == NULL)
					break;

				callbacks.pop_state(userData);
				break;
			}

			case B_PIC_ENTER_STATE_CHANGE:
			case B_PIC_ENTER_FONT_STATE:
			{
				const void* data;
				size_t length;
				if (!reader.GetRemaining(data, length))
					break;

				if (header->op == B_PIC_ENTER_STATE_CHANGE) {
					if (callbacks.enter_state_change != NULL)
						callbacks.enter_state_change(userData);
				} else if (callbacks.enter_font_state != NULL)
					callbacks.enter_font_state(userData);

				status_t result = _Play(callbacks, userData, data, length,
					header->op);
				if (result != B_OK)
					return result;

				if (header->op == B_PIC_ENTER_STATE_CHANGE) {
					if (callbacks.exit_state_change != NULL)
						callbacks.exit_state_change(userData);
				} else if (callbacks.exit_font_state != NULL)
					callbacks.exit_font_state(userData);

				break;
			}

			case B_PIC_SET_ORIGIN:
			{
				const BPoint* origin;
				if (callbacks.set_origin == NULL || !reader.Get(origin))
					break;

				callbacks.set_origin(userData, *origin);
				break;
			}

			case B_PIC_SET_PEN_LOCATION:
			{
				const BPoint* location;
				if (callbacks.set_pen_location == NULL || !reader.Get(location))
					break;

				callbacks.set_pen_location(userData, *location);
				break;
			}

			case B_PIC_SET_DRAWING_MODE:
			{
				const uint16* mode;
				if (callbacks.set_drawing_mode == NULL || !reader.Get(mode))
					break;

				callbacks.set_drawing_mode(userData, (drawing_mode)*mode);
				break;
			}

			case B_PIC_SET_LINE_MODE:
			{
				const uint16* capMode;
				const uint16* joinMode;
				const float* miterLimit;
				if (callbacks.set_line_mode == NULL || !reader.Get(capMode)
					|| !reader.Get(joinMode) || !reader.Get(miterLimit)) {
					break;
				}

				callbacks.set_line_mode(userData, (cap_mode)*capMode,
					(join_mode)*joinMode, *miterLimit);
				break;
			}

			case B_PIC_SET_PEN_SIZE:
			{
				const float* penSize;
				if (callbacks.set_pen_size == NULL || !reader.Get(penSize))
					break;

				callbacks.set_pen_size(userData, *penSize);
				break;
			}

			case B_PIC_SET_FORE_COLOR:
			{
				const rgb_color* color;
				if (callbacks.set_fore_color == NULL || !reader.Get(color))
					break;

				callbacks.set_fore_color(userData, *color);
				break;
			}

			case B_PIC_SET_BACK_COLOR:
			{
				const rgb_color* color;
				if (callbacks.set_back_color == NULL || !reader.Get(color))
					break;

				callbacks.set_back_color(userData, *color);
				break;
			}

			case B_PIC_SET_STIPLE_PATTERN:
			{
				const pattern* stipplePattern;
				if (callbacks.set_stipple_pattern == NULL
					|| !reader.Get(stipplePattern)) {
					break;
				}

				callbacks.set_stipple_pattern(userData, *stipplePattern);
				break;
			}

			case B_PIC_SET_SCALE:
			{
				const float* scale;
				if (callbacks.set_scale == NULL || !reader.Get(scale))
					break;

				callbacks.set_scale(userData, *scale);
				break;
			}

			case B_PIC_SET_FONT_FAMILY:
			{
				const char* family;
				size_t length;
				if (callbacks.set_font_family == NULL
					|| !reader.GetRemaining(family, length)) {
					break;
				}

				callbacks.set_font_family(userData, family, length);
				break;
			}

			case B_PIC_SET_FONT_STYLE:
			{
				const char* style;
				size_t length;
				if (callbacks.set_font_style == NULL
					|| !reader.GetRemaining(style, length)) {
					break;
				}

				callbacks.set_font_style(userData, style, length);
				break;
			}

			case B_PIC_SET_FONT_SPACING:
			{
				const uint32* spacing;
				if (callbacks.set_font_spacing == NULL || !reader.Get(spacing))
					break;

				callbacks.set_font_spacing(userData, *spacing);
				break;
			}

			case B_PIC_SET_FONT_SIZE:
			{
				const float* size;
				if (callbacks.set_font_size == NULL || !reader.Get(size))
					break;

				callbacks.set_font_size(userData, *size);
				break;
			}

			case B_PIC_SET_FONT_ROTATE:
			{
				const float* rotation;
				if (callbacks.set_font_rotation == NULL
					|| !reader.Get(rotation)) {
					break;
				}

				callbacks.set_font_rotation(userData, *rotation);
				break;
			}

			case B_PIC_SET_FONT_ENCODING:
			{
				const uint32* encoding;
				if (callbacks.set_font_encoding == NULL
					|| !reader.Get(encoding)) {
					break;
				}

				callbacks.set_font_encoding(userData, *encoding);
				break;
			}

			case B_PIC_SET_FONT_FLAGS:
			{
				const uint32* flags;
				if (callbacks.set_font_flags == NULL || !reader.Get(flags))
					break;

				callbacks.set_font_flags(userData, *flags);
				break;
			}

			case B_PIC_SET_FONT_SHEAR:
			{
				const float* shear;
				if (callbacks.set_font_shear == NULL || !reader.Get(shear))
					break;

				callbacks.set_font_shear(userData, *shear);
				break;
			}

			case B_PIC_SET_FONT_FACE:
			{
				const uint32* face;
				if (callbacks.set_font_face == NULL || !reader.Get(face))
					break;

				callbacks.set_font_face(userData, *face);
				break;
			}

			case B_PIC_SET_BLENDING_MODE:
			{
				const uint16* alphaSourceMode;
				const uint16* alphaFunctionMode;
				if (callbacks.set_blending_mode == NULL
					|| !reader.Get(alphaSourceMode)
					|| !reader.Get(alphaFunctionMode)) {
					break;
				}

				callbacks.set_blending_mode(userData,
					(source_alpha)*alphaSourceMode,
					(alpha_function)*alphaFunctionMode);
				break;
			}

			case B_PIC_SET_TRANSFORM:
			{
				const BAffineTransform* transform;
				if (callbacks.set_transform == NULL || !reader.Get(transform))
					break;

				callbacks.set_transform(userData, *transform);
				break;
			}

			case B_PIC_AFFINE_TRANSLATE:
			{
				const double* x;
				const double* y;
				if (callbacks.translate_by == NULL || !reader.Get(x)
					|| !reader.Get(y)) {
					break;
				}

				callbacks.translate_by(userData, *x, *y);
				break;
			}

			case B_PIC_AFFINE_SCALE:
			{
				const double* x;
				const double* y;
				if (callbacks.scale_by == NULL || !reader.Get(x)
					|| !reader.Get(y)) {
					break;
				}

				callbacks.scale_by(userData, *x, *y);
				break;
			}

			case B_PIC_AFFINE_ROTATE:
			{
				const double* angleRadians;
				if (callbacks.rotate_by == NULL || !reader.Get(angleRadians))
					break;

				callbacks.rotate_by(userData, *angleRadians);
				break;
			}

			case B_PIC_BLEND_LAYER:
			{
				Layer* const* layer;
				if (callbacks.blend_layer == NULL || !reader.Get<Layer*>(layer))
					break;

				callbacks.blend_layer(userData, *layer);
				break;
			}

			case B_PIC_CLIP_TO_RECT:
			{
				const bool* inverse;
				const BRect* rect;

				if (callbacks.clip_to_rect == NULL || !reader.Get(inverse)
					|| !reader.Get(rect)) {
					break;
				}

				callbacks.clip_to_rect(userData, *rect, *inverse);
				break;
			}

			case B_PIC_CLIP_TO_SHAPE:
			{
				const bool* inverse;
				const uint32* opCount;
				const uint32* pointCount;
				const uint32* opList;
				const BPoint* pointList;
				if (callbacks.clip_to_shape == NULL || !reader.Get(inverse)
					|| !reader.Get(opCount) || !reader.Get(pointCount)
					|| !reader.Get(opList, *opCount)
					|| !reader.Get(pointList, *pointCount)) {
					break;
				}

				callbacks.clip_to_shape(userData, *opCount, opList,
					*pointCount, pointList, *inverse);
				break;
			}

			default:
				break;
		}

#if DEBUG
		numOps++;
#if DEBUG > 1
		printf("executed in %" B_PRId64 " usecs\n", system_time()
			- startOpTime);
#endif
#endif
	}

#if DEBUG
	printf("Done! %" B_PRId32 " ops, rendering completed in %" B_PRId64
		" usecs.\n", numOps, system_time() - startTime);
#endif
	return B_OK;
}
