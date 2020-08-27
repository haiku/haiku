/*
 * Copyright (c) 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#include "View.h"

#include <new>
#include <stdio.h>

#include "AlphaMask.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "Layer.h"
#include "Overlay.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#include "ServerPicture.h"
#include "ServerWindow.h"
#include "Window.h"

#include "BitmapHWInterface.h"
#include "drawing_support.h"

#include <List.h>
#include <Message.h>
#include <PortLink.h>
#include <View.h> // for resize modes
#include <WindowPrivate.h>

#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>


using std::nothrow;


void
resize_frame(IntRect& frame, uint32 resizingMode, int32 x, int32 y)
{
	// follow with left side
	if ((resizingMode & 0x0F00U) == _VIEW_RIGHT_ << 8)
		frame.left += x;
	else if ((resizingMode & 0x0F00U) == _VIEW_CENTER_ << 8)
		frame.left += x / 2;

	// follow with right side
	if ((resizingMode & 0x000FU) == _VIEW_RIGHT_)
		frame.right += x;
	else if ((resizingMode & 0x000FU) == _VIEW_CENTER_)
		frame.right += x / 2;

	// follow with top side
	if ((resizingMode & 0xF000U) == _VIEW_BOTTOM_ << 12)
		frame.top += y;
	else if ((resizingMode & 0xF000U) == _VIEW_CENTER_ << 12)
		frame.top += y / 2;

	// follow with bottom side
	if ((resizingMode & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		frame.bottom += y;
	else if ((resizingMode & 0x00F0U) == _VIEW_CENTER_ << 4)
		frame.bottom += y / 2;
}


//	#pragma mark -


View::View(IntRect frame, IntPoint scrollingOffset, const char* name,
		int32 token, uint32 resizeMode, uint32 flags)
	:
	fName(name),
	fToken(token),

	fFrame(frame),
	fScrollingOffset(scrollingOffset),

	fViewColor((rgb_color){ 255, 255, 255, 255 }),
	fWhichViewColor(B_NO_COLOR),
	fWhichViewColorTint(B_NO_TINT),
	fViewBitmap(NULL),
	fBitmapResizingMode(0),
	fBitmapOptions(0),

	fResizeMode(resizeMode),
	fFlags(flags),

	// Views start visible by default
	fHidden(false),
	fVisible(true),
	fBackgroundDirty(true),
	fIsDesktopBackground(false),

	fEventMask(0),
	fEventOptions(0),

	fWindow(NULL),
	fParent(NULL),

	fFirstChild(NULL),
	fPreviousSibling(NULL),
	fNextSibling(NULL),
	fLastChild(NULL),

	fCursor(NULL),
	fPicture(NULL),

	fLocalClipping((BRect)Bounds()),
	fScreenClipping(),
	fScreenClippingValid(false),
	fUserClipping(NULL),
	fScreenAndUserClipping(NULL)
{
	if (fDrawState.Get() != NULL)
		fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


View::~View()
{
	// iterate over children and delete each one
	View* view = fFirstChild;
	while (view) {
		View* toast = view;
		view = view->fNextSibling;
		delete toast;
	}
}


IntRect
View::Bounds() const
{
	IntRect bounds(fScrollingOffset.x, fScrollingOffset.y,
		fScrollingOffset.x + fFrame.Width(),
		fScrollingOffset.y + fFrame.Height());
	return bounds;
}


void
View::ConvertToVisibleInTopView(IntRect* bounds) const
{
	*bounds = *bounds & Bounds();
	// NOTE: this step is necessary even if we don't have a parent!
	bounds->OffsetBy(fFrame.left - fScrollingOffset.x,
		fFrame.top - fScrollingOffset.y);

	if (fParent)
		fParent->ConvertToVisibleInTopView(bounds);
}


void
View::AttachedToWindow(::Window* window)
{
	fWindow = window;

	// an ugly hack to detect the desktop background
	if (window->Feel() == kDesktopWindowFeel && Parent() == TopView())
		fIsDesktopBackground = true;

	// insert view into local token space
	if (fWindow != NULL) {
		fWindow->ServerWindow()->App()->ViewTokens().SetToken(fToken,
			B_HANDLER_TOKEN, this);
	}

	// attach child views as well
	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->AttachedToWindow(window);
}


void
View::DetachedFromWindow()
{
	// remove view from local token space
	if (fWindow != NULL && fWindow->ServerWindow()->App() != NULL)
		fWindow->ServerWindow()->App()->ViewTokens().RemoveToken(fToken);

	fWindow = NULL;
	// detach child views as well
	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->DetachedFromWindow();
}


// #pragma mark -


DrawingEngine*
View::GetDrawingEngine() const
{
	return Window()->GetDrawingEngine();
}


ServerPicture*
View::GetPicture(int32 token) const
{
	return Window()->ServerWindow()->App()->GetPicture(token);
}


void
View::ResyncDrawState()
{
	return Window()->ServerWindow()->ResyncDrawState();
}


void
View::UpdateCurrentDrawingRegion()
{
	return Window()->ServerWindow()->UpdateCurrentDrawingRegion();
}


void
View::AddChild(View* view)
{
	if (view->fParent) {
		printf("View::AddChild() - View already has a parent\n");
		return;
	}

	view->fParent = this;

	if (!fLastChild) {
		// no children yet
		fFirstChild = view;
	} else {
		// append view to formerly last child
		fLastChild->fNextSibling = view;
		view->fPreviousSibling = fLastChild;
	}
	fLastChild = view;

	view->UpdateVisibleDeep(fVisible);

	if (view->IsVisible())
		RebuildClipping(false);

	if (fWindow) {
		view->AttachedToWindow(fWindow);

		if (view->IsVisible()) {
			// trigger redraw
			IntRect clippedFrame = view->Frame();
			ConvertToVisibleInTopView(&clippedFrame);

			BRegion dirty;
			dirty.Set((clipping_rect)clippedFrame);
			fWindow->MarkContentDirtyAsync(dirty);
		}
	}
}


bool
View::RemoveChild(View* view)
{
	if (view == NULL || view->fParent != this) {
		printf("View::RemoveChild(%p - %s) - View is not child of "
			"this (%p) view!\n", view, view ? view->Name() : NULL, this);
		return false;
	}

	view->fParent = NULL;

	if (fLastChild == view)
		fLastChild = view->fPreviousSibling;
		// view->fNextSibling would be NULL

	if (fFirstChild == view )
		fFirstChild = view->fNextSibling;
		// view->fPreviousSibling would be NULL

	// connect child before and after view
	if (view->fPreviousSibling)
		view->fPreviousSibling->fNextSibling = view->fNextSibling;

	if (view->fNextSibling)
		view->fNextSibling->fPreviousSibling = view->fPreviousSibling;

	// view has no siblings anymore
	view->fPreviousSibling = NULL;
	view->fNextSibling = NULL;

	if (view->IsVisible()) {
		Overlay* overlay = view->_Overlay();
		if (overlay != NULL)
			overlay->Hide();

		RebuildClipping(false);
	}

	if (fWindow) {
		view->DetachedFromWindow();

		if (fVisible && view->IsVisible()) {
			// trigger redraw
			IntRect clippedFrame = view->Frame();
			ConvertToVisibleInTopView(&clippedFrame);

			BRegion dirty;
			dirty.Set((clipping_rect)clippedFrame);
			fWindow->MarkContentDirtyAsync(dirty);
		}
	}

	return true;
}


View*
View::TopView()
{
	// returns the top level view of the hirarchy,
	// it doesn't have to be the top level of a window

	if (fParent)
		return fParent->TopView();

	return this;
}


uint32
View::CountChildren(bool deep) const
{
	uint32 count = 0;
	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		count++;
		if (deep) {
			count += child->CountChildren(deep);
		}
	}
	return count;
}


void
View::CollectTokensForChildren(BList* tokenMap) const
{
	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		tokenMap->AddItem((void*)child);
		child->CollectTokensForChildren(tokenMap);
	}
}


#if 0
bool
View::MarkAt(DrawingEngine* engine, const BPoint& where, int32 level)
{
	BRect rect(fFrame.left, fFrame.top, fFrame.right, fFrame.bottom);

	if (Parent() != NULL) {
		Parent()->ConvertToScreen(&rect);
		if (!rect.Contains(where))
			return false;

		engine->StrokeRect(rect, (rgb_color){level * 30, level * 30, level * 30});
	}


	bool found = false;
	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		found |= child->MarkAt(engine, where, level + 1);
	}

	if (!found) {
		rgb_color color = {0};
		switch (level % 2) {
			case 0: color.green = rand() % 256; break;
			case 1: color.blue = rand() % 256; break;
		}

		rect.InsetBy(1, 1);
		//engine->FillRegion(fLocalClipping, (rgb_color){255, 255, 0, 10});
		engine->StrokeRect(rect, color);
		rect.InsetBy(1, 1);
		engine->StrokeRect(rect, color);
	}

	return true;
}
#endif


void
View::FindViews(uint32 flags, BObjectList<View>& list, int32& left)
{
	if ((Flags() & flags) == flags) {
		list.AddItem(this);
		left--;
		return;
	}

	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		child->FindViews(flags, list, left);
		if (left == 0)
			break;
	}
}


bool
View::HasView(View* view)
{
	if (view == this)
		return true;

	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		if (child->HasView(view))
			return true;
	}

	return false;
}


View*
View::ViewAt(const BPoint& where)
{
	if (!fVisible)
		return NULL;

	IntRect frame = Frame();
	if (Parent() != NULL)
		Parent()->LocalToScreenTransform().Apply(&frame);

	if (!frame.Contains(where))
		return NULL;

	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		View* view = child->ViewAt(where);
		if (view != NULL)
			return view;
	}

	return this;
}


// #pragma mark -


void
View::SetName(const char* string)
{
	fName.SetTo(string);
}


void
View::SetFlags(uint32 flags)
{
	uint32 oldFlags = fFlags;
	fFlags = flags;

	// Child view with B_TRANSPARENT_BACKGROUND flag change clipping of
	// parent view.
	if (fParent != NULL
		&& IsVisible()
		&& (((oldFlags & B_TRANSPARENT_BACKGROUND) != 0)
			!= ((fFlags & B_TRANSPARENT_BACKGROUND) != 0))) {
		fParent->RebuildClipping(false);
	}

	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
View::SetViewBitmap(ServerBitmap* bitmap, IntRect sourceRect,
	IntRect destRect, int32 resizingMode, int32 options)
{
	if (fViewBitmap != NULL) {
		Overlay* overlay = _Overlay();

		if (bitmap != NULL) {
			// take over overlay token from current overlay (if it has any)
			Overlay* newOverlay = bitmap->Overlay();

			if (overlay != NULL && newOverlay != NULL)
				newOverlay->TakeOverToken(overlay);
		} else if (overlay != NULL)
			overlay->Hide();
	}

	fViewBitmap.SetTo(bitmap, false);
	fBitmapSource = sourceRect;
	fBitmapDestination = destRect;
	fBitmapResizingMode = resizingMode;
	fBitmapOptions = options;

	_UpdateOverlayView();
}


::Overlay*
View::_Overlay() const
{
	if (fViewBitmap == NULL)
		return NULL;

	return fViewBitmap->Overlay();
}


void
View::_UpdateOverlayView() const
{
	Overlay* overlay = _Overlay();
	if (overlay == NULL)
		return;

	IntRect destination = fBitmapDestination;
	LocalToScreenTransform().Apply(&destination);

	overlay->Configure(fBitmapSource, destination);
}


/*!
	This method is called whenever the window is resized or moved - would
	be nice to have a better solution for this, though.
*/
void
View::UpdateOverlay()
{
	if (!IsVisible())
		return;

	if (_Overlay() != NULL) {
		_UpdateOverlayView();
	} else {
		// recursively ask children of this view

		for (View* child = FirstChild(); child; child = child->NextSibling()) {
			child->UpdateOverlay();
		}
	}
}


// #pragma mark -


void
View::_LocalToScreenTransform(SimpleTransform& transform) const
{
	const View* view = this;
	int32 offsetX = 0;
	int32 offsetY = 0;
	do {
		offsetX += view->fFrame.left - view->fScrollingOffset.x;
		offsetY += view->fFrame.top  - view->fScrollingOffset.y;
		view = view->fParent;
	} while (view != NULL);

	transform.AddOffset(offsetX, offsetY);
}


void
View::_ScreenToLocalTransform(SimpleTransform& transform) const
{
	const View* view = this;
	int32 offsetX = 0;
	int32 offsetY = 0;
	do {
		offsetX += view->fScrollingOffset.x - view->fFrame.left;
		offsetY += view->fScrollingOffset.y - view->fFrame.top;
		view = view->fParent;
	} while (view != NULL);

	transform.AddOffset(offsetX, offsetY);
}


// #pragma mark -


void
View::MoveBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);

	// to move on screen, we must not be hidden and we must have a parent
	if (fVisible && fParent && dirtyRegion) {
#if 1
// based on redraw on new location
		// the place were we are now visible
		IntRect newVisibleBounds(Bounds());
		// we can use the frame of the old
		// local clipping to see which parts need invalidation
		IntRect oldVisibleBounds(newVisibleBounds);
		oldVisibleBounds.OffsetBy(-x, -y);
		LocalToScreenTransform().Apply(&oldVisibleBounds);

		ConvertToVisibleInTopView(&newVisibleBounds);

		dirtyRegion->Include((clipping_rect)oldVisibleBounds);
		// newVisibleBounds already is in screen coords
		dirtyRegion->Include((clipping_rect)newVisibleBounds);
#else
// blitting version, invalidates
// old contents
		IntRect oldVisibleBounds(Bounds());
		IntRect newVisibleBounds(oldVisibleBounds);
		oldVisibleBounds.OffsetBy(-x, -y);
		LocalToScreenTransform().Apply(&oldVisibleBounds);

		// NOTE: using ConvertToVisibleInTopView()
		// instead of ConvertToScreen()! see below
		ConvertToVisibleInTopView(&newVisibleBounds);

		newVisibleBounds.OffsetBy(-x, -y);

		// clipping oldVisibleBounds to newVisibleBounds
		// makes sure we don't copy parts hidden under
		// parent views
		BRegion* region = fWindow->GetRegion();
		if (region) {
			region->Set(oldVisibleBounds & newVisibleBounds);
			fWindow->CopyContents(region, x, y);

			region->Set(oldVisibleBounds);
			newVisibleBounds.OffsetBy(x, y);
			region->Exclude((clipping_rect)newVisibleBounds);
			dirtyRegion->Include(dirty);

			fWindow->RecycleRegion(region);
		}

#endif
	}

	if (!fParent) {
		// the top view's screen clipping does not change,
		// because no parts are clipped away from parent
		// views
		_MoveScreenClipping(x, y, true);
	} else {
		// parts might have been revealed from underneath
		// the parent, or might now be hidden underneath
		// the parent, this is taken care of when building
		// the screen clipping
		InvalidateScreenClipping();
	}
}


void
View::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	if (fVisible && dirtyRegion) {
		IntRect oldBounds(Bounds());
		oldBounds.right -= x;
		oldBounds.bottom -= y;

		BRegion* dirty = fWindow->GetRegion();
		if (!dirty)
			return;

		dirty->Set((clipping_rect)Bounds());
		dirty->Include((clipping_rect)oldBounds);

		if (!(fFlags & B_FULL_UPDATE_ON_RESIZE)) {
			// the dirty region is just the difference of
			// old and new bounds
			dirty->Exclude((clipping_rect)(oldBounds & Bounds()));
		}

		InvalidateScreenClipping();

		if (dirty->CountRects() > 0) {
			if ((fFlags & B_DRAW_ON_CHILDREN) == 0) {
				// exclude children, they are expected to
				// include their own dirty regions in ParentResized()
				for (View* child = FirstChild(); child;
						child = child->NextSibling()) {
					if (!child->IsVisible()
						|| (child->fFlags & B_TRANSPARENT_BACKGROUND) != 0) {
						continue;
					}
					IntRect previousChildVisible(
						child->Frame() & oldBounds & Bounds());
					if (dirty->Frame().Intersects(previousChildVisible)) {
						dirty->Exclude((clipping_rect)previousChildVisible);
					}
				}
			}

			LocalToScreenTransform().Apply(dirty);
			dirtyRegion->Include(dirty);
		}
		fWindow->RecycleRegion(dirty);
	}

	// layout the children
	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->ParentResized(x, y, dirtyRegion);

	// view bitmap
	if (fViewBitmap != NULL)
		resize_frame(fBitmapDestination, fBitmapResizingMode, x, y);

	// at this point, children are at their new locations,
	// so we can rebuild the clipping
	// TODO: when the implementation of Hide() and Show() is
	// complete, see if this should be avoided
	RebuildClipping(false);
}


void
View::ParentResized(int32 x, int32 y, BRegion* dirtyRegion)
{
	IntRect newFrame = fFrame;
	resize_frame(newFrame, fResizeMode & 0x0000ffff, x, y);

	if (newFrame != fFrame) {
		// careful, MoveBy will change fFrame
		int32 widthDiff = (int32)(newFrame.Width() - fFrame.Width());
		int32 heightDiff = (int32)(newFrame.Height() - fFrame.Height());

		MoveBy(newFrame.left - fFrame.left,
			newFrame.top - fFrame.top, dirtyRegion);

		ResizeBy(widthDiff, heightDiff, dirtyRegion);
	} else {
		// TODO: this covers the fact that our screen clipping might change
		// when the parent changes its size, even though our frame stays
		// the same - there might be a way to test for this, but axeld doesn't
		// know, stippi should look into this when he's back :)
		InvalidateScreenClipping();
	}
}


void
View::ScrollBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (!fVisible || !fWindow) {
		fScrollingOffset.x += x;
		fScrollingOffset.y += y;
		return;
	}

	// blitting version, invalidates
	// old contents

	// remember old bounds for tracking dirty region
	IntRect oldBounds(Bounds());

	// NOTE: using ConvertToVisibleInTopView()
	// instead of ConvertToScreen(), this makes
	// sure we don't try to move or invalidate an
	// area hidden underneath the parent view
	ConvertToVisibleInTopView(&oldBounds);

	// find the area of the view that can be scrolled,
	// contents are shifted in the opposite direction from scrolling
	IntRect stillVisibleBounds(oldBounds);
	stillVisibleBounds.OffsetBy(x, y);
	stillVisibleBounds = stillVisibleBounds & oldBounds;

	fScrollingOffset.x += x;
	fScrollingOffset.y += y;

	// do the blit, this will make sure
	// that other more complex dirty regions
	// are taken care of
	BRegion* copyRegion = fWindow->GetRegion();
	if (!copyRegion)
		return;
	copyRegion->Set((clipping_rect)stillVisibleBounds);
	fWindow->CopyContents(copyRegion, -x, -y);

	// find the dirty region as far as we are
	// concerned
	BRegion* dirty = copyRegion;
		// reuse copyRegion and call it dirty

	dirty->Set((clipping_rect)oldBounds);
	stillVisibleBounds.OffsetBy(-x, -y);
	dirty->Exclude((clipping_rect)stillVisibleBounds);
	dirtyRegion->Include(dirty);

	fWindow->RecycleRegion(dirty);

	// the screen clipping of this view and it's
	// childs is no longer valid
	InvalidateScreenClipping();
	RebuildClipping(false);
}


void
View::CopyBits(IntRect src, IntRect dst, BRegion& windowContentClipping)
{
	if (!fVisible || !fWindow)
		return;

	// TODO: figure out what to do when we have a transform which is not
	// a dilation
	BAffineTransform transform = CurrentState()->CombinedTransform();
	if (!transform.IsIdentity() && transform.IsDilation()) {
		BPoint points[4] = { src.LeftTop(), src.RightBottom(),
							 dst.LeftTop(), dst.RightBottom() };
		transform.Apply(&points[0], 4);
		src.Set(points[0].x, points[0].y, points[1].x, points[1].y);
		dst.Set(points[2].x, points[2].y, points[3].x, points[3].y);
	}

	// TODO: confirm that in R5 this call is affected by origin and scale

	// blitting version

	int32 xOffset = dst.left - src.left;
	int32 yOffset = dst.top - src.top;

	// figure out which part can be blittet
	IntRect visibleSrc(src);
	ConvertToVisibleInTopView(&visibleSrc);

	IntRect visibleSrcAtDest(src);
	visibleSrcAtDest.OffsetBy(xOffset, yOffset);
	ConvertToVisibleInTopView(&visibleSrcAtDest);

	// clip src to visible at dest
	visibleSrcAtDest.OffsetBy(-xOffset, -yOffset);
	visibleSrc = visibleSrc & visibleSrcAtDest;

	// do the blit, this will make sure
	// that other more complex dirty regions
	// are taken care of
	BRegion* copyRegion = fWindow->GetRegion();
	if (!copyRegion)
		return;

	// move src rect to destination here for efficiency reasons
	visibleSrc.OffsetBy(xOffset, yOffset);

	// we need to interstect the copyRegion two times, onces
	// at the source and once at the destination (here done
	// the other way arround but it doesn't matter)
	// the reason for this is that we are not supposed to visually
	// copy children in the source rect and neither to copy onto
	// children in the destination rect...
	copyRegion->Set((clipping_rect)visibleSrc);
	BRegion *screenAndUserClipping
		= &ScreenAndUserClipping(&windowContentClipping);
	copyRegion->IntersectWith(screenAndUserClipping);
	copyRegion->OffsetBy(-xOffset, -yOffset);
	copyRegion->IntersectWith(screenAndUserClipping);

	// do the actual blit
	fWindow->CopyContents(copyRegion, xOffset, yOffset);

	// find the dirty region as far as we are concerned
	IntRect dirtyDst(dst);
	ConvertToVisibleInTopView(&dirtyDst);

	BRegion* dirty = fWindow->GetRegion();
	if (!dirty) {
		fWindow->RecycleRegion(copyRegion);
		return;
	}

	// offset copyRegion to destination again
	copyRegion->OffsetBy(xOffset, yOffset);
	// start with destination given by user
	dirty->Set((clipping_rect)dirtyDst);
	// exclude the part that we could copy
	dirty->Exclude(copyRegion);

	dirty->IntersectWith(screenAndUserClipping);
	fWindow->MarkContentDirty(*dirty);

	fWindow->RecycleRegion(dirty);
	fWindow->RecycleRegion(copyRegion);
}


// #pragma mark -


void
View::ColorUpdated(color_which which, rgb_color color)
{
	float tint = B_NO_TINT;

	if (fWhichViewColor == which)
		SetViewColor(tint_color(color, fWhichViewColorTint));

	if (CurrentState()->HighUIColor(&tint) == which)
		CurrentState()->SetHighColor(tint_color(color, tint));

	if (CurrentState()->LowUIColor(&tint) == which)
		CurrentState()->SetLowColor(tint_color(color, tint));

	for (View* child = FirstChild(); child != NULL;
			child = child->NextSibling()) {

		child->ColorUpdated(which, color);
	}
}


void
View::SetViewUIColor(color_which which, float tint)
{
	if (which != B_NO_COLOR) {
		DesktopSettings settings(fWindow->Desktop());
		SetViewColor(tint_color(settings.UIColor(which), tint));
	}

	fWhichViewColor = which;
	fWhichViewColorTint = tint;
}


color_which
View::ViewUIColor(float* tint)
{
	if (tint != NULL)
		*tint = fWhichViewColorTint;

	return fWhichViewColor;
}


// #pragma mark -


void
View::PushState()
{
	DrawState* previousState = fDrawState.Detach();
	DrawState* newState = previousState->PushState();
	if (newState == NULL)
		newState = previousState;

	fDrawState.SetTo(newState);
	// In BeAPI, B_SUBPIXEL_PRECISE is a view flag, and not affected by the
	// view state. Our implementation moves it to the draw state, but let's
	// be compatible with the API here and make it survive accross state
	// changes.
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
View::PopState()
{
	if (fDrawState->PreviousState() == NULL) {
		fprintf(stderr, "WARNING: User called BView(%s)::PopState(), "
			"but there is NO state on stack!\n", Name());
		return;
	}

	bool rebuildClipping = fDrawState->HasAdditionalClipping();

	fDrawState.SetTo(fDrawState->PopState());
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);

	// rebuild clipping
	// (the clipping from the popped state is not effective anymore)
	if (rebuildClipping)
		RebuildClipping(false);
}


// #pragma mark -


void
View::SetEventMask(uint32 eventMask, uint32 options)
{
	fEventMask = eventMask;
	fEventOptions = options;
}


void
View::SetCursor(ServerCursor* cursor)
{
	if (cursor == fCursor)
		return;

	fCursor.SetTo(cursor, false);
}


void
View::SetPicture(ServerPicture* picture)
{
	if (picture == fPicture)
		return;

	fPicture.SetTo(picture, false);
}


void
View::BlendAllLayers()
{
	if (fPicture == NULL)
		return;
	Layer* layer = dynamic_cast<Layer*>(fPicture.Get());
	if (layer == NULL)
		return;
	BlendLayer(layer);
}


void
View::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping,
	BRegion* windowContentClipping, bool deep)
{
	if (!fVisible) {
		// child views cannot be visible either
		return;
	}

	if (fViewBitmap != NULL || fViewColor != B_TRANSPARENT_COLOR) {
		// we can only draw within our own area
		BRegion* redraw;
		if ((fFlags & B_DRAW_ON_CHILDREN) != 0) {
			// The client may actually want to prevent the background to
			// be painted outside the user clipping.
			redraw = fWindow->GetRegion(
				ScreenAndUserClipping(windowContentClipping));
		} else {
			// Ignore user clipping as in BeOS for painting the background.
			redraw = fWindow->GetRegion(
				_ScreenClipping(windowContentClipping));
		}
		if (!redraw)
			return;
		// add the current clipping
		redraw->IntersectWith(effectiveClipping);

		Overlay* overlayCookie = _Overlay();

		if (fViewBitmap != NULL && overlayCookie == NULL) {
			// draw view bitmap
			// TODO: support other options!
			BRect rect = fBitmapDestination;
			PenToScreenTransform().Apply(&rect);

			align_rect_to_pixels(&rect);

			if (fBitmapOptions & B_TILE_BITMAP_Y) {
				// move rect up as much as needed
				while (rect.top > redraw->Frame().top)
					rect.OffsetBy(0.0, -(rect.Height() + 1));
			}
			if (fBitmapOptions & B_TILE_BITMAP_X) {
				// move rect left as much as needed
				while (rect.left > redraw->Frame().left)
					rect.OffsetBy(-(rect.Width() + 1), 0.0);
			}

// XXX: locking removed because the Window keeps the engine locked
// because it keeps track of syncing right now

			// lock the drawing engine for as long as we need the clipping
			// to be valid
			if (rect.IsValid()/* && drawingEngine->Lock()*/) {
				drawingEngine->ConstrainClippingRegion(redraw);

				drawing_mode oldMode;
				drawingEngine->SetDrawingMode(B_OP_COPY, oldMode);

				if (fBitmapOptions & B_TILE_BITMAP) {
					// tile across entire view

					float start = rect.left;
					while (rect.top < redraw->Frame().bottom) {
						while (rect.left < redraw->Frame().right) {
							drawingEngine->DrawBitmap(fViewBitmap,
								fBitmapSource, rect, fBitmapOptions);
							rect.OffsetBy(rect.Width() + 1, 0.0);
						}
						rect.OffsetBy(start - rect.left, rect.Height() + 1);
					}
					// nothing left to be drawn
					redraw->MakeEmpty();
				} else if (fBitmapOptions & B_TILE_BITMAP_X) {
					// tile in x direction

					while (rect.left < redraw->Frame().right) {
						drawingEngine->DrawBitmap(fViewBitmap, fBitmapSource,
							rect, fBitmapOptions);
						rect.OffsetBy(rect.Width() + 1, 0.0);
					}
					// remove horizontal stripe from clipping
					rect.left = redraw->Frame().left;
					rect.right = redraw->Frame().right;
					redraw->Exclude(rect);
				} else if (fBitmapOptions & B_TILE_BITMAP_Y) {
					// tile in y direction

					while (rect.top < redraw->Frame().bottom) {
						drawingEngine->DrawBitmap(fViewBitmap, fBitmapSource,
							rect, fBitmapOptions);
						rect.OffsetBy(0.0, rect.Height() + 1);
					}
					// remove vertical stripe from clipping
					rect.top = redraw->Frame().top;
					rect.bottom = redraw->Frame().bottom;
					redraw->Exclude(rect);
				} else {
					// no tiling at all

					drawingEngine->DrawBitmap(fViewBitmap, fBitmapSource,
						rect, fBitmapOptions);
					redraw->Exclude(rect);
				}

				drawingEngine->SetDrawingMode(oldMode);

				// NOTE: It is ok not to reset the clipping, that
				// would only waste time
//				drawingEngine->Unlock();
			}

		}

		if (fViewColor != B_TRANSPARENT_COLOR) {
			// fill visible region with view color,
			// this version of FillRegion ignores any
			// clipping, that's why "redraw" needs to
			// be correct
// see #634
//			if (redraw->Frame().left < 0 || redraw->Frame().top < 0) {
//				char message[1024];
//				BRect c = effectiveClipping->Frame();
//				BRect w = windowContentClipping->Frame();
//				BRect r = redraw->Frame();
//				sprintf(message, "invalid background: current clipping: (%d, %d)->(%d, %d), "
//					"window content: (%d, %d)->(%d, %d), redraw: (%d, %d)->(%d, %d)",
//					(int)c.left, (int)c.top, (int)c.right, (int)c.bottom,
//					(int)w.left, (int)w.top, (int)w.right, (int)w.bottom,
//					(int)r.left, (int)r.top, (int)r.right, (int)r.bottom);
//				debugger(message);
//			}

			drawingEngine->FillRegion(*redraw, overlayCookie != NULL
				? overlayCookie->Color() : fViewColor);
		}

		fWindow->RecycleRegion(redraw);
	}

	fBackgroundDirty = false;

	// let children draw
	if (deep) {
		for (View* child = FirstChild(); child; child = child->NextSibling()) {
			child->Draw(drawingEngine, effectiveClipping,
				windowContentClipping, deep);
		}
	}
}


// #pragma mark -


void
View::MouseDown(BMessage* message, BPoint where)
{
	// empty hook method
}


void
View::MouseUp(BMessage* message, BPoint where)
{
	// empty hook method
}


void
View::MouseMoved(BMessage* message, BPoint where)
{
	// empty hook method
}


// #pragma mark -


void
View::SetHidden(bool hidden)
{
	if (fHidden != hidden) {
		fHidden = hidden;

		// recurse into children and update their visible flag
		bool oldVisible = fVisible;
		UpdateVisibleDeep(fParent ? fParent->IsVisible() : !fHidden);
		if (oldVisible != fVisible) {
			// Include or exclude us from the parent area, and update the
			// children's clipping as well when the view will be visible
			if (fParent)
				fParent->RebuildClipping(fVisible);
			else
				RebuildClipping(fVisible);

			if (fWindow) {
				// trigger a redraw
				IntRect clippedBounds = Bounds();
				ConvertToVisibleInTopView(&clippedBounds);

				BRegion dirty;
				dirty.Set((clipping_rect)clippedBounds);
				fWindow->MarkContentDirty(dirty);
			}
		}
	}
}


bool
View::IsHidden() const
{
	return fHidden;
}


void
View::UpdateVisibleDeep(bool parentVisible)
{
	bool wasVisible = fVisible;

	fVisible = parentVisible && !fHidden;
	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->UpdateVisibleDeep(fVisible);

	// overlay handling

	Overlay* overlay = _Overlay();
	if (overlay == NULL)
		return;

	if (fVisible && !wasVisible)
		_UpdateOverlayView();
	else if (!fVisible && wasVisible)
		overlay->Hide();
}


// #pragma mark -


void
View::MarkBackgroundDirty()
{
	if (fBackgroundDirty)
		return;
	fBackgroundDirty = true;
	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->MarkBackgroundDirty();
}


void
View::AddTokensForViewsInRegion(BPrivate::PortLink& link, BRegion& region,
	BRegion* windowContentClipping)
{
	if (!fVisible)
		return;

	{
		// NOTE: use scope in order to reduce stack space requirements

		// This check will prevent descending the view hierarchy
		// any further than necessary
		IntRect screenBounds(Bounds());
		LocalToScreenTransform().Apply(&screenBounds);
		if (!region.Intersects((clipping_rect)screenBounds))
			return;

		// Unfortunately, we intersecting another region, but otherwise
		// we couldn't provide the exact update rect to the client
		BRegion localDirty = _ScreenClipping(windowContentClipping);
		localDirty.IntersectWith(&region);
		if (localDirty.CountRects() > 0) {
			link.Attach<int32>(fToken);
			link.Attach<BRect>(localDirty.Frame());
		}
	}

	for (View* child = FirstChild(); child; child = child->NextSibling())
		child->AddTokensForViewsInRegion(link, region, windowContentClipping);
}


void
View::PrintToStream() const
{
	printf("View:          %s\n", Name());
	printf("  fToken:           %" B_PRId32 "\n", fToken);
	printf("  fFrame:           IntRect(%" B_PRId32 ", %" B_PRId32 ", %" B_PRId32 ", %" B_PRId32 ")\n",
		fFrame.left, fFrame.top, fFrame.right, fFrame.bottom);
	printf("  fScrollingOffset: IntPoint(%" B_PRId32 ", %" B_PRId32 ")\n",
		fScrollingOffset.x, fScrollingOffset.y);
	printf("  fHidden:          %d\n", fHidden);
	printf("  fVisible:         %d\n", fVisible);
	printf("  fWindow:          %p\n", fWindow);
	printf("  fParent:          %p\n", fParent);
	printf("  fLocalClipping:\n");
	fLocalClipping.PrintToStream();
	printf("  fScreenClipping:\n");
	fScreenClipping.PrintToStream();
	printf("  valid:            %d\n", fScreenClippingValid);

	printf("  fUserClipping:\n");
	if (fUserClipping.Get() != NULL)
		fUserClipping->PrintToStream();
	else
		printf("  none\n");

	printf("  fScreenAndUserClipping:\n");
	if (fScreenAndUserClipping.Get() != NULL)
		fScreenAndUserClipping->PrintToStream();
	else
		printf("  invalid\n");

	printf("  state:\n");
	printf("    user clipping:  %d\n", fDrawState->HasClipping());
	BPoint origin = fDrawState->CombinedOrigin();
	printf("    origin:         BPoint(%.1f, %.1f)\n", origin.x, origin.y);
	printf("    scale:          %.2f\n", fDrawState->CombinedScale());
	printf("\n");
}


void
View::RebuildClipping(bool deep)
{
	// the clipping spans over the bounds area
	fLocalClipping.Set((clipping_rect)Bounds());

	if (View* child = FirstChild()) {
		// if this view does not draw over children,
		// exclude all children from the clipping
		if ((fFlags & B_DRAW_ON_CHILDREN) == 0) {
			BRegion* childrenRegion = fWindow->GetRegion();
			if (!childrenRegion)
				return;

			for (; child; child = child->NextSibling()) {
				if (child->IsVisible()
					&& (child->fFlags & B_TRANSPARENT_BACKGROUND) == 0) {
					childrenRegion->Include((clipping_rect)child->Frame());
				}
			}

			fLocalClipping.Exclude(childrenRegion);
			fWindow->RecycleRegion(childrenRegion);
		}
		// if the operation is "deep", make children rebuild their
		// clipping too
		if (deep) {
			for (child = FirstChild(); child; child = child->NextSibling())
				child->RebuildClipping(true);
		}
	}

	// add the user clipping in case there is one
	if (fDrawState->HasClipping()) {
		// NOTE: in case the user sets a user defined clipping region,
		// rebuilding the clipping is a bit more expensive because there
		// is no separate "drawing region"... on the other
		// hand, views for which this feature is actually used will
		// probably not have any children, so it is not that expensive
		// after all
		if (fUserClipping.Get() == NULL) {
			fUserClipping.SetTo(new (nothrow) BRegion);
			if (fUserClipping.Get() == NULL)
				return;
		}

		fDrawState->GetCombinedClippingRegion(fUserClipping.Get());
	} else {
		fUserClipping.SetTo(NULL);
	}

	fScreenAndUserClipping.SetTo(NULL);
	fScreenClippingValid = false;
}


BRegion&
View::ScreenAndUserClipping(BRegion* windowContentClipping, bool force) const
{
	// no user clipping - return screen clipping directly
	if (fUserClipping.Get() == NULL)
		return _ScreenClipping(windowContentClipping, force);

	// combined screen and user clipping already valid
	if (fScreenAndUserClipping.Get() != NULL)
		return *fScreenAndUserClipping.Get();

	// build a new combined user and screen clipping
	fScreenAndUserClipping.SetTo(new (nothrow) BRegion(*fUserClipping.Get()));
	if (fScreenAndUserClipping.Get() == NULL)
		return fScreenClipping;

	LocalToScreenTransform().Apply(fScreenAndUserClipping.Get());
	fScreenAndUserClipping->IntersectWith(
		&_ScreenClipping(windowContentClipping, force));
	return *fScreenAndUserClipping.Get();
}


void
View::InvalidateScreenClipping()
{
// TODO: appearantly, we are calling ScreenClipping() on
// views who's parents don't have a valid screen clipping yet,
// this messes up the logic that for any given view with
// fScreenClippingValid == false, all children have
// fScreenClippingValid == false too. If this could be made the
// case, we could save some performance here with the commented
// out check, since InvalidateScreenClipping() might be called
// frequently.
// TODO: investigate, if InvalidateScreenClipping() could be
// called in "deep" and "non-deep" mode, ie. see if there are
// any cases where the children would still have valid screen
// clipping, even though the parent's screen clipping becomes
// invalid.
//	if (!fScreenClippingValid)
//		return;

	fScreenAndUserClipping.SetTo(NULL);
	fScreenClippingValid = false;
	// invalidate the childrens screen clipping as well
	for (View* child = FirstChild(); child; child = child->NextSibling()) {
		child->InvalidateScreenClipping();
	}
}


BRegion&
View::_ScreenClipping(BRegion* windowContentClipping, bool force) const
{
	if (!fScreenClippingValid || force) {
		fScreenClipping = fLocalClipping;
		LocalToScreenTransform().Apply(&fScreenClipping);

		// see if parts of our bounds are hidden underneath
		// the parent, the local clipping does not account for this
		IntRect clippedBounds = Bounds();
		ConvertToVisibleInTopView(&clippedBounds);
		if (clippedBounds.Width() < fScreenClipping.Frame().Width()
			|| clippedBounds.Height() < fScreenClipping.Frame().Height()) {
			BRegion temp;
			temp.Set((clipping_rect)clippedBounds);
			fScreenClipping.IntersectWith(&temp);
		}

		fScreenClipping.IntersectWith(windowContentClipping);
		fScreenClippingValid = true;
	}

	return fScreenClipping;
}


void
View::_MoveScreenClipping(int32 x, int32 y, bool deep)
{
	if (fScreenClippingValid) {
		fScreenClipping.OffsetBy(x, y);
		fScreenAndUserClipping.SetTo(NULL);
	}

	if (deep) {
		// move the childrens screen clipping as well
		for (View* child = FirstChild(); child; child = child->NextSibling()) {
			child->_MoveScreenClipping(x, y, deep);
		}
	}
}

