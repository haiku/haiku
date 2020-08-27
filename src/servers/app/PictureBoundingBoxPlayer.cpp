/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Marcus Overhagen <marcus@overhagen.de>
 *      Julian Harnath <julian.harnath@rwth-aachen.de>
 */

#include "PictureBoundingBoxPlayer.h"

#include <new>
#include <stdio.h>

#include "DrawState.h"
#include "FontManager.h"
#include "Layer.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "View.h"
#include "Window.h"

#include <AutoDeleter.h>
#include <Bitmap.h>
#include <Debug.h>
#include <List.h>
#include <ObjectListPrivate.h>
#include <PicturePlayer.h>
#include <PictureProtocol.h>
#include <Shape.h>


//#define DEBUG_TRACE_BB
#ifdef DEBUG_TRACE_BB
#	define TRACE_BB(text, ...) debug_printf("PBBP: " text, ##__VA_ARGS__)
#else
#	define TRACE_BB(text, ...)
#endif


typedef PictureBoundingBoxPlayer::State BoundingBoxState;


// #pragma mark - PictureBoundingBoxPlayer::State


class PictureBoundingBoxPlayer::State {
public:
	State(const DrawState* drawState, BRect* boundingBox)
		:
		fDrawState(drawState->Squash()),
		fBoundingBox(boundingBox)
	{
		fBoundingBox->Set(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
	}

	~State()
	{
	}

	DrawState* GetDrawState()
	{
		return fDrawState.Get();
	}

	void PushDrawState()
	{
		DrawState* previousState = fDrawState.Detach();
		DrawState* newState = previousState->PushState();
		if (newState == NULL)
			newState = previousState;

		fDrawState.SetTo(newState);
	}

	void PopDrawState()
	{
		if (fDrawState->PreviousState() != NULL)
			fDrawState.SetTo(fDrawState->PopState());
	}

	SimpleTransform PenToLocalTransform() const
	{
		SimpleTransform transform;
		fDrawState->Transform(transform);
		return transform;
	}

	void IncludeRect(BRect& rect)
	{
		_AffineTransformRect(rect);
		*fBoundingBox = (*fBoundingBox) | rect;
	}

private:
	void _AffineTransformRect(BRect& rect)
	{
		BAffineTransform transform = fDrawState->CombinedTransform();
		if (transform.IsIdentity())
			return;

		BPoint transformedShape[4];
		transformedShape[0] = rect.LeftTop();
		transformedShape[1] = rect.LeftBottom();
		transformedShape[2] = rect.RightTop();
		transformedShape[3] = rect.RightBottom();

		transform.Apply(&transformedShape[0], 4);

		float minX = INT_MAX;
		float minY = INT_MAX;
		float maxX = INT_MIN;
		float maxY = INT_MIN;

		for (uint32 i = 0; i < 4; i++) {
			if (transformedShape[i].x < minX)
				minX = transformedShape[i].x;
			else if (transformedShape[i].x > maxX)
				maxX = transformedShape[i].x;
			if (transformedShape[i].y < minY)
				minY = transformedShape[i].y;
			else if (transformedShape[i].y > maxY)
				maxY = transformedShape[i].y;
		}

		rect.Set(minX, minY, maxX, maxY);
	}


private:
	ObjectDeleter<DrawState>
				fDrawState;
	BRect*		fBoundingBox;
};


// #pragma mark - Picture playback hooks


static void
get_polygon_frame(const BPoint* points, int32 numPoints, BRect* frame)
{
	ASSERT(numPoints > 0);

	float left = points->x;
	float top = points->y;
	float right = left;
	float bottom = top;

	points++;
	numPoints--;

	while (numPoints--) {
		if (points->x < left)
			left = points->x;
		if (points->x > right)
			right = points->x;
		if (points->y < top)
			top = points->y;
		if (points->y > bottom)
			bottom = points->y;
		points++;
	}

	frame->Set(left, top, right, bottom);
}


template<class RectType>
static void
expand_rect_for_pen_size(BoundingBoxState* state, RectType& rect)
{
	float penInset = -((state->GetDrawState()->PenSize() / 2.0f) + 1.0f);
	rect.InsetBy(penInset, penInset);
}


static void
move_pen_by(void* _state, const BPoint& delta)
{
	TRACE_BB("%p move pen by %.2f %.2f\n", _state, delta.x, delta.y);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetPenLocation(
		state->GetDrawState()->PenLocation() + delta);
}


static void
determine_bounds_stroke_line(void* _state, const BPoint& _start,
	const BPoint& _end)
{
	TRACE_BB("%p stroke line %.2f %.2f -> %.2f %.2f\n", _state,
		_start.x, _start.y, _end.x, _end.y);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BPoint start = _start;
	BPoint end = _end;

	const SimpleTransform transform = state->PenToLocalTransform();
	transform.Apply(&start);
	transform.Apply(&end);

	BRect rect;
	if (start.x <= end.x) {
		rect.left = start.x;
		rect.right = end.x;
	} else {
		rect.left = end.x;
		rect.right = start.x;
	}
	if (start.y <= end.y) {
		rect.top = start.y;
		rect.bottom = end.y;
	} else {
		rect.top = end.y;
		rect.bottom = start.y;
	}

	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);

	state->GetDrawState()->SetPenLocation(_end);
}


static void
determine_bounds_draw_rect(void* _state, const BRect& _rect, bool fill)
{
	TRACE_BB("%p draw rect fill=%d %.2f %.2f %.2f %.2f\n", _state, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BRect rect = _rect;
	state->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_draw_round_rect(void* _state, const BRect& _rect,
	const BPoint&, bool fill)
{
	determine_bounds_draw_rect(_state, _rect, fill);
}


static void
determine_bounds_bezier(BoundingBoxState* state, const BPoint* viewPoints,
	BRect& outRect)
{
	// Note: this is an approximation which results in a rectangle which
	// encloses all four control points. That will always enclose the curve,
	// although not necessarily tightly, but it's good enough for the purpose.
	// The exact bounding box of a bezier curve is not trivial to determine,
	// (need to calculate derivative of the curve) and we're going for
	// performance here.
	BPoint points[4];
	state->PenToLocalTransform().Apply(points, viewPoints, 4);
	BPoint topLeft = points[0];
	BPoint bottomRight = points[0];
	for (uint32 index = 1; index < 4; index++) {
		if (points[index].x < topLeft.x || points[index].y < topLeft.y)
			topLeft = points[index];
		if (points[index].x > topLeft.x || points[index].y > topLeft.y)
			bottomRight = points[index];
	}
	outRect.SetLeftTop(topLeft);
	outRect.SetRightBottom(bottomRight);
}


static void
determine_bounds_draw_bezier(void* _state, size_t numPoints,
	const BPoint viewPoints[], bool fill)
{
	TRACE_BB("%p draw bezier fill=%d (%.2f %.2f) (%.2f %.2f) "
		"(%.2f %.2f) (%.2f %.2f)\n",
		_state,
		fill,
		viewPoints[0].x, viewPoints[0].y,
		viewPoints[1].x, viewPoints[1].y,
		viewPoints[2].x, viewPoints[2].y,
		viewPoints[3].x, viewPoints[3].y);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	const size_t kSupportedPoints = 4;
	if (numPoints != kSupportedPoints)
		return;

	BRect rect;
	determine_bounds_bezier(state, viewPoints, rect);
	if (!fill)
		expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_draw_ellipse(void* _state, const BRect& _rect, bool fill)
{
	TRACE_BB("%p draw ellipse fill=%d (%.2f %.2f) (%.2f %.2f)\n", _state, fill,
		_rect.left, _rect.top, _rect.right, _rect.bottom);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BRect rect = _rect;
	state->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_draw_arc(void* _state, const BPoint& center,
	const BPoint& radii, float, float, bool fill)
{
	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	determine_bounds_draw_ellipse(_state, rect, fill);
}


static void
determine_bounds_polygon(BoundingBoxState* state, int32 numPoints,
	const BPoint* viewPoints, BRect& outRect)
{
	if (numPoints <= 0)
		return;

	if (numPoints <= 200) {
		// fast path: no malloc/free, also avoid
		// constructor/destructor calls
		char data[200 * sizeof(BPoint)];
		BPoint* points = (BPoint*)data;

		state->PenToLocalTransform().Apply(points, viewPoints, numPoints);
		get_polygon_frame(points, numPoints, &outRect);

	} else {
		 // avoid constructor/destructor calls by
		 // using malloc instead of new []
		BPoint* points = (BPoint*)malloc(numPoints * sizeof(BPoint));
		if (points == NULL)
			return;

		state->PenToLocalTransform().Apply(points, viewPoints, numPoints);
		get_polygon_frame(points, numPoints, &outRect);

		free(points);
	}
}


void
determine_bounds_draw_polygon(void* _state, size_t numPoints,
	const BPoint viewPoints[], bool, bool fill)
{
	TRACE_BB("%p draw polygon fill=%d (%ld points)\n", _state, fill, numPoints);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BRect rect;
	determine_bounds_polygon(state, numPoints, viewPoints, rect);
	if (!fill)
		expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_draw_shape(void* _state, const BShape& shape, bool fill)
{
	BRect rect = shape.Bounds();

	TRACE_BB("%p stroke shape (bounds %.2f %.2f %.2f %.2f)\n", _state,
		rect.left, rect.top, rect.right, rect.bottom);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->PenToLocalTransform().Apply(&rect);
	if (!fill)
		expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_draw_string(void* _state, const char* string, size_t length,
	float deltaSpace, float deltaNonSpace)
{
	TRACE_BB("%p string '%s'\n", _state, string);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font = state->GetDrawState()->Font();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	BRect rect;
	font.GetBoundingBoxesForStrings((char**)&string, &length, 1, &rect,
		B_SCREEN_METRIC, &delta);

	BPoint location = state->GetDrawState()->PenLocation();

	state->PenToLocalTransform().Apply(&location);
	rect.OffsetBy(location);
	state->IncludeRect(rect);

	state->PenToLocalTransform().Apply(&location);
	state->GetDrawState()->SetPenLocation(location);
}


static void
determine_bounds_draw_pixels(void* _state, const BRect&, const BRect& _dest,
	uint32, uint32, size_t, color_space, uint32, const void*, size_t)
{
	TRACE_BB("%p pixels (dest %.2f %.2f %.2f %.2f)\n", _state,
		_dest.left, _dest.top, _dest.right, _dest.bottom);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BRect dest = _dest;
	state->PenToLocalTransform().Apply(&dest);
	state->IncludeRect(dest);
}


static void
draw_picture(void* _state, const BPoint& where, int32 token)
{
	TRACE_BB("%p picture (unimplemented)\n", _state);

	// TODO
	(void)_state;
	(void)where;
	(void)token;
}


static void
set_clipping_rects(void* _state, size_t numRects, const BRect rects[])
{
	TRACE_BB("%p cliping rects (%ld rects)\n", _state, numRects);

	// TODO
	(void)_state;
	(void)rects;
	(void)numRects;
}


static void
clip_to_picture(void* _state, int32 pictureToken, const BPoint& where,
	bool clipToInverse)
{
	TRACE_BB("%p clip to picture (unimplemented)\n", _state);

	// TODO
}


static void
push_state(void* _state)
{
	TRACE_BB("%p push state\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->PushDrawState();
}


static void
pop_state(void* _state)
{
	TRACE_BB("%p pop state\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->PopDrawState();
}


static void
enter_state_change(void*)
{
}


static void
exit_state_change(void*)
{
}


static void
enter_font_state(void*)
{
}


static void
exit_font_state(void*)
{
}


static void
set_origin(void* _state, const BPoint& pt)
{
	TRACE_BB("%p set origin %.2f %.2f\n", _state, pt.x, pt.y);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	state->GetDrawState()->SetOrigin(pt);
}


static void
set_pen_location(void* _state, const BPoint& pt)
{
	TRACE_BB("%p set pen location %.2f %.2f\n", _state, pt.x, pt.y);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	state->GetDrawState()->SetPenLocation(pt);
}


static void
set_drawing_mode(void*, drawing_mode)
{
}


static void
set_line_mode(void* _state, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	DrawState* drawState = state->GetDrawState();
	drawState->SetLineCapMode(capMode);
	drawState->SetLineJoinMode(joinMode);
	drawState->SetMiterLimit(miterLimit);
}


static void
set_pen_size(void* _state, float size)
{
	TRACE_BB("%p set pen size %.2f\n", _state, size);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetPenSize(size);
}


static void
set_fore_color(void* _state, const rgb_color& color)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetHighColor(color);
}


static void
set_back_color(void* _state, const rgb_color& color)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetLowColor(color);
}


static void
set_stipple_pattern(void* _state, const pattern& _pattern)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetPattern(Pattern(_pattern));
}


static void
set_scale(void* _state, float scale)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	state->GetDrawState()->SetScale(scale);
}


static void
set_font_family(void* _state, const char* _family, size_t length)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BString family(_family, length);
	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	state->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_style(void* _state, const char* _style, size_t length)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BString style(_style, length);
	ServerFont font(state->GetDrawState()->Font());
	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);
	font.SetStyle(fontStyle);
	state->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_spacing(void* _state, uint8 spacing)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetSpacing(spacing);
	state->GetDrawState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(void* _state, float size)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetSize(size);
	state->GetDrawState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotate(void* _state, float rotation)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetRotation(rotation);
	state->GetDrawState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(void* _state, uint8 encoding)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetEncoding(encoding);
	state->GetDrawState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(void* _state, uint32 flags)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetFlags(flags);
	state->GetDrawState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(void* _state, float shear)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetShear(shear);
	state->GetDrawState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(void* _state, uint16 face)
{
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	ServerFont font;
	font.SetFace(face);
	state->GetDrawState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(void*, source_alpha, alpha_function)
{
}


static void
set_transform(void* _state, const BAffineTransform& transform)
{
	TRACE_BB("%p transform\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	state->GetDrawState()->SetTransform(transform);
}


static void
translate_by(void* _state, double x, double y)
{
	TRACE_BB("%p translate\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	BAffineTransform transform = state->GetDrawState()->Transform();
	transform.PreTranslateBy(x, y);
	state->GetDrawState()->SetTransform(transform);
}


static void
scale_by(void* _state, double x, double y)
{
	TRACE_BB("%p scale\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	BAffineTransform transform = state->GetDrawState()->Transform();
	transform.PreScaleBy(x, y);
	state->GetDrawState()->SetTransform(transform);
}


static void
rotate_by(void* _state, double angleRadians)
{
	TRACE_BB("%p rotate\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);
	BAffineTransform transform = state->GetDrawState()->Transform();
	transform.PreRotateBy(angleRadians);
	state->GetDrawState()->SetTransform(transform);
}


static void
determine_bounds_nested_layer(void* _state, Layer* layer)
{
	TRACE_BB("%p nested layer\n", _state);
	BoundingBoxState* const state =
		reinterpret_cast<BoundingBoxState*>(_state);

	BRect boundingBox;
	PictureBoundingBoxPlayer::Play(layer, state->GetDrawState(), &boundingBox);
	if (boundingBox.IsValid())
		state->IncludeRect(boundingBox);
}


static const BPrivate::picture_player_callbacks
	kPictureBoundingBoxPlayerCallbacks = {
	move_pen_by,
	determine_bounds_stroke_line,
	determine_bounds_draw_rect,
	determine_bounds_draw_round_rect,
	determine_bounds_draw_bezier,
	determine_bounds_draw_arc,
	determine_bounds_draw_ellipse,
	determine_bounds_draw_polygon,
	determine_bounds_draw_shape,
	determine_bounds_draw_string,
	determine_bounds_draw_pixels,
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
	set_font_rotate,
	set_font_encoding,
	set_font_flags,
	set_font_shear,
	set_font_face,
	set_blending_mode,
	set_transform,
	translate_by,
	scale_by,
	rotate_by,
	determine_bounds_nested_layer
};


// #pragma mark - PictureBoundingBoxPlayer


/* static */ void
PictureBoundingBoxPlayer::Play(ServerPicture* picture,
	const DrawState* drawState, BRect* outBoundingBox)
{
	State state(drawState, outBoundingBox);

	BMallocIO* mallocIO = dynamic_cast<BMallocIO*>(picture->fData.Get());
	if (mallocIO == NULL)
		return;

	BPrivate::PicturePlayer player(mallocIO->Buffer(),
		mallocIO->BufferLength(), ServerPicture::PictureList::Private(
			picture->fPictures.Get()).AsBList());
	player.Play(kPictureBoundingBoxPlayerCallbacks,
		sizeof(kPictureBoundingBoxPlayerCallbacks), &state);
}
