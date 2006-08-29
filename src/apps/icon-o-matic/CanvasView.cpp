/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CanvasView.h"

#include <Bitmap.h>
#include <Region.h>

#include <stdio.h>

#include "ui_defines.h"

#include "CommandStack.h"
#include "IconRenderer.h"

// constructor
CanvasView::CanvasView(BRect frame)
	: StateView(frame, "canvas view", B_FOLLOW_ALL,
				B_WILL_DRAW | B_FRAME_EVENTS),
	  fBitmap(new BBitmap(BRect(0, 0, 63, 63), 0, B_RGB32)),
	  fBackground(new BBitmap(BRect(0, 0, 63, 63), 0, B_RGB32)),
	  fIcon(NULL),
	  fRenderer(new IconRenderer(fBitmap)),
	  fDirtyIconArea(fBitmap->Bounds()),

	  fCanvasOrigin(50.0, 50.0),
	  fZoomLevel(8.0),

	  fMouseFilterMode(SNAPPING_OFF),

	  fOffsreenBitmap(NULL),
	  fOffsreenView(NULL)
{
	#if __HAIKU__
	SetFlags(Flags() | B_SUBPIXEL_PRECISE);
	#endif // __HAIKU__

	_MakeBackground();
	fRenderer->SetBackground(fBackground);
}

// destructor
CanvasView::~CanvasView()
{
	SetIcon(NULL);
	delete fRenderer;
	delete fBitmap;
	delete fBackground;

	_FreeBackBitmap();
}

// #pragma mark -

// AttachedToWindow
void
CanvasView::AttachedToWindow()
{
	StateView::AttachedToWindow();

	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(kStripesHigh);
	SetHighColor(kStripesLow);

	_AllocBackBitmap(Bounds().Width(), Bounds().Height());
}

// FrameResized
void
CanvasView::FrameResized(float width, float height)
{
	_AllocBackBitmap(width, height);
}

// Draw
void
CanvasView::Draw(BRect updateRect)
{
	if (!fOffsreenView) {
		_DrawInto(this, updateRect);
	} else {
		BPoint boundsLeftTop = Bounds().LeftTop();
		if (fOffsreenBitmap->Lock()) {
			fOffsreenView->PushState();

			// apply scrolling offset to offscreen view
			fOffsreenView->SetOrigin(-boundsLeftTop.x, -boundsLeftTop.y);
			// mirror the clipping of this view
			// to the offscreen view for performance
			BRegion clipping;
			GetClippingRegion(&clipping);
			fOffsreenView->ConstrainClippingRegion(&clipping);
			fOffsreenView->FillRect(updateRect, B_SOLID_LOW);

			_DrawInto(fOffsreenView, updateRect);

			fOffsreenView->PopState();
			fOffsreenView->Sync();

			fOffsreenBitmap->Unlock();
		}
		// compensate scrolling offset in BView
		BRect bitmapRect = updateRect;
		bitmapRect.OffsetBy(-boundsLeftTop.x, -boundsLeftTop.y);

		SetDrawingMode(B_OP_COPY);
		DrawBitmap(fOffsreenBitmap, bitmapRect, updateRect);
	}
}

// #pragma mark -

// MouseDown
void
CanvasView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus(true);

	_FilterMouse(&where);

	StateView::MouseDown(where);
}


// MouseMoved
void
CanvasView::MouseMoved(BPoint where, uint32 transit,
					   const BMessage* dragMessage)
{
	_FilterMouse(&where);

	StateView::MouseMoved(where, transit, dragMessage);
}

// #pragma mark -

// AreaInvalidated
void
CanvasView::AreaInvalidated(const BRect& area)
{
	if (fDirtyIconArea.Contains(area))
		return;

	fDirtyIconArea = fDirtyIconArea | area;

	BRect viewArea(area);
	ConvertFromCanvas(&viewArea);
	Invalidate(viewArea);
}


// #pragma mark -

// SetIcon
void
CanvasView::SetIcon(Icon* icon)
{
	if (fIcon == icon)
		return;

	if (fIcon)
		fIcon->RemoveListener(this);

	fIcon = icon;
	fRenderer->SetIcon(icon);

	if (fIcon)
		fIcon->AddListener(this);
}

// SetMouseFilterMode
void
CanvasView::SetMouseFilterMode(uint32 mode)
{
	if (fMouseFilterMode == mode)
		return;

	fMouseFilterMode = mode;
	Invalidate(_CanvasRect());
}

// ConvertFromCanvas
void
CanvasView::ConvertFromCanvas(BPoint* point) const
{
	point->x = point->x * fZoomLevel + fCanvasOrigin.x;
	point->y = point->y * fZoomLevel + fCanvasOrigin.y;
}

// ConvertToCanvas
void
CanvasView::ConvertToCanvas(BPoint* point) const
{
	point->x = (point->x - fCanvasOrigin.x) / fZoomLevel;
	point->y = (point->y - fCanvasOrigin.y) / fZoomLevel;
}

// ConvertFromCanvas
void
CanvasView::ConvertFromCanvas(BRect* r) const
{
	r->left = r->left * fZoomLevel + fCanvasOrigin.x;
	r->top = r->top * fZoomLevel + fCanvasOrigin.y;
	r->right++;
	r->bottom++;
	r->right = r->right * fZoomLevel + fCanvasOrigin.x;
	r->bottom = r->bottom * fZoomLevel + fCanvasOrigin.y;
	r->right--;
	r->bottom--;
}

// ConvertToCanvas
void
CanvasView::ConvertToCanvas(BRect* r) const
{
	r->left = (r->left - fCanvasOrigin.x) / fZoomLevel;
	r->top = (r->top - fCanvasOrigin.y) / fZoomLevel;
	r->right = (r->right - fCanvasOrigin.x) / fZoomLevel;
	r->bottom = (r->bottom - fCanvasOrigin.y) / fZoomLevel;
}

// #pragma mark -

// _HandleKeyDown
bool
CanvasView::_HandleKeyDown(uint32 key, uint32 modifiers)
{
	switch (key) {
		case 'z':
		case 'y':
			if (modifiers & B_SHIFT_KEY)
				CommandStack()->Redo();
			else
				CommandStack()->Undo();
			break;

		default:
			return StateView::_HandleKeyDown(key, modifiers);
	}

	return true;
}

// _CanvasRect()
BRect
CanvasView::_CanvasRect() const
{
	BRect r = fBitmap->Bounds();
	ConvertFromCanvas(&r);
	return r;
}

// _AllocBackBitmap
void
CanvasView::_AllocBackBitmap(float width, float height)
{
	// sanity check
	if (width <= 0.0 || height <= 0.0)
		return;

	if (fOffsreenBitmap) {
		// see if the bitmap needs to be expanded
		BRect b = fOffsreenBitmap->Bounds();
		if (b.Width() >= width && b.Height() >= height)
			return;

		// it does; clean up:
		_FreeBackBitmap();
	}

	BRect b(0.0, 0.0, width, height);
	fOffsreenBitmap = new (nothrow) BBitmap(b, B_RGB32, true);
	if (!fOffsreenBitmap) {
		fprintf(stderr, "CanvasView::_AllocBackBitmap(): failed to allocate\n");
		return;
	}
	if (fOffsreenBitmap->IsValid()) {
		fOffsreenView = new BView(b, 0, B_FOLLOW_NONE, B_WILL_DRAW);
		BFont font;
		GetFont(&font);
		fOffsreenView->SetFont(&font);
		fOffsreenView->SetHighColor(HighColor());
		fOffsreenView->SetLowColor(LowColor());
		fOffsreenView->SetFlags(Flags());
		fOffsreenBitmap->AddChild(fOffsreenView);
	} else {
		_FreeBackBitmap();
		fprintf(stderr, "CanvasView::_AllocBackBitmap(): bitmap invalid\n");
	}
}

// _FreeBackBitmap
void
CanvasView::_FreeBackBitmap()
{
	if (fOffsreenBitmap) {
		delete fOffsreenBitmap;
		fOffsreenBitmap = NULL;
		fOffsreenView = NULL;
	}
}

// _DrawInto
void
CanvasView::_DrawInto(BView* view, BRect updateRect)
{
	if (fDirtyIconArea.IsValid()) {
		fRenderer->Render(fDirtyIconArea);
		fDirtyIconArea.Set(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	}

	// icon
	BRect canvas(_CanvasRect());
	view->DrawBitmap(fBitmap, fBitmap->Bounds(), canvas);

	// grid
	int32 gridLines = 0;
	int32 scale = 1;
	switch (fMouseFilterMode) {
		case SNAPPING_64:
			gridLines = 63;
			break;
		case SNAPPING_32:
			gridLines = 31;
			scale = 2;
			break;
		case SNAPPING_16:
			gridLines = 15;
			scale = 4;
			break;
		case SNAPPING_OFF:
		default:
			break;
	}
	view->SetDrawingMode(B_OP_BLEND);
	for (int32 i = 1; i <= gridLines; i++) {
		BPoint cross(i * scale, i * scale);
		ConvertFromCanvas(&cross);
		view->StrokeLine(BPoint(canvas.left, cross.y),
						 BPoint(canvas.right, cross.y));
		view->StrokeLine(BPoint(cross.x, canvas.top),
						 BPoint(cross.x, canvas.bottom));
	}
	view->SetDrawingMode(B_OP_COPY);

	// outside icon
	BRegion outside(Bounds() & updateRect);
	outside.Exclude(canvas);
	view->FillRegion(&outside, kStripes);

	StateView::Draw(view, updateRect);
}

// _FilterMouse
void
CanvasView::_FilterMouse(BPoint* where) const
{
	switch (fMouseFilterMode) {

		case SNAPPING_64:
			ConvertToCanvas(where);
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			ConvertFromCanvas(where);
			break;

		case SNAPPING_32:
			ConvertToCanvas(where);
			where->x /= 2.0;
			where->y /= 2.0;
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			where->x *= 2.0;
			where->y *= 2.0;
			ConvertFromCanvas(where);
			break;

		case SNAPPING_16:
			ConvertToCanvas(where);
			where->x /= 4.0;
			where->y /= 4.0;
			where->x = floorf(where->x + 0.5);
			where->y = floorf(where->y + 0.5);
			where->x *= 4.0;
			where->y *= 4.0;
			ConvertFromCanvas(where);
			break;

		case SNAPPING_OFF:
		default:
			break;
	}
}

// _MakeBackground
void
CanvasView::_MakeBackground()
{
	uint8* row = (uint8*)fBackground->Bits();
	uint32 bpr = fBackground->BytesPerRow();
	uint32 width = fBackground->Bounds().IntegerWidth() + 1;
	uint32 height = fBackground->Bounds().IntegerHeight() + 1;

	const GammaTable& lut = fRenderer->GammaTable();
	uint8 redLow = lut.dir(kAlphaLow.red);
	uint8 greenLow = lut.dir(kAlphaLow.blue);
	uint8 blueLow = lut.dir(kAlphaLow.green);
	uint8 redHigh = lut.dir(kAlphaHigh.red);
	uint8 greenHigh = lut.dir(kAlphaHigh.blue);
	uint8 blueHigh = lut.dir(kAlphaHigh.green);

	for (uint32 y = 0; y < height; y++) {
		uint8* p = row;
		for (uint32 x = 0; x < width; x++) {
			p[3] = 255;
			if (x % 8 >= 4) {
				if (y % 8 >= 4) {
					p[0] = blueLow;
					p[1] = greenLow;
					p[2] = redLow;
				} else {
					p[0] = blueHigh;
					p[1] = greenHigh;
					p[2] = redHigh;
				}
			} else {
				if (y % 8 >= 4) {
					p[0] = blueHigh;
					p[1] = greenHigh;
					p[2] = redHigh;
				} else {
					p[0] = blueLow;
					p[1] = greenLow;
					p[2] = redLow;
				}
			}
			p += 4;
		}
		row += bpr;
	}
}

