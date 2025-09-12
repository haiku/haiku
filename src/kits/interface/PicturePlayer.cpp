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
#include <DataIO.h>
#include <Gradient.h>
#include <PictureProtocol.h>
#include <Shape.h>
#include <ShapePrivate.h>

#include <AutoDeleter.h>
#include <StackOrHeapArray.h>


using BPrivate::PicturePlayer;
using BPrivate::PicturePlayerCallbacks;
using BPrivate::picture_player_callbacks_compat;


class CallbackAdapterPlayer : public PicturePlayerCallbacks {
public:
	CallbackAdapterPlayer(void* userData, void** functionTable);

	virtual void MovePenBy(const BPoint& where);
	virtual void StrokeLine(const BPoint& start, const BPoint& end);
	virtual void DrawRect(const BRect& rect, bool fill);
	virtual void DrawRoundRect(const BRect& rect, const BPoint& radii, bool fill);
	virtual void DrawBezier(const BPoint controlPoints[4], bool fill);
	virtual void DrawArc(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, bool fill);
	virtual void DrawEllipse(const BRect& rect, bool fill);
	virtual void DrawPolygon(size_t numPoints, const BPoint points[], bool isClosed, bool fill);
	virtual void DrawShape(const BShape& shape, bool fill);
	virtual void DrawString(const char* string, size_t length, float spaceEscapement,
		float nonSpaceEscapement);
	virtual void DrawPixels(const BRect& source, const BRect& destination, uint32 width,
		uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 flags, const void* data,
		size_t length);
	virtual void DrawPicture(const BPoint& where, int32 token);
	virtual void SetClippingRects(size_t numRects, const clipping_rect rects[]);
	virtual void ClipToPicture(int32 token, const BPoint& where, bool clipToInverse);
	virtual void PushState();
	virtual void PopState();
	virtual void EnterStateChange();
	virtual void ExitStateChange();
	virtual void EnterFontState();
	virtual void ExitFontState();
	virtual void SetOrigin(const BPoint& origin);
	virtual void SetPenLocation(const BPoint& location);
	virtual void SetDrawingMode(drawing_mode mode);
	virtual void SetLineMode(cap_mode capMode, join_mode joinMode, float miterLimit);
	virtual void SetPenSize(float size);
	virtual void SetForeColor(const rgb_color& color);
	virtual void SetBackColor(const rgb_color& color);
	virtual void SetStipplePattern(const pattern& patter);
	virtual void SetScale(float scale);
	virtual void SetFontFamily(const char* familyName, size_t length);
	virtual void SetFontStyle(const char* styleName, size_t length);
	virtual void SetFontSpacing(uint8 spacing);
	virtual void SetFontSize(float size);
	virtual void SetFontRotation(float rotation);
	virtual void SetFontEncoding(uint8 encoding);
	virtual void SetFontFlags(uint32 flags);
	virtual void SetFontShear(float shear);
	virtual void SetFontFace(uint16 face);
	virtual void SetBlendingMode(source_alpha alphaSourceMode, alpha_function alphaFunctionMode);
	virtual void SetTransform(const BAffineTransform& transform);
	virtual void TranslateBy(double x, double y);
	virtual void ScaleBy(double x, double y);
	virtual void RotateBy(double angleRadians);
	virtual void BlendLayer(Layer* layer);
	virtual void ClipToRect(const BRect& rect, bool inverse);
	virtual void ClipToShape(int32 opCount, const uint32 opList[], int32 ptCount,
		const BPoint ptList[], bool inverse);
	virtual void DrawStringLocations(const char* string, size_t length, const BPoint locations[],
		size_t locationCount);
	virtual void DrawRectGradient(const BRect& rect, BGradient& gradient, bool fill);
	virtual void DrawRoundRectGradient(const BRect& rect, const BPoint& radii, BGradient& gradient,
		bool fill);
	virtual void DrawBezierGradient(const BPoint controlPoints[4], BGradient& gradient, bool fill);
	virtual void DrawArcGradient(const BPoint& center, const BPoint& radii, float startTheta,
		float arcTheta, BGradient& gradient, bool fill);
	virtual void DrawEllipseGradient(const BRect& rect, BGradient& gradient, bool fill);
	virtual void DrawPolygonGradient(size_t numPoints, const BPoint points[], bool isClosed,
		BGradient& gradient, bool fill);
	virtual void DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill);
	virtual void SetFillRule(int32 fillRule);
	virtual void StrokeLineGradient(const BPoint& start, const BPoint& end,
		const BGradient& gradient);

private:
	void* fUserData;
	picture_player_callbacks_compat* fCallbacks;
};


static void
nop()
{
}


CallbackAdapterPlayer::CallbackAdapterPlayer(void* userData, void** functionTable)
	:
	fUserData(userData),
	fCallbacks((picture_player_callbacks_compat*)functionTable)
{
}


void
CallbackAdapterPlayer::MovePenBy(const BPoint& delta)
{
	fCallbacks->move_pen_by(fUserData, delta);
}


void
CallbackAdapterPlayer::StrokeLine(const BPoint& start, const BPoint& end)
{
	fCallbacks->stroke_line(fUserData, start, end);
}


void
CallbackAdapterPlayer::DrawRect(const BRect& rect, bool fill)
{
	if (fill)
		fCallbacks->fill_rect(fUserData, rect);
	else
		fCallbacks->stroke_rect(fUserData, rect);
}


void
CallbackAdapterPlayer::DrawRoundRect(const BRect& rect, const BPoint& radii,
	bool fill)
{
	if (fill)
		fCallbacks->fill_round_rect(fUserData, rect, radii);
	else
		fCallbacks->stroke_round_rect(fUserData, rect, radii);
}


void
CallbackAdapterPlayer::DrawBezier(const BPoint _points[4], bool fill)
{
	BPoint points[4] = { _points[0], _points[1], _points[2], _points[3] };

	if (fill)
		fCallbacks->fill_bezier(fUserData, points);
	else
		fCallbacks->stroke_bezier(fUserData, points);
}


void
CallbackAdapterPlayer::DrawArc(const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, bool fill)
{
	if (fill)
		fCallbacks->fill_arc(fUserData, center, radii, startTheta, arcTheta);
	else
		fCallbacks->stroke_arc(fUserData, center, radii, startTheta, arcTheta);
}


void
CallbackAdapterPlayer::DrawEllipse(const BRect& rect, bool fill)
{
	BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
	BPoint center = rect.LeftTop() + radii;

	if (fill)
		fCallbacks->fill_ellipse(fUserData, center, radii);
	else
		fCallbacks->stroke_ellipse(fUserData, center, radii);
}


void
CallbackAdapterPlayer::DrawPolygon(size_t numPoints, const BPoint _points[],
	bool isClosed, bool fill)
{

	BStackOrHeapArray<BPoint, 200> points(numPoints);
	if (!points.IsValid())
		return;

	memcpy((void*)points, _points, numPoints * sizeof(BPoint));

	if (fill)
		fCallbacks->fill_polygon(fUserData, numPoints, points, isClosed);
	else
		fCallbacks->stroke_polygon(fUserData, numPoints, points, isClosed);
}


void
CallbackAdapterPlayer::DrawShape(const BShape& shape, bool fill)
{
	if (fill)
		fCallbacks->fill_shape(fUserData, &shape);
	else
		fCallbacks->stroke_shape(fUserData, &shape);
}


void
CallbackAdapterPlayer::DrawString(const char* _string, size_t length,
	float deltaSpace, float deltaNonSpace)
{
	char* string = strndup(_string, length);

	fCallbacks->draw_string(fUserData, string, deltaSpace, deltaNonSpace);

	free(string);
}


void
CallbackAdapterPlayer::DrawPixels(const BRect& src, const BRect& dest, uint32 width,
	uint32 height, size_t bytesPerRow, color_space pixelFormat, uint32 options,
	const void* _data, size_t length)
{
	void* data = malloc(length);
	if (data == NULL)
		return;

	memcpy(data, _data, length);

	fCallbacks->draw_pixels(fUserData, src, dest, width,
			height, bytesPerRow, pixelFormat, options, data);

	free(data);
}


void
CallbackAdapterPlayer::DrawPicture(const BPoint& where, int32 token)
{
	fCallbacks->draw_picture(fUserData, where, token);
}


void
CallbackAdapterPlayer::SetClippingRects(size_t numRects, const clipping_rect _rects[])
{
	// This is rather ugly but works for such a trivial class.
	BStackOrHeapArray<BRect, 100> rects(numRects);
	if (!rects.IsValid())
		return;

	for (size_t i = 0; i < numRects; i++) {
		clipping_rect srcRect = _rects[i];
		rects[i] = BRect(srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
	}

	fCallbacks->set_clipping_rects(fUserData, rects, numRects);
}


void
CallbackAdapterPlayer::ClipToPicture(int32 token, const BPoint& origin,
	bool clipToInverse)
{
	fCallbacks->clip_to_picture(fUserData, token, origin, clipToInverse);
}


void
CallbackAdapterPlayer::PushState()
{
	fCallbacks->push_state(fUserData);
}


void
CallbackAdapterPlayer::PopState()
{
	fCallbacks->pop_state(fUserData);
}


void
CallbackAdapterPlayer::EnterStateChange()
{
	fCallbacks->enter_state_change(fUserData);
}


void
CallbackAdapterPlayer::ExitStateChange()
{
	fCallbacks->exit_state_change(fUserData);
}


void
CallbackAdapterPlayer::EnterFontState()
{
	fCallbacks->enter_font_state(fUserData);
}


void
CallbackAdapterPlayer::ExitFontState()
{
	fCallbacks->exit_font_state(fUserData);
}


void
CallbackAdapterPlayer::SetOrigin(const BPoint& origin)
{
	fCallbacks->set_origin(fUserData, origin);
}


void
CallbackAdapterPlayer::SetPenLocation(const BPoint& penLocation)
{
	fCallbacks->set_pen_location(fUserData, penLocation);
}


void
CallbackAdapterPlayer::SetDrawingMode(drawing_mode mode)
{
	fCallbacks->set_drawing_mode(fUserData, mode);
}


void
CallbackAdapterPlayer::SetLineMode(cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	fCallbacks->set_line_mode(fUserData, capMode, joinMode, miterLimit);
}


void
CallbackAdapterPlayer::SetPenSize(float size)
{
	fCallbacks->set_pen_size(fUserData, size);
}


void
CallbackAdapterPlayer::SetForeColor(const rgb_color& color)
{
	fCallbacks->set_fore_color(fUserData, color);
}


void
CallbackAdapterPlayer::SetBackColor(const rgb_color& color)
{
	fCallbacks->set_back_color(fUserData, color);
}


void
CallbackAdapterPlayer::SetStipplePattern(const pattern& stipplePattern)
{
	fCallbacks->set_stipple_pattern(fUserData, stipplePattern);
}


void
CallbackAdapterPlayer::SetScale(float scale)
{
	fCallbacks->set_scale(fUserData, scale);
}


void
CallbackAdapterPlayer::SetFontFamily(const char* _family, size_t length)
{
	char* family = strndup(_family, length);

	fCallbacks->set_font_family(fUserData, family);

	free(family);
}


void
CallbackAdapterPlayer::SetFontStyle(const char* _style, size_t length)
{
	char* style = strndup(_style, length);

	fCallbacks->set_font_style(fUserData, style);

	free(style);
}


void
CallbackAdapterPlayer::SetFontSpacing(uint8 spacing)
{
	fCallbacks->set_font_spacing(fUserData, spacing);
}


void
CallbackAdapterPlayer::SetFontSize(float size)
{
	fCallbacks->set_font_size(fUserData, size);
}


void
CallbackAdapterPlayer::SetFontRotation(float rotation)
{
	fCallbacks->set_font_rotate(fUserData, rotation);
}


void
CallbackAdapterPlayer::SetFontEncoding(uint8 encoding)
{
	fCallbacks->set_font_encoding(fUserData, encoding);
}


void
CallbackAdapterPlayer::SetFontFlags(uint32 flags)
{
	fCallbacks->set_font_flags(fUserData, flags);
}


void
CallbackAdapterPlayer::SetFontShear(float shear)
{
	fCallbacks->set_font_shear(fUserData, shear);
}


void
CallbackAdapterPlayer::SetFontFace(uint16 face)
{
	fCallbacks->set_font_face(fUserData, face);
}


void
CallbackAdapterPlayer::SetBlendingMode(source_alpha alphaSrcMode,
	alpha_function alphaFncMode)
{
	fCallbacks->set_blending_mode(fUserData, alphaSrcMode, alphaFncMode);
}


void
CallbackAdapterPlayer::SetTransform(const BAffineTransform& transform)
{
	fCallbacks->set_transform(fUserData, transform);
}


void
CallbackAdapterPlayer::TranslateBy(double x, double y)
{
	fCallbacks->translate_by(fUserData, x, y);
}


void
CallbackAdapterPlayer::ScaleBy(double x, double y)
{
	fCallbacks->scale_by(fUserData, x, y);
}


void
CallbackAdapterPlayer::RotateBy(double angleRadians)
{
	fCallbacks->rotate_by(fUserData, angleRadians);
}


void
CallbackAdapterPlayer::BlendLayer(Layer* layer)
{
	fCallbacks->blend_layer(fUserData, layer);
}


void
CallbackAdapterPlayer::ClipToRect(const BRect& rect, bool inverse)
{
	fCallbacks->clip_to_rect(fUserData, rect, inverse);
}


void
CallbackAdapterPlayer::ClipToShape(int32 opCount, const uint32 opList[],
	int32 ptCount, const BPoint ptList[], bool inverse)
{
	fCallbacks->clip_to_shape(fUserData, opCount, opList,
			ptCount, ptList, inverse);
}


void
CallbackAdapterPlayer::DrawStringLocations(const char* _string, size_t length,
	const BPoint* locations, size_t locationCount)
{
	char* string = strndup(_string, length);

	fCallbacks->draw_string_locations(fUserData, string, locations,
			locationCount);

	free(string);
}


void
CallbackAdapterPlayer::DrawRectGradient(const BRect& rect, BGradient& gradient, bool fill)
{
	if (fill)
		fCallbacks->fill_rect_gradient(fUserData, rect, gradient);
	else
		fCallbacks->stroke_rect_gradient(fUserData, rect, gradient);
}


void
CallbackAdapterPlayer::DrawRoundRectGradient(const BRect& rect, const BPoint& radii, BGradient& gradient,
	bool fill)
{
	if (fill)
		fCallbacks->fill_round_rect_gradient(fUserData, rect, radii, gradient);
	else
		fCallbacks->stroke_round_rect_gradient(fUserData, rect, radii, gradient);
}


void
CallbackAdapterPlayer::DrawBezierGradient(const BPoint _points[4], BGradient& gradient, bool fill)
{
	BPoint points[4] = { _points[0], _points[1], _points[2], _points[3] };

	if (fill)
		fCallbacks->fill_bezier_gradient(fUserData, points, gradient);
	else
		fCallbacks->stroke_bezier_gradient(fUserData, points, gradient);
}


void
CallbackAdapterPlayer::DrawArcGradient(const BPoint& center, const BPoint& radii,
	float startTheta, float arcTheta, BGradient& gradient, bool fill)
{
	if (fill)
		fCallbacks->fill_arc_gradient(fUserData, center, radii, startTheta, arcTheta, gradient);
	else
		fCallbacks->stroke_arc_gradient(fUserData, center, radii, startTheta, arcTheta, gradient);
}


void
CallbackAdapterPlayer::DrawEllipseGradient(const BRect& rect, BGradient& gradient, bool fill)
{
	BPoint radii((rect.Width() + 1) / 2.0f, (rect.Height() + 1) / 2.0f);
	BPoint center = rect.LeftTop() + radii;

	if (fill)
		fCallbacks->fill_ellipse_gradient(fUserData, center, radii, gradient);
	else
		fCallbacks->stroke_ellipse_gradient(fUserData, center, radii, gradient);
}


void
CallbackAdapterPlayer::DrawPolygonGradient(size_t numPoints, const BPoint _points[],
	bool isClosed, BGradient& gradient, bool fill)
{
	BStackOrHeapArray<BPoint, 200> points(numPoints);
	if (!points.IsValid())
		return;

	memcpy((void*)points, _points, numPoints * sizeof(BPoint));

	if (fill)
		fCallbacks->fill_polygon_gradient(fUserData, numPoints, points, isClosed, gradient);
	else
		fCallbacks->stroke_polygon_gradient(fUserData, numPoints, points, isClosed, gradient);
}


void
CallbackAdapterPlayer::DrawShapeGradient(const BShape& shape, BGradient& gradient, bool fill)
{
	if (fill)
		fCallbacks->fill_shape_gradient(fUserData, shape, gradient);
	else
		fCallbacks->stroke_shape_gradient(fUserData, shape, gradient);
}


void
CallbackAdapterPlayer::SetFillRule(int32 fillRule)
{
	fCallbacks->set_fill_rule(fUserData, fillRule);
}


void
CallbackAdapterPlayer::StrokeLineGradient(const BPoint& start, const BPoint& end, const BGradient& gradient)
{
	fCallbacks->stroke_line_gradient(fUserData, start, end, gradient);
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
		RETURN_STRING(B_PIC_STROKE_RECT_GRADIENT);
		RETURN_STRING(B_PIC_FILL_RECT_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_ROUND_RECT_GRADIENT);
		RETURN_STRING(B_PIC_FILL_ROUND_RECT_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_BEZIER_GRADIENT);
		RETURN_STRING(B_PIC_FILL_BEZIER_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_POLYGON_GRADIENT);
		RETURN_STRING(B_PIC_FILL_POLYGON_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_SHAPE_GRADIENT);
		RETURN_STRING(B_PIC_FILL_SHAPE_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_ARC_GRADIENT);
		RETURN_STRING(B_PIC_FILL_ARC_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_ELLIPSE_GRADIENT);
		RETURN_STRING(B_PIC_FILL_ELLIPSE_GRADIENT);
		RETURN_STRING(B_PIC_STROKE_LINE_GRADIENT);

		RETURN_STRING(B_PIC_ENTER_STATE_CHANGE);
		RETURN_STRING(B_PIC_SET_CLIPPING_RECTS);
		RETURN_STRING(B_PIC_CLIP_TO_PICTURE);
		RETURN_STRING(B_PIC_PUSH_STATE);
		RETURN_STRING(B_PIC_POP_STATE);
		RETURN_STRING(B_PIC_CLEAR_CLIPPING_RECTS);
		RETURN_STRING(B_PIC_CLIP_TO_RECT);
		RETURN_STRING(B_PIC_CLIP_TO_SHAPE);

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
		RETURN_STRING(B_PIC_SET_FILL_RULE);
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
	// We don't check if the functions in the table are NULL, but we
	// check the tableEntries to see if the table is big enough.
	// If an application supplies the wrong size or an invalid pointer,
	// it's its own fault.

	// If the caller supplied a function table smaller than needed,
	// we use our dummy table, and copy the supported ops from the supplied one.
	void *dummyTable[kOpsTableSize];

	if ((size_t)tableEntries < kOpsTableSize) {
		memcpy(dummyTable, callBackTable, tableEntries * sizeof(void*));
		for (size_t i = (size_t)tableEntries; i < kOpsTableSize; i++)
			dummyTable[i] = (void*)nop;

		callBackTable = dummyTable;
	}

	CallbackAdapterPlayer callbackAdapterPlayer(userData, callBackTable);
	return _Play(callbackAdapterPlayer, fData, fSize, 0);
}


status_t
PicturePlayer::Play(PicturePlayerCallbacks& callbacks)
{
	return _Play(callbacks, fData, fSize, 0);
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

		bool GetGradient(BGradient*& gradient)
		{
			BMemoryIO stream(fBuffer, fRemaining);
			printf("fRemaining: %ld\n", fRemaining);
			if (BGradient::Unflatten(gradient, &stream) != B_OK) {
				printf("BGradient::Unflatten(_gradient, &stream) != B_OK\n");
				return false;
			}

			fRemaining -= stream.Position();
			fBuffer += stream.Position();
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
PicturePlayer::_Play(PicturePlayerCallbacks& callbacks,
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
				if (!reader.Get(where))
					break;

				callbacks.MovePenBy(*where);
				break;
			}

			case B_PIC_STROKE_LINE:
			{
				const BPoint* start;
				const BPoint* end;
				if (!reader.Get(start) || !reader.Get(end))
					break;

				callbacks.StrokeLine(*start, *end);
				break;
			}

			case B_PIC_STROKE_RECT:
			case B_PIC_FILL_RECT:
			{
				const BRect* rect;
				if (!reader.Get(rect))
					break;

				callbacks.DrawRect(*rect, header->op == B_PIC_FILL_RECT);
				break;
			}

			case B_PIC_STROKE_ROUND_RECT:
			case B_PIC_FILL_ROUND_RECT:
			{
				const BRect* rect;
				const BPoint* radii;
				if (!reader.Get(rect) || !reader.Get(radii))
					break;

				callbacks.DrawRoundRect(*rect, *radii,
					header->op == B_PIC_FILL_ROUND_RECT);
				break;
			}

			case B_PIC_STROKE_BEZIER:
			case B_PIC_FILL_BEZIER:
			{
				const size_t kNumControlPoints = 4;
				const BPoint* controlPoints;
				if (!reader.Get(controlPoints, kNumControlPoints))
					break;

				callbacks.DrawBezier(controlPoints, header->op == B_PIC_FILL_BEZIER);
				break;
			}

			case B_PIC_STROKE_ARC:
			case B_PIC_FILL_ARC:
			{
				const BPoint* center;
				const BPoint* radii;
				const float* startTheta;
				const float* arcTheta;
				if (!reader.Get(center)
					|| !reader.Get(radii) || !reader.Get(startTheta)
					|| !reader.Get(arcTheta)) {
					break;
				}

				callbacks.DrawArc(*center, *radii, *startTheta,
					*arcTheta, header->op == B_PIC_FILL_ARC);
				break;
			}

			case B_PIC_STROKE_ELLIPSE:
			case B_PIC_FILL_ELLIPSE:
			{
				const BRect* rect;
				if (!reader.Get(rect))
					break;

				callbacks.DrawEllipse(*rect,
					header->op == B_PIC_FILL_ELLIPSE);
				break;
			}

			case B_PIC_STROKE_POLYGON:
			case B_PIC_FILL_POLYGON:
			{
				const uint32* numPoints;
				const BPoint* points;
				if (!reader.Get(numPoints) || !reader.Get(points, *numPoints))
					break;

				bool isClosed = true;
				const bool* closedPointer;
				if (header->op != B_PIC_FILL_POLYGON) {
					if (!reader.Get(closedPointer))
						break;

					isClosed = *closedPointer;
				}

				callbacks.DrawPolygon(*numPoints, points, isClosed,
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
				if (!reader.Get(opCount)
					|| !reader.Get(pointCount) || !reader.Get(opList, *opCount)
					|| !reader.Get(pointList, *pointCount)) {
					break;
				}

				// TODO: remove BShape data copying
				BShape shape;
				BShape::Private(shape).SetData(*opCount, *pointCount, opList, pointList);

				callbacks.DrawShape(shape, header->op == B_PIC_FILL_SHAPE);
				break;
			}

			case B_PIC_STROKE_RECT_GRADIENT:
			case B_PIC_FILL_RECT_GRADIENT:
			{
				const BRect* rect;
				BGradient* gradient;
				if (!reader.Get(rect) || !reader.GetGradient(gradient))
					break;
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawRectGradient(*rect, *gradient,
					header->op == B_PIC_FILL_RECT_GRADIENT);
				break;
			}

			case B_PIC_STROKE_ROUND_RECT_GRADIENT:
			case B_PIC_FILL_ROUND_RECT_GRADIENT:
			{
				const BRect* rect;
				const BPoint* radii;
				BGradient* gradient;
				if (!reader.Get(rect)
					|| !reader.Get(radii) || !reader.GetGradient(gradient)) {
					break;
				}
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawRoundRectGradient(*rect, *radii, *gradient,
					header->op == B_PIC_FILL_ROUND_RECT_GRADIENT);
				break;
			}

			case B_PIC_STROKE_BEZIER_GRADIENT:
			case B_PIC_FILL_BEZIER_GRADIENT:
			{
				const size_t kNumControlPoints = 4;
				const BPoint* controlPoints;
				BGradient* gradient;
				if (!reader.Get(controlPoints, kNumControlPoints) || !reader.GetGradient(gradient))
					break;
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawBezierGradient(controlPoints, *gradient,
					header->op == B_PIC_FILL_BEZIER_GRADIENT);
				break;
			}

			case B_PIC_STROKE_POLYGON_GRADIENT:
			case B_PIC_FILL_POLYGON_GRADIENT:
			{
				const uint32* numPoints;
				const BPoint* points;
				BGradient* gradient;
				if (!reader.Get(numPoints) || !reader.Get(points, *numPoints))
					break;

				bool isClosed = true;
				const bool* closedPointer;
				if (header->op != B_PIC_FILL_POLYGON_GRADIENT) {
					if (!reader.Get(closedPointer))
						break;

					isClosed = *closedPointer;
				}

				if (!reader.GetGradient(gradient))
					break;
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawPolygonGradient(*numPoints, points, isClosed, *gradient,
					header->op == B_PIC_FILL_POLYGON_GRADIENT);
				break;
			}

			case B_PIC_STROKE_SHAPE_GRADIENT:
			case B_PIC_FILL_SHAPE_GRADIENT:
			{
				const uint32* opCount;
				const uint32* pointCount;
				const uint32* opList;
				const BPoint* pointList;
				BGradient* gradient;
				if (!reader.Get(opCount)
					|| !reader.Get(pointCount) || !reader.Get(opList, *opCount)
					|| !reader.Get(pointList, *pointCount) || !reader.GetGradient(gradient)) {
					break;
				}
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				// TODO: remove BShape data copying
				BShape shape;
				BShape::Private(shape).SetData(*opCount, *pointCount, opList, pointList);

				callbacks.DrawShapeGradient(shape, *gradient,
					header->op == B_PIC_FILL_SHAPE_GRADIENT);
				break;
			}

			case B_PIC_STROKE_ARC_GRADIENT:
			case B_PIC_FILL_ARC_GRADIENT:
			{
				const BPoint* center;
				const BPoint* radii;
				const float* startTheta;
				const float* arcTheta;
				BGradient* gradient;
				if (!reader.Get(center)
					|| !reader.Get(radii) || !reader.Get(startTheta)
					|| !reader.Get(arcTheta) || !reader.GetGradient(gradient)) {
					break;
				}
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawArcGradient(*center, *radii, *startTheta,
					*arcTheta, *gradient, header->op == B_PIC_FILL_ARC_GRADIENT);
				break;
			}

			case B_PIC_STROKE_ELLIPSE_GRADIENT:
			case B_PIC_FILL_ELLIPSE_GRADIENT:
			{
				const BRect* rect;
				BGradient* gradient;
				if (!reader.Get(rect) || !reader.GetGradient(gradient))
					break;
				ObjectDeleter<BGradient> gradientDeleter(gradient);

				callbacks.DrawEllipseGradient(*rect, *gradient,
					header->op == B_PIC_FILL_ELLIPSE_GRADIENT);
				break;
			}

			case B_PIC_STROKE_LINE_GRADIENT:
			{
				const BPoint* start;
				const BPoint* end;
				BGradient* gradient;
				if (!reader.Get(start)
					|| !reader.Get(end) || !reader.GetGradient(gradient)) {
					break;
				}

				callbacks.StrokeLineGradient(*start, *end, *gradient);
				break;
			}

			case B_PIC_DRAW_STRING:
			{
				const int32* length;
				const char* string;
				const float* escapementSpace;
				const float* escapementNonSpace;
				if (!reader.Get(length)
					|| !reader.Get(string, *length)
					|| !reader.Get(escapementSpace)
					|| !reader.Get(escapementNonSpace)) {
					break;
				}

				callbacks.DrawString(string, *length,
					*escapementSpace, *escapementNonSpace);
				break;
			}

			case B_PIC_DRAW_STRING_LOCATIONS:
			{
				const uint32* pointCount;
				const BPoint* pointList;
				const int32* length;
				const char* string;
				if (!reader.Get(pointCount)
					|| !reader.Get(pointList, *pointCount)
					|| !reader.Get(length)
					|| !reader.Get(string, *length)) {
					break;
				}

				callbacks.DrawStringLocations(string, *length,
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
				if (!reader.Get(sourceRect)
					|| !reader.Get(destinationRect) || !reader.Get(width)
					|| !reader.Get(height) || !reader.Get(bytesPerRow)
					|| !reader.Get(colorSpace) || !reader.Get(flags)
					|| !reader.GetRemaining(data, length)) {
					break;
				}

				callbacks.DrawPixels(*sourceRect, *destinationRect,
					*width, *height, *bytesPerRow, (color_space)*colorSpace,
					*flags, data, length);
				break;
			}

			case B_PIC_DRAW_PICTURE:
			{
				const BPoint* where;
				const int32* token;
				if (!reader.Get(where) || !reader.Get(token))
					break;

				callbacks.DrawPicture(*where, *token);
				break;
			}

			case B_PIC_SET_CLIPPING_RECTS:
			{
				const clipping_rect* frame;
				if (!reader.Get(frame))
					break;

				uint32 numRects = reader.Remaining() / sizeof(clipping_rect);

				const clipping_rect* rects;
				if (!reader.Get(rects, numRects))
					break;

				callbacks.SetClippingRects(numRects, rects);
				break;
			}

			case B_PIC_CLEAR_CLIPPING_RECTS:
			{
				callbacks.SetClippingRects(0, NULL);
				break;
			}

			case B_PIC_CLIP_TO_PICTURE:
			{
				const int32* token;
				const BPoint* where;
				const bool* inverse;
				if (!reader.Get(token)
					|| !reader.Get(where) || !reader.Get(inverse))
					break;

				callbacks.ClipToPicture(*token, *where, *inverse);
				break;
			}

			case B_PIC_PUSH_STATE:
			{
				callbacks.PushState();
				break;
			}

			case B_PIC_POP_STATE:
			{
				callbacks.PopState();
				break;
			}

			case B_PIC_ENTER_STATE_CHANGE:
			case B_PIC_ENTER_FONT_STATE:
			{
				const void* data;
				size_t length;
				if (!reader.GetRemaining(data, length))
					break;

				if (header->op == B_PIC_ENTER_STATE_CHANGE)
					callbacks.EnterStateChange();
				else
					callbacks.EnterFontState();

				status_t result = _Play(callbacks, data, length,
					header->op);
				if (result != B_OK)
					return result;

				if (header->op == B_PIC_ENTER_STATE_CHANGE)
					callbacks.ExitStateChange();
				else
					callbacks.ExitFontState();

				break;
			}

			case B_PIC_SET_ORIGIN:
			{
				const BPoint* origin;
				if (!reader.Get(origin))
					break;

				callbacks.SetOrigin(*origin);
				break;
			}

			case B_PIC_SET_PEN_LOCATION:
			{
				const BPoint* location;
				if (!reader.Get(location))
					break;

				callbacks.SetPenLocation(*location);
				break;
			}

			case B_PIC_SET_DRAWING_MODE:
			{
				const uint16* mode;
				if (!reader.Get(mode))
					break;

				callbacks.SetDrawingMode((drawing_mode)*mode);
				break;
			}

			case B_PIC_SET_LINE_MODE:
			{
				const uint16* capMode;
				const uint16* joinMode;
				const float* miterLimit;
				if (!reader.Get(capMode)
					|| !reader.Get(joinMode) || !reader.Get(miterLimit)) {
					break;
				}

				callbacks.SetLineMode((cap_mode)*capMode,
					(join_mode)*joinMode, *miterLimit);
				break;
			}

			case B_PIC_SET_PEN_SIZE:
			{
				const float* penSize;
				if (!reader.Get(penSize))
					break;

				callbacks.SetPenSize(*penSize);
				break;
			}

			case B_PIC_SET_FORE_COLOR:
			{
				const rgb_color* color;
				if (!reader.Get(color))
					break;

				callbacks.SetForeColor(*color);
				break;
			}

			case B_PIC_SET_BACK_COLOR:
			{
				const rgb_color* color;
				if (!reader.Get(color))
					break;

				callbacks.SetBackColor(*color);
				break;
			}

			case B_PIC_SET_STIPLE_PATTERN:
			{
				const pattern* stipplePattern;
				if (!reader.Get(stipplePattern)) {
					break;
				}

				callbacks.SetStipplePattern(*stipplePattern);
				break;
			}

			case B_PIC_SET_SCALE:
			{
				const float* scale;
				if (!reader.Get(scale))
					break;

				callbacks.SetScale(*scale);
				break;
			}

			case B_PIC_SET_FONT_FAMILY:
			{
				const int32* length;
				const char* family;
				if (!reader.Get(length)
					|| !reader.Get(family, *length)) {
					break;
				}

				callbacks.SetFontFamily(family, *length);
				break;
			}

			case B_PIC_SET_FONT_STYLE:
			{
				const int32* length;
				const char* style;
				if (!reader.Get(length)
					|| !reader.Get(style, *length)) {
					break;
				}

				callbacks.SetFontStyle(style, *length);
				break;
			}

			case B_PIC_SET_FONT_SPACING:
			{
				const uint32* spacing;
				if (!reader.Get(spacing))
					break;

				callbacks.SetFontSpacing(*spacing);
				break;
			}

			case B_PIC_SET_FONT_SIZE:
			{
				const float* size;
				if (!reader.Get(size))
					break;

				callbacks.SetFontSize(*size);
				break;
			}

			case B_PIC_SET_FONT_ROTATE:
			{
				const float* rotation;
				if (!reader.Get(rotation))
					break;

				callbacks.SetFontRotation(*rotation);
				break;
			}

			case B_PIC_SET_FONT_ENCODING:
			{
				const uint32* encoding;
				if (!reader.Get(encoding))
					break;

				callbacks.SetFontEncoding(*encoding);
				break;
			}

			case B_PIC_SET_FONT_FLAGS:
			{
				const uint32* flags;
				if (!reader.Get(flags))
					break;

				callbacks.SetFontFlags(*flags);
				break;
			}

			case B_PIC_SET_FONT_SHEAR:
			{
				const float* shear;
				if (!reader.Get(shear))
					break;

				callbacks.SetFontShear(*shear);
				break;
			}

			case B_PIC_SET_FONT_FACE:
			{
				const uint32* face;
				if (!reader.Get(face))
					break;

				callbacks.SetFontFace(*face);
				break;
			}

			case B_PIC_SET_BLENDING_MODE:
			{
				const uint16* alphaSourceMode;
				const uint16* alphaFunctionMode;
				if (!reader.Get(alphaSourceMode)
					|| !reader.Get(alphaFunctionMode)) {
					break;
				}

				callbacks.SetBlendingMode(
					(source_alpha)*alphaSourceMode,
					(alpha_function)*alphaFunctionMode);
				break;
			}

			case B_PIC_SET_FILL_RULE:
			{
				const uint32* fillRule;
				if (!reader.Get(fillRule))
					break;

				callbacks.SetFillRule(*fillRule);
				break;
			}

			case B_PIC_SET_TRANSFORM:
			{
				const BAffineTransform* transform;
				if (!reader.Get(transform))
					break;

				callbacks.SetTransform(*transform);
				break;
			}

			case B_PIC_AFFINE_TRANSLATE:
			{
				const double* x;
				const double* y;
				if (!reader.Get(x) || !reader.Get(y))
					break;

				callbacks.TranslateBy(*x, *y);
				break;
			}

			case B_PIC_AFFINE_SCALE:
			{
				const double* x;
				const double* y;
				if (!reader.Get(x) || !reader.Get(y))
					break;

				callbacks.ScaleBy(*x, *y);
				break;
			}

			case B_PIC_AFFINE_ROTATE:
			{
				const double* angleRadians;
				if (!reader.Get(angleRadians))
					break;

				callbacks.RotateBy(*angleRadians);
				break;
			}

			case B_PIC_BLEND_LAYER:
			{
				Layer* const* layer;
				if (!reader.Get<Layer*>(layer))
					break;

				callbacks.BlendLayer(*layer);
				break;
			}

			case B_PIC_CLIP_TO_RECT:
			{
				const bool* inverse;
				const BRect* rect;

				if (!reader.Get(inverse) || !reader.Get(rect))
					break;

				callbacks.ClipToRect(*rect, *inverse);
				break;
			}

			case B_PIC_CLIP_TO_SHAPE:
			{
				const bool* inverse;
				const uint32* opCount;
				const uint32* pointCount;
				const uint32* opList;
				const BPoint* pointList;
				if (!reader.Get(inverse)
					|| !reader.Get(opCount) || !reader.Get(pointCount)
					|| !reader.Get(opList, *opCount)
					|| !reader.Get(pointList, *pointCount)) {
					break;
				}

				callbacks.ClipToShape(*opCount, opList,
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
