
#include <stdio.h>

#include <List.h>
#include <View.h> // for resize modes

#include "Desktop.h"
#include "DrawingEngine.h"
#include "WindowLayer.h"

#include "ViewLayer.h"

extern BWindow* wind;

// constructor
ViewLayer::ViewLayer(BRect frame, const char* name,
			 uint32 resizeMode, uint32 flags,
			 rgb_color viewColor)
	: fName(name),

	  fFrame(frame),
	  fScrollingOffset(0.0, 0.0),

	  fViewColor(viewColor),

	  fResizeMode(resizeMode),
	  fFlags(flags),

	  // ViewLayers start visible by default
	  fHidden(false),
	  fVisible(true),

	  fWindow(NULL),
	  fParent(NULL),

	  fFirstChild(NULL),
	  fPreviousSibling(NULL),
	  fNextSibling(NULL),
	  fLastChild(NULL),

	  fCurrentChild(NULL),

	  fLocalClipping(Bounds()),
	  fScreenClipping(),
	  fScreenClippingValid(false)
{
	fFrame.left = float((int32)fFrame.left);
	fFrame.top = float((int32)fFrame.top);
	fFrame.right = float((int32)fFrame.right);
	fFrame.bottom = float((int32)fFrame.bottom);
}

// destructor
ViewLayer::~ViewLayer()
{
	// iterate over children and delete each one
	ViewLayer* layer = fFirstChild;
	while (layer) {
		ViewLayer* toast = layer;
		layer = layer->fNextSibling;
		delete toast;
	}
}

// Bounds
BRect
ViewLayer::Bounds() const
{
	BRect bounds(fScrollingOffset.x, fScrollingOffset.y,
				 fScrollingOffset.x + fFrame.Width(),
				 fScrollingOffset.y + fFrame.Height());
	return bounds;
}

// ConvertToVisibleInTopView
void
ViewLayer::ConvertToVisibleInTopView(BRect* bounds) const
{
	*bounds = *bounds & Bounds();
	// NOTE: this step is necessary even if we don't have a parent!
	ConvertToParent(bounds);

	if (fParent)
		fParent->ConvertToVisibleInTopView(bounds);
}

// AttachedToWindow
void
ViewLayer::AttachedToWindow(WindowLayer* window)
{
	fWindow = window;

	for (ViewLayer* child = FirstChild(); child; child = NextChild())
		child->AttachedToWindow(window);
}


// DetachedFromWindow
void
ViewLayer::DetachedFromWindow()
{
	fWindow = NULL;
	for (ViewLayer* child = FirstChild(); child; child = NextChild())
		child->DetachedFromWindow();
}

// AddChild
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

	if (layer->IsVisible())
		RebuildClipping(false);

	if (fWindow) {
		layer->AttachedToWindow(fWindow);

		if (fVisible && layer->IsVisible()) {
			// trigger redraw
			BRect clippedFrame = layer->Frame();
			ConvertToVisibleInTopView(&clippedFrame);
			BRegion dirty(clippedFrame);
			fWindow->MarkContentDirty(&dirty);
		}
	}
}

// RemoveChild
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

	if (layer->IsVisible())
		RebuildClipping(false);

	if (fWindow) {
		layer->DetachedFromWindow();

		if (fVisible && layer->IsVisible()) {
			// trigger redraw
			BRect clippedFrame = layer->Frame();
			ConvertToVisibleInTopView(&clippedFrame);
			BRegion dirty(clippedFrame);
			fWindow->MarkContentDirty(&dirty);
		}
	}

	return true;
}

// FirstChild
ViewLayer*
ViewLayer::FirstChild() const
{
	fCurrentChild = fFirstChild;
	return fCurrentChild;
}

// PreviousChild
ViewLayer*
ViewLayer::PreviousChild() const
{
	fCurrentChild = fCurrentChild->fPreviousSibling;
	return fCurrentChild;
}

// NextChild
ViewLayer*
ViewLayer::NextChild() const
{
	fCurrentChild = fCurrentChild->fNextSibling;
	return fCurrentChild;
}

// LastChild
ViewLayer*
ViewLayer::LastChild() const
{
	fCurrentChild = fLastChild;
	return fCurrentChild;
}

// TopLayer
ViewLayer*
ViewLayer::TopLayer()
{
	// returns the top level view of the hirarchy,
	// it doesn't have to be the top level of a window

	if (fParent)
		return fParent->TopLayer();

	return this;
}

// CountChildren
uint32
ViewLayer::CountChildren(bool deep) const
{
	uint32 count = 0;
	for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
		count++;
		if (deep) {
			count += child->CountChildren(deep);
		}
	}
	return count;	
}

// CollectTokensForChildren
void
ViewLayer::CollectTokensForChildren(BList* tokenMap) const
{
	for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
		tokenMap->AddItem((void*)child);
		child->CollectTokensForChildren(tokenMap);
	}
}

// ViewAt
ViewLayer*
ViewLayer::ViewAt(const BPoint& where, BRegion* windowContentClipping)
{
	if (!fVisible)
		return NULL;

	if (ScreenClipping(windowContentClipping).Contains(where))
		return this;

	for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
		ViewLayer* layer = child->ViewAt(where, windowContentClipping);
		if (layer)
			return layer;
	}
	return NULL;
}

// ConvertToParent
void
ViewLayer::ConvertToParent(BPoint* point) const
{
	// remove scrolling offset and convert to parent coordinate space
	point->x += fFrame.left - fScrollingOffset.x;
	point->y += fFrame.top - fScrollingOffset.y;
}

// ConvertToParent
void
ViewLayer::ConvertToParent(BRect* rect) const
{
	// remove scrolling offset and convert to parent coordinate space
	rect->OffsetBy(fFrame.left - fScrollingOffset.x,
				   fFrame.top - fScrollingOffset.y);
}

// ConvertToParent
void
ViewLayer::ConvertToParent(BRegion* region) const
{
	// remove scrolling offset and convert to parent coordinate space
	region->OffsetBy(fFrame.left - fScrollingOffset.x,
					 fFrame.top - fScrollingOffset.y);
}

// ConvertFromParent
void
ViewLayer::ConvertFromParent(BPoint* point) const
{
	// remove scrolling offset and convert to parent coordinate space
	point->x += fScrollingOffset.x - fFrame.left;
	point->y += fScrollingOffset.y - fFrame.top;
}

// ConvertFromParent
void
ViewLayer::ConvertFromParent(BRect* rect) const
{
	// remove scrolling offset and convert to parent coordinate space
	rect->OffsetBy(fScrollingOffset.x - fFrame.left,
				   fScrollingOffset.y - fFrame.top);
}

// ConvertFromParent
void
ViewLayer::ConvertFromParent(BRegion* region) const
{
	// remove scrolling offset and convert to parent coordinate space
	region->OffsetBy(fScrollingOffset.x - fFrame.left,
					 fScrollingOffset.y - fFrame.top);
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BPoint* point) const
{
	ConvertToParent(point);

	if (fParent)
		fParent->ConvertToTop(point);
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BRect* rect) const
{
	ConvertToParent(rect);

	if (fParent)
		fParent->ConvertToTop(rect);
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BRegion* region) const
{
	ConvertToParent(region);

	if (fParent)
		fParent->ConvertToTop(region);
}

// ConvertFromTop
void
ViewLayer::ConvertFromTop(BPoint* point) const
{
	ConvertFromParent(point);

	if (fParent)
		fParent->ConvertFromTop(point);
}

// ConvertFromTop
void
ViewLayer::ConvertFromTop(BRect* rect) const
{
	ConvertFromParent(rect);

	if (fParent)
		fParent->ConvertFromTop(rect);
}

// ConvertFromTop
void
ViewLayer::ConvertFromTop(BRegion* region) const
{
	ConvertFromParent(region);

	if (fParent)
		fParent->ConvertFromTop(region);
}

// SetName
void
ViewLayer::SetName(const char* string)
{
	fName.SetTo(string);
}

// #pragma mark -

// MoveBy
void
ViewLayer::MoveBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);

	// to move on screen, we must not be hidden and we must have a parent
	if (fVisible && fParent && dirtyRegion) {
#if 0
// based on redraw on new location
		// the place were we are now visible
		BRect newVisibleBounds = Bounds();
		// we can use the frame of the old
		// local clipping to see which parts need invalidation
		BRect oldVisibleBounds(Bounds());
		oldVisibleBounds.OffsetBy(-x, -y);
		ConvertToTop(&oldVisibleBounds);

		ConvertToVisibleInTopView(&newVisibleBounds);

		dirtyRegion->Include(oldVisibleBounds);
		// newVisibleBounds already is in screen coords
		dirtyRegion->Include(newVisibleBounds);
#else
// blitting version, invalidates
// old contents
		BRect oldVisibleBounds(Bounds());
		oldVisibleBounds.OffsetBy(-x, -y);
		ConvertToTop(&oldVisibleBounds);

		BRect newVisibleBounds(Bounds());
		// NOTE: using ConvertToVisibleInTopView()
		// instead of ConvertToTop()! see below
		ConvertToVisibleInTopView(&newVisibleBounds);

		newVisibleBounds.OffsetBy(-x, -y);

		// clipping oldVisibleBounds to newVisibleBounds
		// makes sure we don't copy parts hidden under
		// parent views
		BRegion copyRegion(oldVisibleBounds & newVisibleBounds);
		fWindow->CopyContents(&copyRegion, x, y);

		BRegion dirty(oldVisibleBounds);
		newVisibleBounds.OffsetBy(x, y);
		dirty.Exclude(newVisibleBounds);
		dirtyRegion->Include(&dirty);
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
		InvalidateScreenClipping(true);
	}
}

// ResizeBy
void
ViewLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	if (fVisible && dirtyRegion) {
		BRect oldBounds(Bounds());
		oldBounds.right -= x;
		oldBounds.bottom -= y;

		BRegion dirty(Bounds());
		dirty.Include(oldBounds);

		if (!(fFlags & B_FULL_UPDATE_ON_RESIZE)) {
			// the dirty region is just the difference of
			// old and new bounds
			dirty.Exclude(oldBounds & Bounds());
		}

		InvalidateScreenClipping(true);

		if (dirty.CountRects() > 0) {
			// exclude children, they are expected to
			// include their own dirty regions in ParentResized()
			for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
				if (child->IsVisible()) {
					BRect previousChildVisible(child->Frame() & oldBounds & Bounds());
					if (dirty.Frame().Intersects(previousChildVisible)) {
						dirty.Exclude(previousChildVisible);
					}
				}
			}

			ConvertToTop(&dirty);
			dirtyRegion->Include(&dirty);
		}
	}

	// layout the children
	for (ViewLayer* child = FirstChild(); child; child = NextChild())
		child->ParentResized(x, y, dirtyRegion);

	// at this point, children are at their new locations,
	// so we can rebuild the clipping
	// TODO: when the implementation of Hide() and Show() is
	// complete, see if this should be avoided
	RebuildClipping(false);
}

// ParentResized
void
ViewLayer::ParentResized(int32 x, int32 y, BRegion* dirtyRegion)
{
	uint16 rm = fResizeMode & 0x0000FFFF;
	BRect newFrame = fFrame;

	// follow with left side
	if ((rm & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((rm & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x / 2;

	// follow with right side
	if ((rm & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((rm & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x / 2;

	// follow with top side
	if ((rm & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((rm & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y / 2;

	// follow with bottom side
	if ((rm & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((rm & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y / 2;

	if (newFrame != fFrame) {
		// careful, MoveBy will change fFrame
		int32 widthDiff = (int32)(newFrame.Width() - fFrame.Width());
		int32 heightDiff = (int32)(newFrame.Height() - fFrame.Height());

		MoveBy(newFrame.left - fFrame.left,
			   newFrame.top - fFrame.top, dirtyRegion);

		ResizeBy(widthDiff, heightDiff, dirtyRegion);
	}
}

// ScrollBy
void
ViewLayer::ScrollBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	// blitting version, invalidates
	// old contents

	// remember old bounds for tracking dirty region
	BRect oldBounds(Bounds());
	// find the area of the view that can be scrolled,
	// contents are shifted in the opposite direction from scrolling
	BRect stillVisibleBounds(oldBounds);
	stillVisibleBounds.OffsetBy(x, y);

	// NOTE: using ConvertToVisibleInTopView()
	// instead of ConvertToTop(), this makes
	// sure we don't try to move or invalidate an
	// area hidden underneath the parent view
	ConvertToVisibleInTopView(&oldBounds);
	ConvertToVisibleInTopView(&stillVisibleBounds);

	fScrollingOffset.x += x;
	fScrollingOffset.y += y;

	// do the blit, this will make sure
	// that other more complex dirty regions
	// are taken care of
	BRegion copyRegion(stillVisibleBounds);
	fWindow->CopyContents(&copyRegion, -x, -y);

	// find the dirty region as far as we are
	// concerned
	BRegion dirty(oldBounds);
	stillVisibleBounds.OffsetBy(-x, -y);
	dirty.Exclude(stillVisibleBounds);
	dirtyRegion->Include(&dirty);

	// the screen clipping of this view and it's
	// childs is no longer valid
	InvalidateScreenClipping(true);
	RebuildClipping(false);
}

// #pragma mark -

// Draw
void
ViewLayer::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping,
				BRegion* windowContentClipping, bool deep)
{
	// we can only draw within our own area
	BRegion redraw(ScreenClipping(windowContentClipping));
	// add the current clipping
	redraw.IntersectWith(effectiveClipping);

	// fill visible region with white
	drawingEngine->FillRegion(&redraw, (rgb_color){ 255, 255, 255, 255 });

	// let children draw
	if (deep) {
		// before passing the clipping on to children, exclude our
		// own region from the available clipping
		effectiveClipping->Exclude(&ScreenClipping(windowContentClipping));

		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->Draw(drawingEngine, effectiveClipping,
						windowContentClipping, deep);
		}
	}
}

// ClientDraw
void
ViewLayer::ClientDraw(DrawingEngine* drawingEngine, BRegion* effectiveClipping)
{
	BRect b(Bounds());
	b.OffsetTo(0.0, 0.0);
	ConvertToTop(&b);

	if (drawingEngine->Lock()) {
		drawingEngine->ConstrainClipping(effectiveClipping);

		// draw a frame with the view color
		drawingEngine->StrokeRect(b, fViewColor);
		b.InsetBy(1, 1);
		drawingEngine->StrokeRect(b, fViewColor);
		b.InsetBy(1, 1);
		drawingEngine->StrokeRect(b, fViewColor);
		b.InsetBy(1, 1);
		drawingEngine->StrokeLine(b.LeftTop(), b.RightBottom(), fViewColor);

		drawingEngine->Unlock();
	}
}

// #pragma mark -

// SetHidden
void
ViewLayer::SetHidden(bool hidden)
{
	// TODO: test...

	if (fHidden != hidden) {
		fHidden = hidden;

		// recurse into children and update their visible flag
		if (fParent) {
			bool olfVisible = fVisible;
			UpdateVisibleDeep(fParent->IsVisible());
			if (olfVisible != fVisible) {
				// include or exclude us from the parent area
				fParent->RebuildClipping(false);
				if (fWindow) {
					// trigger a redraw
					BRect clippedBounds = Bounds();
					ConvertToVisibleInTopView(&clippedBounds);
					BRegion dirty(clippedBounds);
					fWindow->MarkContentDirty(&dirty);
				}
			}
		} else {
			UpdateVisibleDeep(true);
		}
	}
}

// IsHidden
bool
ViewLayer::IsHidden() const
{
	return fHidden;
}

// UpdateVisibleDeep
void
ViewLayer::UpdateVisibleDeep(bool parentVisible)
{
	fVisible = parentVisible && !fHidden;
	for (ViewLayer* child = FirstChild(); child; child = NextChild())
		child->UpdateVisibleDeep(fVisible);
}

// PrintToStream
void
ViewLayer::PrintToStream() const
{
}

// RebuildClipping
void
ViewLayer::RebuildClipping(bool deep)
{
	// the clipping spans over the bounds area
	fLocalClipping.Set(Bounds());

	// exclude all childs from the clipping
	for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
		if (child->IsVisible())
			fLocalClipping.Exclude(child->Frame());

		if (deep)
			child->RebuildClipping(deep);
	}

	fScreenClippingValid = false;
}

// ScreenClipping
BRegion&
ViewLayer::ScreenClipping(BRegion* windowContentClipping, bool force) const
{
	if (!fScreenClippingValid || force) {
		fScreenClipping = fLocalClipping;
		ConvertToTop(&fScreenClipping);

		// see if we parts of our bounds are hidden underneath
		// the parent, the local clipping does not account for this
		BRect clippedBounds = Bounds();
		ConvertToVisibleInTopView(&clippedBounds);
		if (clippedBounds.Width() < fScreenClipping.Frame().Width() ||
			clippedBounds.Height() < fScreenClipping.Frame().Height()) {
			BRegion temp(clippedBounds);
			fScreenClipping.IntersectWith(&temp);
		}

		fScreenClipping.IntersectWith(windowContentClipping);
		fScreenClippingValid = true;
	}
	return fScreenClipping;
}

// InvalidateScreenClipping
void
ViewLayer::InvalidateScreenClipping(bool deep)
{
	fScreenClippingValid = false;
	if (deep) {
		// invalidate the childrens screen clipping as well
		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->InvalidateScreenClipping(deep);
		}
	}
}

// _MoveScreenClipping
void
ViewLayer::_MoveScreenClipping(int32 x, int32 y, bool deep)
{
	if (fScreenClippingValid)
		fScreenClipping.OffsetBy(x, y);

	if (deep) {
		// move the childrens screen clipping as well
		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->_MoveScreenClipping(x, y, deep);
		}
	}
}

