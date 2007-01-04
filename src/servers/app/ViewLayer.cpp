/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "ViewLayer.h"

#include "BitmapManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "Overlay.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#include "ServerPicture.h"
#include "ServerWindow.h"
#include "WindowLayer.h"

#include "drawing_support.h"

#include <List.h>
#include <Message.h>
#include <PortLink.h>
#include <View.h> // for resize modes
#include <WindowPrivate.h>

#include <stdio.h>

#include <new>

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


ViewLayer::ViewLayer(IntRect frame, IntPoint scrollingOffset, const char* name,
		int32 token, uint32 resizeMode, uint32 flags)
	:
	fName(name),
	fToken(token),

	fFrame(frame),
	fScrollingOffset(scrollingOffset),

	fViewColor(RGBColor(255, 255, 255)),
	fDrawState(new (nothrow) DrawState),
	fViewBitmap(NULL),

	fResizeMode(resizeMode),
	fFlags(flags),

	// ViewLayers start visible by default
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

	fLocalClipping(Bounds()),
	fScreenClipping(),
	fScreenClippingValid(false)
{
	if (fDrawState)
		fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


ViewLayer::~ViewLayer()
{
	if (fViewBitmap != NULL)
		gBitmapManager->DeleteBitmap(fViewBitmap);

	delete fDrawState;

//	if (fWindow && this == fWindow->TopLayer())
//		fWindow->SetTopLayer(NULL);
//
	// TODO: Don't know yet if we should also delete fPicture

	if (fCursor)
		fCursor->Release();

	// iterate over children and delete each one
	ViewLayer* layer = fFirstChild;
	while (layer) {
		ViewLayer* toast = layer;
		layer = layer->fNextSibling;
		delete toast;
	}
}


IntRect
ViewLayer::Bounds() const
{
	IntRect bounds(fScrollingOffset.x, fScrollingOffset.y,
				   fScrollingOffset.x + fFrame.Width(),
				   fScrollingOffset.y + fFrame.Height());
	return bounds;
}


void
ViewLayer::ConvertToVisibleInTopView(IntRect* bounds) const
{
	*bounds = *bounds & Bounds();
	// NOTE: this step is necessary even if we don't have a parent!
	ConvertToParent(bounds);

	if (fParent)
		fParent->ConvertToVisibleInTopView(bounds);
}


void
ViewLayer::AttachedToWindow(WindowLayer* window)
{
	fWindow = window;

	// an ugly hack to detect the desktop background
	if (window->Feel() == kDesktopWindowFeel && Parent() == TopLayer())
		fIsDesktopBackground = true;

	// insert view into local token space
	if (fWindow != NULL)
		fWindow->ServerWindow()->App()->ViewTokens().SetToken(fToken, B_HANDLER_TOKEN, this);

	// attach child views as well
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->AttachedToWindow(window);
}


void
ViewLayer::DetachedFromWindow()
{
	// remove view from local token space
	if (fWindow != NULL)
		fWindow->ServerWindow()->App()->ViewTokens().RemoveToken(fToken);

	fWindow = NULL;
	// detach child views as well
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->DetachedFromWindow();
}


// #pragma mark -


void
ViewLayer::AddChild(ViewLayer* layer)
{
	if (layer->fParent) {
		printf("ViewLayer::AddChild() - ViewLayer already has a parent\n");
		return;
	}
	
	layer->fParent = this;
	
	if (!fLastChild) {
		// no children yet
		fFirstChild = layer;
	} else {
		// append layer to formerly last child
		fLastChild->fNextSibling = layer;
		layer->fPreviousSibling = fLastChild;
	}
	fLastChild = layer;

	layer->UpdateVisibleDeep(fVisible);

	if (layer->IsVisible())
		RebuildClipping(false);

	if (fWindow) {
		layer->AttachedToWindow(fWindow);

		if (layer->IsVisible()) {
			// trigger redraw
			IntRect clippedFrame = layer->Frame();
			ConvertToVisibleInTopView(&clippedFrame);
			BRegion* dirty = fWindow->GetRegion();
			if (dirty) {
				dirty->Set((clipping_rect)clippedFrame);
				fWindow->MarkContentDirtyAsync(*dirty);
				fWindow->RecycleRegion(dirty);
			}
		}
	}
}


bool
ViewLayer::RemoveChild(ViewLayer* layer)
{
	if (layer->fParent != this) {
		printf("ViewLayer::RemoveChild(%p - %s) - ViewLayer is not child of this (%p) layer!\n", layer, layer ? layer->Name() : NULL, this);
		return false;
	}

	layer->fParent = NULL;

	if (fLastChild == layer)
		fLastChild = layer->fPreviousSibling;
		// layer->fNextSibling would be NULL

	if (fFirstChild == layer )
		fFirstChild = layer->fNextSibling;
		// layer->fPreviousSibling would be NULL

	// connect child before and after layer
	if (layer->fPreviousSibling)
		layer->fPreviousSibling->fNextSibling = layer->fNextSibling;

	if (layer->fNextSibling)
		layer->fNextSibling->fPreviousSibling = layer->fPreviousSibling;

	// layer has no siblings anymore
	layer->fPreviousSibling = NULL;
	layer->fNextSibling = NULL;

	if (layer->IsVisible()) {
		Overlay* overlay = _Overlay();
		if (overlay != NULL)
			overlay->Hide();

		RebuildClipping(false);
	}

	if (fWindow) {
		layer->DetachedFromWindow();

		if (fVisible && layer->IsVisible()) {
			// trigger redraw
			IntRect clippedFrame = layer->Frame();
			ConvertToVisibleInTopView(&clippedFrame);
			BRegion* dirty = fWindow->GetRegion();
			if (dirty) {
				dirty->Set((clipping_rect)clippedFrame);
				fWindow->MarkContentDirtyAsync(*dirty);
				fWindow->RecycleRegion(dirty);
			}
		}
	}

	return true;
}


ViewLayer*
ViewLayer::TopLayer()
{
	// returns the top level view of the hirarchy,
	// it doesn't have to be the top level of a window

	if (fParent)
		return fParent->TopLayer();

	return this;
}


uint32
ViewLayer::CountChildren(bool deep) const
{
	uint32 count = 0;
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
		count++;
		if (deep) {
			count += child->CountChildren(deep);
		}
	}
	return count;	
}


void
ViewLayer::CollectTokensForChildren(BList* tokenMap) const
{
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
		tokenMap->AddItem((void*)child);
		child->CollectTokensForChildren(tokenMap);
	}
}


ViewLayer*
ViewLayer::ViewAt(const BPoint& where, BRegion* windowContentClipping)
{
	if (!fVisible)
		return NULL;

	if (ScreenClipping(windowContentClipping).Contains(where))
		return this;

	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
		ViewLayer* layer = child->ViewAt(where, windowContentClipping);
		if (layer)
			return layer;
	}
	return NULL;
}


// #pragma mark -


void
ViewLayer::SetName(const char* string)
{
	fName.SetTo(string);
}


void
ViewLayer::SetFlags(uint32 flags)
{
	fFlags = flags;
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
ViewLayer::SetDrawingOrigin(BPoint origin)
{
	fDrawState->SetOrigin(origin);

	// rebuild clipping
	if (fDrawState->ClippingRegion())
		RebuildClipping(false);
}


BPoint
ViewLayer::DrawingOrigin() const
{
	BPoint origin(fDrawState->Origin());
	float scale = Scale();

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}


void
ViewLayer::SetScale(float scale)
{
	fDrawState->SetScale(scale);

	// rebuild clipping
	if (fDrawState->ClippingRegion())
		RebuildClipping(false);
}


float
ViewLayer::Scale() const
{
	return CurrentState()->Scale();
}


void
ViewLayer::SetUserClipping(const BRegion* region)
{
	fDrawState->SetClippingRegion(region);
	
	// rebuild clipping (for just this view)
	RebuildClipping(false);
}


void
ViewLayer::SetViewBitmap(ServerBitmap* bitmap, IntRect sourceRect,
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

		gBitmapManager->DeleteBitmap(fViewBitmap);
	}

	// the caller is allowed to delete the bitmap after setting the background
	if (bitmap != NULL)
		bitmap->Acquire();

	fViewBitmap = bitmap;
	fBitmapSource = sourceRect;
	fBitmapDestination = destRect;
	fBitmapResizingMode = resizingMode;
	fBitmapOptions = options;

	_UpdateOverlayView();
}


::Overlay*
ViewLayer::_Overlay() const
{
	if (fViewBitmap == NULL)
		return NULL;

	return fViewBitmap->Overlay();
}


void
ViewLayer::_UpdateOverlayView() const
{
	Overlay* overlay = _Overlay();
	if (overlay == NULL)
		return;

	IntRect destination = fBitmapDestination;
	ConvertToScreen(&destination);

	overlay->Configure(fBitmapSource, destination);
}


/*!
	This method is called whenever the window is resized or moved - would
	be nice to have a better solution for this, though.
*/
void
ViewLayer::UpdateOverlay()
{
	if (!IsVisible())
		return;

	if (_Overlay() != NULL) {
		_UpdateOverlayView();
	} else {
		// recursively ask children of this view

		for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
			child->UpdateOverlay();
		}
	}
}


// #pragma mark -


void
ViewLayer::ConvertToParent(BPoint* point) const
{
	// remove scrolling offset and convert to parent coordinate space
	point->x += fFrame.left - fScrollingOffset.x;
	point->y += fFrame.top - fScrollingOffset.y;
}


void
ViewLayer::ConvertToParent(IntPoint* point) const
{
	// remove scrolling offset and convert to parent coordinate space
	point->x += fFrame.left - fScrollingOffset.x;
	point->y += fFrame.top - fScrollingOffset.y;
}


void
ViewLayer::ConvertToParent(BRect* rect) const
{
	// remove scrolling offset and convert to parent coordinate space
	rect->OffsetBy(fFrame.left - fScrollingOffset.x,
				   fFrame.top - fScrollingOffset.y);
}


void
ViewLayer::ConvertToParent(IntRect* rect) const
{
	// remove scrolling offset and convert to parent coordinate space
	rect->OffsetBy(fFrame.left - fScrollingOffset.x,
				   fFrame.top - fScrollingOffset.y);
}


void
ViewLayer::ConvertToParent(BRegion* region) const
{
	// remove scrolling offset and convert to parent coordinate space
	region->OffsetBy(fFrame.left - fScrollingOffset.x,
					 fFrame.top - fScrollingOffset.y);
}


void
ViewLayer::ConvertFromParent(BPoint* point) const
{
	// convert from parent coordinate space amd add scrolling offset
	point->x += fScrollingOffset.x - fFrame.left;
	point->y += fScrollingOffset.y - fFrame.top;
}


void
ViewLayer::ConvertFromParent(IntPoint* point) const
{
	// convert from parent coordinate space amd add scrolling offset
	point->x += fScrollingOffset.x - fFrame.left;
	point->y += fScrollingOffset.y - fFrame.top;
}


void
ViewLayer::ConvertFromParent(BRect* rect) const
{
	// convert from parent coordinate space amd add scrolling offset
	rect->OffsetBy(fScrollingOffset.x - fFrame.left,
				   fScrollingOffset.y - fFrame.top);
}


void
ViewLayer::ConvertFromParent(IntRect* rect) const
{
	// convert from parent coordinate space amd add scrolling offset
	rect->OffsetBy(fScrollingOffset.x - fFrame.left,
				   fScrollingOffset.y - fFrame.top);
}


void
ViewLayer::ConvertFromParent(BRegion* region) const
{
	// convert from parent coordinate space amd add scrolling offset
	region->OffsetBy(fScrollingOffset.x - fFrame.left,
					 fScrollingOffset.y - fFrame.top);
}

//! converts a point from local to screen coordinate system 
void
ViewLayer::ConvertToScreen(BPoint* pt) const
{
	ConvertToParent(pt);

	if (fParent)
		fParent->ConvertToScreen(pt);
}


//! converts a point from local to screen coordinate system 
void
ViewLayer::ConvertToScreen(IntPoint* pt) const
{
	ConvertToParent(pt);

	if (fParent)
		fParent->ConvertToScreen(pt);
}


//! converts a rect from local to screen coordinate system 
void
ViewLayer::ConvertToScreen(BRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertToScreen(&offset);

	rect->OffsetBy(offset);
}


//! converts a rect from local to screen coordinate system 
void
ViewLayer::ConvertToScreen(IntRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertToScreen(&offset);

	rect->OffsetBy(offset);
}


//! converts a region from local to screen coordinate system 
void
ViewLayer::ConvertToScreen(BRegion* region) const
{
	BPoint offset(0.0, 0.0);
	ConvertToScreen(&offset);

	region->OffsetBy(offset.x, offset.y);
}


//! converts a point from screen to local coordinate system 
void
ViewLayer::ConvertFromScreen(BPoint* pt) const
{
	ConvertFromParent(pt);

	if (fParent)
		fParent->ConvertFromScreen(pt);
}


//! converts a point from screen to local coordinate system 
void
ViewLayer::ConvertFromScreen(IntPoint* pt) const
{
	ConvertFromParent(pt);

	if (fParent)
		fParent->ConvertFromScreen(pt);
}


//! converts a rect from screen to local coordinate system 
void
ViewLayer::ConvertFromScreen(BRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);

	rect->OffsetBy(offset.x, offset.y);
}


//! converts a rect from screen to local coordinate system 
void
ViewLayer::ConvertFromScreen(IntRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);

	rect->OffsetBy(offset.x, offset.y);
}


//! converts a region from screen to local coordinate system 
void
ViewLayer::ConvertFromScreen(BRegion* region) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);

	region->OffsetBy(offset.x, offset.y);
}


//! converts a point from local *drawing* to screen coordinate system 
void
ViewLayer::ConvertToScreenForDrawing(BPoint* point) const
{
	fDrawState->Transform(point);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(point);
}


//! converts a rect from local *drawing* to screen coordinate system 
void
ViewLayer::ConvertToScreenForDrawing(BRect* rect) const
{
	fDrawState->Transform(rect);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(rect);
}


//! converts a region from local *drawing* to screen coordinate system 
void
ViewLayer::ConvertToScreenForDrawing(BRegion* region) const
{
	fDrawState->Transform(region);
	// NOTE: from here on, don't use the
	// "*ForDrawing()" versions of the parent!
	ConvertToScreen(region);
}


//! converts a point from screen to local coordinate system 
void
ViewLayer::ConvertFromScreenForDrawing(BPoint* point) const
{
	ConvertFromScreen(point);
	fDrawState->InverseTransform(point);
}


// #pragma mark -


void
ViewLayer::MoveBy(int32 x, int32 y, BRegion* dirtyRegion)
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
		ConvertToScreen(&oldVisibleBounds);

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
		ConvertToScreen(&oldVisibleBounds);

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
ViewLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
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
			// exclude children, they are expected to
			// include their own dirty regions in ParentResized()
			for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
				if (child->IsVisible()) {
					IntRect previousChildVisible(child->Frame() & oldBounds & Bounds());
					if (dirty->Frame().Intersects(previousChildVisible)) {
						dirty->Exclude((clipping_rect)previousChildVisible);
					}
				}
			}

			ConvertToScreen(dirty);
			dirtyRegion->Include(dirty);
		}
		fWindow->RecycleRegion(dirty);
	}

	// layout the children
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->ParentResized(x, y, dirtyRegion);

	// view bitmap

	resize_frame(fBitmapDestination, fBitmapResizingMode, x, y);

	// at this point, children are at their new locations,
	// so we can rebuild the clipping
	// TODO: when the implementation of Hide() and Show() is
	// complete, see if this should be avoided
	RebuildClipping(false);
}


void
ViewLayer::ParentResized(int32 x, int32 y, BRegion* dirtyRegion)
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
ViewLayer::ScrollBy(int32 x, int32 y, BRegion* dirtyRegion)
{
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
ViewLayer::CopyBits(IntRect src, IntRect dst, BRegion& windowContentClipping)
{
	if (!fVisible || !fWindow)
		return;

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

	// we need to interstect the copyRegion too times, onces
	// at the source and once at the destination (here done
	// the other way arround but it doesn't matter)
	// the reason for this is that we are not supposed to visually
	// copy children in the source rect and neither to copy onto
	// children in the destination rect...
	copyRegion->Set((clipping_rect)visibleSrc);
	copyRegion->IntersectWith(&ScreenClipping(&windowContentClipping));
		// note that fScreenClipping is used directly from hereon
		// because it is now up to date

	copyRegion->OffsetBy(-xOffset, -yOffset);
	copyRegion->IntersectWith(&fScreenClipping);

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

	dirty->IntersectWith(&fScreenClipping);
	fWindow->MarkContentDirty(*dirty);

	fWindow->RecycleRegion(dirty);
	fWindow->RecycleRegion(copyRegion);
}


// #pragma mark -


void
ViewLayer::PushState()
{
	fDrawState = fDrawState->PushState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
ViewLayer::PopState()
{
	if (fDrawState->PreviousState() == NULL) {
		fprintf(stderr, "WARNING: User called BView(%s)::PopState(), "
			"but there is NO state on stack!\n", Name());
		return;
	}

	bool rebuildClipping = fDrawState->ClippingRegion() != NULL;

	fDrawState = fDrawState->PopState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);

	// rebuild clipping
	// (the clipping from the popped state is not effective anymore)
	if (rebuildClipping)
		RebuildClipping(false);
}


void
ViewLayer::SetEventMask(uint32 eventMask, uint32 options)
{
	fEventMask = eventMask;
	fEventOptions = options;
}


void
ViewLayer::SetCursor(ServerCursor *cursor)
{
	if (cursor != fCursor) {
		if (fCursor)
			fCursor->Release();

		fCursor = cursor;

		if (fCursor) {
			fCursor->Acquire();
			fCursor->SetPendingViewCursor(false);
		}
	}
}


void
ViewLayer::SetPicture(ServerPicture *picture)
{
	fPicture = picture;
}


ServerPicture *
ViewLayer::Picture() const
{
	return fPicture;
}


void
ViewLayer::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping,
	BRegion* windowContentClipping, bool deep)
{
	if (!fVisible) {
		// child views cannot be visible either
		return;
	}

	if (fViewBitmap != NULL || !fViewColor.IsTransparentMagic()) {
		// we can only draw within our own area
		BRegion* redraw = fWindow->GetRegion(ScreenClipping(windowContentClipping));
		if (!redraw)
			return;
		// add the current clipping
		redraw->IntersectWith(effectiveClipping);

		Overlay* overlayCookie = _Overlay();

		if (fViewBitmap != NULL && overlayCookie == NULL) {
			// draw view bitmap
			// TODO: support other options!
			BRect rect = fBitmapDestination;
			ConvertToScreenForDrawing(&rect);

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

// XXX: locking removed because the WindowLayer keeps the engine locked
// because it keeps track of syncing right now

			// lock the drawing engine for as long as we need the clipping
			// to be valid
			if (rect.IsValid()/* && drawingEngine->Lock()*/) {
				drawingEngine->ConstrainClippingRegion(redraw);

				DrawState defaultDrawState;

				if (fBitmapOptions & B_TILE_BITMAP) {
					// tile across entire view

					float start = rect.left;
					while (rect.top < redraw->Frame().bottom) {
						while (rect.left < redraw->Frame().right) {
							drawingEngine->DrawBitmap(fViewBitmap, fBitmapSource,
								rect, &defaultDrawState);
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
							rect, &defaultDrawState);
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
							rect, &defaultDrawState);
						rect.OffsetBy(0.0, rect.Height() + 1);
					}
					// remove vertical stripe from clipping
					rect.top = redraw->Frame().top;
					rect.bottom = redraw->Frame().bottom;
					redraw->Exclude(rect);
				} else {
					// no tiling at all

					drawingEngine->DrawBitmap(fViewBitmap, fBitmapSource,
						rect, &defaultDrawState);
					redraw->Exclude(rect);
				}

				// NOTE: It is ok not to reset the clipping, that
				// would only waste time
//				drawingEngine->Unlock();
			}

		}

		if (!fViewColor.IsTransparentMagic()) {
			// fill visible region with view color,
			// this version of FillRegion ignores any
			// clipping, that's why "redraw" needs to
			// be correct
			drawingEngine->FillRegion(*redraw, overlayCookie != NULL
				? overlayCookie->Color() : fViewColor);
		}

		fWindow->RecycleRegion(redraw);
	}

	fBackgroundDirty = false;

	// let children draw
	if (deep) {
		for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
			child->Draw(drawingEngine, effectiveClipping,
						windowContentClipping, deep);
		}
	}
}


void
ViewLayer::MouseDown(BMessage* message, BPoint where)
{
	// empty hook method
}


void
ViewLayer::MouseUp(BMessage* message, BPoint where)
{
	// empty hook method
}


void
ViewLayer::MouseMoved(BMessage* message, BPoint where)
{
	// empty hook method
}


// #pragma mark -


void
ViewLayer::SetHidden(bool hidden)
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
				BRegion* dirty = fWindow->GetRegion();
				if (!dirty)
					return;
				dirty->Set((clipping_rect)clippedBounds);
				fWindow->MarkContentDirty(*dirty);
				fWindow->RecycleRegion(dirty);
			}
		}
	}
}


bool
ViewLayer::IsHidden() const
{
	return fHidden;
}


void
ViewLayer::UpdateVisibleDeep(bool parentVisible)
{
	bool wasVisible = fVisible;

	fVisible = parentVisible && !fHidden;
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
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
ViewLayer::MarkBackgroundDirty()
{
	if (fBackgroundDirty)
		return;
	fBackgroundDirty = true;
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->MarkBackgroundDirty();
}


void
ViewLayer::AddTokensForLayersInRegion(BMessage* message,
									  BRegion& region,
									  BRegion* windowContentClipping)
{
	if (!fVisible)
		return;

	if (region.Intersects(ScreenClipping(windowContentClipping).Frame()))
		message->AddInt32("_token", fToken);

	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->AddTokensForLayersInRegion(message, region,
										  windowContentClipping);
}


void
ViewLayer::AddTokensForLayersInRegion(BPrivate::PortLink& link,
									  BRegion& region,
									  BRegion* windowContentClipping)
{
	if (!fVisible)
		return;

//	if (region.Intersects(ScreenClipping(windowContentClipping).Frame()))
	IntRect screenBounds(Bounds());
	ConvertToScreen(&screenBounds);
	if (!region.Intersects((clipping_rect)screenBounds))
		return;

	link.Attach<int32>(fToken);

	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling())
		child->AddTokensForLayersInRegion(link, region,
										  windowContentClipping);
}


void
ViewLayer::PrintToStream() const
{
	printf("ViewLayer:          %s\n", Name());
	printf("  fToken:           %ld\n", fToken);
	printf("  fFrame:           IntRect(%ld, %ld, %ld, %ld)\n", fFrame.left, fFrame.top, fFrame.right, fFrame.bottom);
	printf("  fScrollingOffset: IntPoint(%ld, %ld)\n", fScrollingOffset.x, fScrollingOffset.y);
	printf("  fHidden:          %d\n", fHidden);
	printf("  fVisible:         %d\n", fVisible);
	printf("  fWindow:          %p\n", fWindow);
	printf("  fParent:          %p\n", fParent);
	printf("  fLocalClipping:\n");
	fLocalClipping.PrintToStream();
	printf("  fScreenClipping:\n");
	fScreenClipping.PrintToStream();
	printf("  valid:            %d\n", fScreenClippingValid);
	printf("  state:\n");
	printf("    user clipping:  %p\n", fDrawState->ClippingRegion());
	printf("    origin:         BPoint(%.1f, %.1f)\n", fDrawState->Origin().x, fDrawState->Origin().y);
	printf("    scale:          %.2f\n", fDrawState->Scale());
	printf("\n");
}


void
ViewLayer::RebuildClipping(bool deep)
{
	// the clipping spans over the bounds area
	fLocalClipping.Set((clipping_rect)Bounds());

	// exclude all childs from the clipping
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
		if (child->IsVisible())
			fLocalClipping.Exclude((clipping_rect)child->Frame());

		if (deep)
			child->RebuildClipping(deep);
	}

	// add the user clipping in case there is one
	if (const BRegion* userClipping = fDrawState->ClippingRegion()) {
		// NOTE: in case the user sets a user defined clipping region,
		// rebuilding the clipping is a bit more expensive because there
		// is no separate "drawing region"... on the other
		// hand, views for which this feature is actually used will
		// probably not have any children, so it is not that expensive
		// after all
		BRegion* screenUserClipping = fWindow->GetRegion(*userClipping);
		if (!screenUserClipping)
			return;
		fDrawState->Transform(screenUserClipping);
		fLocalClipping.IntersectWith(screenUserClipping);
		fWindow->RecycleRegion(screenUserClipping);
	}

	fScreenClippingValid = false;
}


BRegion&
ViewLayer::ScreenClipping(BRegion* windowContentClipping, bool force) const
{
	if (!fScreenClippingValid || force) {
		fScreenClipping = fLocalClipping;
		ConvertToScreen(&fScreenClipping);

		// see if we parts of our bounds are hidden underneath
		// the parent, the local clipping does not account for this
		IntRect clippedBounds = Bounds();
		ConvertToVisibleInTopView(&clippedBounds);
		if (clippedBounds.Width() < fScreenClipping.Frame().Width() ||
			clippedBounds.Height() < fScreenClipping.Frame().Height()) {
			BRegion* temp = fWindow->GetRegion();
			if (temp) {
				temp->Set((clipping_rect)clippedBounds);
				fScreenClipping.IntersectWith(temp);
				fWindow->RecycleRegion(temp);
			}
		}

		fScreenClipping.IntersectWith(windowContentClipping);
		fScreenClippingValid = true;
	}
//printf("###ViewLayer(%s)::ScreenClipping():\n", Name());
//fScreenClipping.PrintToStream();
	return fScreenClipping;
}


void
ViewLayer::InvalidateScreenClipping()
{
	if (!fScreenClippingValid)
		return;

	fScreenClippingValid = false;
	// invalidate the childrens screen clipping as well
	for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
		child->InvalidateScreenClipping();
	}
}


void
ViewLayer::_MoveScreenClipping(int32 x, int32 y, bool deep)
{
	if (fScreenClippingValid)
		fScreenClipping.OffsetBy(x, y);

	if (deep) {
		// move the childrens screen clipping as well
		for (ViewLayer* child = FirstChild(); child; child = child->NextSibling()) {
			child->_MoveScreenClipping(x, y, deep);
		}
	}
}

