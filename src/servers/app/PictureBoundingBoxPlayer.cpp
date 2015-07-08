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
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerFont.h"
#include "ServerPicture.h"
#include "ServerTokenSpace.h"
#include "View.h"
#include "Window.h"

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
	State(DrawState* drawState, BRect* boundingBox)
		:
		fDrawState(drawState->Squash()),
		fBoundingBox(boundingBox)
	{
		fBoundingBox->Set(INT_MAX, INT_MAX, 0, 0);
	}

	~State()
	{
		delete fDrawState;
	}

	DrawState* GetDrawState()
	{
		return fDrawState;
	}

	void PushDrawState()
	{
		DrawState* nextState = fDrawState->PushState();
		if (nextState != NULL)
			fDrawState = nextState;
	}

	void PopDrawState()
	{
		if (fDrawState->PreviousState() != NULL)
			fDrawState = fDrawState->PopState();
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
		BAffineTransform transform = fDrawState->Transform();
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
		float maxX = 0;
		float maxY = 0;

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
	DrawState*	fDrawState;
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
nop()
{
}


static void
move_pen_by(BoundingBoxState* state, BPoint delta)
{
	TRACE_BB("%p move pen by %.2f %.2f\n", state, delta.x, delta.y);

	state->GetDrawState()->SetPenLocation(
		state->GetDrawState()->PenLocation() + delta);
}


static void
determine_bounds_stroke_line(BoundingBoxState* state, BPoint start, BPoint end)
{
	TRACE_BB("%p stroke line %.2f %.2f -> %.2f %.2f\n", state,
		start.x, start.y, end.x, end.y);

	BPoint penPos = end;

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

	state->GetDrawState()->SetPenLocation(penPos);
}


static void
determine_bounds_stroke_rect(BoundingBoxState* state, BRect rect)
{
	TRACE_BB("%p stroke rect %.2f %.2f %.2f %.2f\n", state,
		rect.left, rect.top, rect.right, rect.bottom);

	state->PenToLocalTransform().Apply(&rect);
	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_fill_rect(BoundingBoxState* state, BRect rect)
{
	TRACE_BB("%p fill rect %.2f %.2f %.2f %.2f\n", state,
		rect.left, rect.top, rect.right, rect.bottom);

	state->PenToLocalTransform().Apply(&rect);
	state->IncludeRect(rect);
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
determine_bounds_stroke_bezier(BoundingBoxState* state, const BPoint* viewPoints)
{
	TRACE_BB("%p stroke bezier (%.2f %.2f) (%.2f %.2f) (%.2f %.2f) (%.2f %.2f)\n",
		state,
		viewPoints[0].x, viewPoints[0].y,
		viewPoints[1].x, viewPoints[1].y,
		viewPoints[2].x, viewPoints[2].y,
		viewPoints[3].x, viewPoints[3].y);

	BRect rect;
	determine_bounds_bezier(state, viewPoints, rect);
	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_fill_bezier(BoundingBoxState* state, const BPoint* viewPoints)
{
	TRACE_BB("%p fill bezier (%.2f %.2f) (%.2f %.2f) (%.2f %.2f) (%.2f %.2f)\n",
		state,
		viewPoints[0].x, viewPoints[0].y,
		viewPoints[1].x, viewPoints[1].y,
		viewPoints[2].x, viewPoints[2].y,
		viewPoints[3].x, viewPoints[3].y);

	BRect rect;
	determine_bounds_bezier(state, viewPoints, rect);
	state->IncludeRect(rect);
}



static void
determine_bounds_stroke_ellipse(BoundingBoxState* state, BPoint center, BPoint radii)
{
	TRACE_BB("%p stroke ellipse (%.2f %.2f) (%.2f %.2f)\n", state,
		center.x, center.y, radii.x, radii.y);

	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);
	state->PenToLocalTransform().Apply(&rect);
	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_fill_ellipse(BoundingBoxState* state, BPoint center, BPoint radii)
{
	TRACE_BB("%p fill ellipse (%.2f %.2f) (%.2f %.2f)\n", state,
		center.x, center.y, radii.x, radii.y);

	BRect rect(center.x - radii.x, center.y - radii.y,
		center.x + radii.x - 1, center.y + radii.y - 1);

	TRACE_BB("  --> (%.2f %.2f %.2f %.2f)\n",
		rect.left, rect.top, rect.right, rect.bottom);

	state->PenToLocalTransform().Apply(&rect);
	state->IncludeRect(rect);
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
determine_bounds_stroke_polygon(BoundingBoxState* state, int32 numPoints,
	const BPoint* viewPoints, bool)
{
	TRACE_BB("%p stroke polygon (%ld points)\n", state, numPoints);

	BRect rect;
	determine_bounds_polygon(state, numPoints, viewPoints, rect);
	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


void
determine_bounds_fill_polygon(BoundingBoxState* state, int32 numPoints,
	const BPoint* viewPoints, bool)
{
	TRACE_BB("%p fill polygon (%ld points)\n", state, numPoints);

	BRect rect;
	determine_bounds_polygon(state, numPoints, viewPoints, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_stroke_shape(BoundingBoxState* state, const BShape* shape)
{
	BRect rect = shape->Bounds();

	TRACE_BB("%p stroke shape (bounds %.2f %.2f %.2f %.2f)\n", state,
		rect.left, rect.top, rect.right, rect.bottom);

	state->PenToLocalTransform().Apply(&rect);
	expand_rect_for_pen_size(state, rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_fill_shape(BoundingBoxState* state, const BShape* shape)
{
	BRect rect = shape->Bounds();

	TRACE_BB("%p fill shape (bounds %.2f %.2f %.2f %.2f)\n", state,
		rect.left, rect.top, rect.right, rect.bottom);

	state->PenToLocalTransform().Apply(&rect);
	state->IncludeRect(rect);
}


static void
determine_bounds_string(BoundingBoxState* state, const char* string, float deltaSpace,
	float deltaNonSpace)
{
	TRACE_BB("%p string '%s'\n", state, string);

	ServerFont font = state->GetDrawState()->Font();

	escapement_delta delta = { deltaSpace, deltaNonSpace };
	BRect rect;
	int32 length = strlen(string);
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
determine_bounds_pixels(BoundingBoxState* state, BRect, BRect dest, int32 w, int32 h,
	int32 bpr, int32 pf, int32, const void*)
{
	TRACE_BB("%p pixels (dest %.2f %.2f %.2f %.2f) w=%ld h=%ld bpr=%ld pf=%ld\n", state,
		dest.left, dest.top, dest.right, dest.bottom,
		w, h, bpr, pf);
		// TODO remove params

	state->PenToLocalTransform().Apply(&dest);
	state->IncludeRect(dest);
}


static void
draw_picture(BoundingBoxState* state, BPoint where, int32 token)
{
	TRACE_BB("%p picture (unimplemented)\n", state);

	// TODO
	(void)state;
	(void)where;
	(void)token;
}


static void
set_clipping_rects(BoundingBoxState* state, const BRect* rects,
	uint32 numRects)
{
	TRACE_BB("%p cliping rects (%ld rects)\n", state, numRects);

	// TODO
	(void)state;
	(void)rects;
	(void)numRects;
}


static void
clip_to_picture(BoundingBoxState* state, BPicture* picture, BPoint pt,
	bool clipToInverse)
{
	TRACE_BB("%p clip to picture (unimplemented)\n", state);

	// TODO
	printf("ClipToPicture(picture, BPoint(%.2f, %.2f), %s)\n",
		pt.x, pt.y, clipToInverse ? "inverse" : "");
}


static void
push_state(BoundingBoxState* state)
{
	TRACE_BB("%p push state\n", state);
	state->PushDrawState();
}


static void
pop_state(BoundingBoxState* state)
{
	TRACE_BB("%p pop state\n", state);
	state->PopDrawState();
}


static void
enter_state_change(BoundingBoxState*)
{
}


static void
exit_state_change(BoundingBoxState*)
{
}


static void
enter_font_state(BoundingBoxState*)
{
}


static void
exit_font_state(BoundingBoxState*)
{
}


static void
set_origin(BoundingBoxState* state, BPoint pt)
{
	TRACE_BB("%p set origin %.2f %.2f\n", state, pt.x, pt.y);
	state->GetDrawState()->SetOrigin(pt);
}


static void
set_pen_location(BoundingBoxState* state, BPoint pt)
{
	TRACE_BB("%p set pen location %.2f %.2f\n", state, pt.x, pt.y);
	state->GetDrawState()->SetPenLocation(pt);
}


static void
set_drawing_mode(BoundingBoxState*, drawing_mode)
{
}


static void
set_line_mode(BoundingBoxState* state, cap_mode capMode, join_mode joinMode,
	float miterLimit)
{
	DrawState* drawState = state->GetDrawState();
	drawState->SetLineCapMode(capMode);
	drawState->SetLineJoinMode(joinMode);
	drawState->SetMiterLimit(miterLimit);
}


static void
set_pen_size(BoundingBoxState* state, float size)
{
	TRACE_BB("%p set pen size %.2f\n", state, size);
	state->GetDrawState()->SetPenSize(size);
}


static void
set_fore_color(BoundingBoxState* state, rgb_color color)
{
	state->GetDrawState()->SetHighColor(color);
}


static void
set_back_color(BoundingBoxState* state, rgb_color color)
{
	state->GetDrawState()->SetLowColor(color);
}


static void
set_stipple_pattern(BoundingBoxState* state, pattern p)
{
	state->GetDrawState()->SetPattern(Pattern(p));
}


static void
set_scale(BoundingBoxState* state, float scale)
{
	state->GetDrawState()->SetScale(scale);
}


static void
set_font_family(BoundingBoxState* state, const char* family)
{
	FontStyle* fontStyle = gFontManager->GetStyleByIndex(family, 0);
	ServerFont font;
	font.SetStyle(fontStyle);
	state->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_style(BoundingBoxState* state, const char* style)
{
	ServerFont font(state->GetDrawState()->Font());
	FontStyle* fontStyle = gFontManager->GetStyle(font.Family(), style);
	font.SetStyle(fontStyle);
	state->GetDrawState()->SetFont(font, B_FONT_FAMILY_AND_STYLE);
}


static void
set_font_spacing(BoundingBoxState* state, int32 spacing)
{
	ServerFont font;
	font.SetSpacing(spacing);
	state->GetDrawState()->SetFont(font, B_FONT_SPACING);
}


static void
set_font_size(BoundingBoxState* state, float size)
{
	ServerFont font;
	font.SetSize(size);
	state->GetDrawState()->SetFont(font, B_FONT_SIZE);
}


static void
set_font_rotate(BoundingBoxState* state, float rotation)
{
	ServerFont font;
	font.SetRotation(rotation);
	state->GetDrawState()->SetFont(font, B_FONT_ROTATION);
}


static void
set_font_encoding(BoundingBoxState* state, int32 encoding)
{
	ServerFont font;
	font.SetEncoding(encoding);
	state->GetDrawState()->SetFont(font, B_FONT_ENCODING);
}


static void
set_font_flags(BoundingBoxState* state, int32 flags)
{
	ServerFont font;
	font.SetFlags(flags);
	state->GetDrawState()->SetFont(font, B_FONT_FLAGS);
}


static void
set_font_shear(BoundingBoxState* state, float shear)
{
	ServerFont font;
	font.SetShear(shear);
	state->GetDrawState()->SetFont(font, B_FONT_SHEAR);
}


static void
set_font_face(BoundingBoxState* state, int32 face)
{
	ServerFont font;
	font.SetFace(face);
	state->GetDrawState()->SetFont(font, B_FONT_FACE);
}


static void
set_blending_mode(BoundingBoxState*, int16, int16)
{
}


static void
set_transform(BoundingBoxState* state, BAffineTransform transform)
{
	TRACE_BB("%p transform\n", state);
	state->GetDrawState()->SetTransform(transform);
}


const static void* kTableEntries[] = {
	(const void*)nop,									//	0
	(const void*)move_pen_by,
	(const void*)determine_bounds_stroke_line,
	(const void*)determine_bounds_stroke_rect,
	(const void*)determine_bounds_fill_rect,
	(const void*)determine_bounds_stroke_rect,			//	5
	(const void*)determine_bounds_fill_rect,
	(const void*)determine_bounds_stroke_bezier,
	(const void*)determine_bounds_fill_bezier,
	(const void*)determine_bounds_stroke_ellipse,
	(const void*)determine_bounds_fill_ellipse,			//	10
	(const void*)determine_bounds_stroke_ellipse,
	(const void*)determine_bounds_fill_ellipse,
	(const void*)determine_bounds_stroke_polygon,
	(const void*)determine_bounds_fill_polygon,
	(const void*)determine_bounds_stroke_shape,			//	15
	(const void*)determine_bounds_fill_shape,
	(const void*)determine_bounds_string,
	(const void*)determine_bounds_pixels,
	(const void*)draw_picture,
	(const void*)set_clipping_rects,					//	20
	(const void*)clip_to_picture,
	(const void*)push_state,
	(const void*)pop_state,
	(const void*)enter_state_change,
	(const void*)exit_state_change,						//	25
	(const void*)enter_font_state,
	(const void*)exit_font_state,
	(const void*)set_origin,
	(const void*)set_pen_location,
	(const void*)set_drawing_mode,						//	30
	(const void*)set_line_mode,
	(const void*)set_pen_size,
	(const void*)set_fore_color,
	(const void*)set_back_color,
	(const void*)set_stipple_pattern,					//	35
	(const void*)set_scale,
	(const void*)set_font_family,
	(const void*)set_font_style,
	(const void*)set_font_spacing,
	(const void*)set_font_size,							//	40
	(const void*)set_font_rotate,
	(const void*)set_font_encoding,
	(const void*)set_font_flags,
	(const void*)set_font_shear,
	(const void*)nop,									//	45
	(const void*)set_font_face,
	(const void*)set_blending_mode,
	(const void*)set_transform							//	48
};


// #pragma mark - PictureBoundingBoxPlayer


/* static */ void
PictureBoundingBoxPlayer::Play(ServerPicture* picture,
	DrawState* drawState, BRect* outBoundingBox)
{
	State state(drawState, outBoundingBox);

	BMallocIO* mallocIO = dynamic_cast<BMallocIO*>(picture->fData);
	if (mallocIO == NULL)
		return;

	BPrivate::PicturePlayer player(mallocIO->Buffer(),
		mallocIO->BufferLength(), ServerPicture::PictureList::Private(
			picture->fPictures).AsBList());
	player.Play(const_cast<void**>(kTableEntries),
		sizeof(kTableEntries) / sizeof(void*), &state);
}
