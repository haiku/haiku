
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
	  fShowLevel(1),

	  fWindow(NULL),
	  fParent(NULL),
	  fIsTopLayer(false),

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

// AttachedToWindow
void
ViewLayer::AttachedToWindow(WindowLayer* window, bool topLayer)
{
	fWindow = window;
	fIsTopLayer = topLayer;

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

	RebuildClipping(false);

	if (fWindow) {
		layer->AttachedToWindow(fWindow);
		fWindow->MarkDirty(&layer->ScreenClipping());
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

	if (fParent) {
		RebuildClipping(false);
	}
	if (fWindow) {
		layer->DetachedFromWindow();
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

// SetName
void
ViewLayer::SetName(const char* string)
{
	fName.SetTo(string);
}

#define HW_MOVE 0

// MoveBy
void
ViewLayer::MoveBy(int32 x, int32 y)
{
	if (x == 0 && y == 0)
		return;

#if HW_MOVE
	if (!fIsTopLayer && fWindow) {
		// blit to new location (children as well)
		BRect screenRect;
if (fParent) {
	screenRect = fFrame & fParent->Bounds();
	fParent->ConvertToTop(&screenRect);
} else {
	screenRect = Bounds();
	ConvertToTop(&screenRect);
}

		BRegion effectiveClippingRegion;
		fWindow->GetContentRegion(&effectiveClippingRegion);
		effectiveClippingRegion.IntersectWith(&fWindow->VisibleRegion());

		BRegion copyRegion(screenRect);
		copyRegion.IntersectWith(&effectiveClippingRegion);

		copyRegion.OffsetBy(x, y);
		copyRegion.IntersectWith(&effectiveClippingRegion);

		BRegion alreadyDirty(screenRect);
		{
			BRegion windowDirty(fWindow->DirtyRegion());
			alreadyDirty.IntersectWith(&windowDirty);
		}

		fFrame.OffsetBy(x, y);
		_MoveScreenClipping(x, y, true);

if (fParent) {
	screenRect = fFrame & fParent->Bounds();
	fParent->ConvertToTop(&screenRect);
} else {
	screenRect = Bounds();
	ConvertToTop(&screenRect);
}
		BRegion dirtyRegion(screenRect);
		dirtyRegion.Exclude(&copyRegion);

		alreadyDirty.OffsetBy(x, y);
		dirtyRegion.Include(&alreadyDirty);

		fWindow->MarkDirty(&dirtyRegion);

		copyRegion.OffsetBy(-x, -y);

		if (fWindow->GetDrawingEngine()->Lock()) {
			fWindow->GetDrawingEngine()->CopyRegion(&copyRegion, x, y);
			fWindow->GetDrawingEngine()->Unlock();
		}
	} else {
		fFrame.OffsetBy(x, y);
		_MoveScreenClipping(x, y, true);
	}
#else // HW_MOVE
	fFrame.OffsetBy(x, y);

	_MoveScreenClipping(x, y, true);

	if (!fIsTopLayer && fWindow) {
		BRect screenRect(Bounds());
		ConvertToTop(&screenRect);
		screenRect = screenRect | screenRect.OffsetByCopy(-x, -y);
		BRegion dirty(screenRect);

		fWindow->MarkDirty(&dirty);
	}
#endif // !HW_MOVE
}

// ResizeBy
void
ViewLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	BRect oldBounds(Bounds());

	fFrame.right += x;
	fFrame.bottom += y;

	BRegion dirty(Bounds());
	dirty.Include(oldBounds);
	if (!(fFlags & B_FULL_UPDATE_ON_RESIZE)) {
		// the dirty region is just the difference of
		// old and new bounds
		dirty.Exclude(oldBounds & Bounds());
	}

	_InvalidateScreenClipping(true);

	if (dirty.CountRects() > 0) {
		// exclude children, they are expected to
		// have included their own dirty regions
		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			BRect previousChildVisible(child->Frame() & oldBounds);
			if (dirty.Frame().Intersects(previousChildVisible)) {
				dirty.Exclude(previousChildVisible);
			}
		}

		ConvertToTop(&dirty);
		dirtyRegion->Include(&dirty);
	}

	// layout the children
	for (ViewLayer* child = FirstChild(); child; child = NextChild())
		child->ParentResized(x, y, dirtyRegion);

	// at this point, children are at their new locations,
	// so we can rebuild the clipping
	RebuildClipping(false);
}

// ScrollBy
void
ViewLayer::ScrollBy(int32 x, int32 y)
{
	fScrollingOffset.x += x;
	fScrollingOffset.y += y;
	// TODO: CopyRegion...
	// TODO: ...

	_InvalidateScreenClipping(true);
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
		// MoveBy will change fFrame, so cache it
		BRect oldFrame = fFrame;
		MoveBy(newFrame.left - oldFrame.left,
			   newFrame.top - oldFrame.top);

		ResizeBy(newFrame.Width() - oldFrame.Width(),
				 newFrame.Height() - oldFrame.Height(), dirtyRegion);
	}
}

// Draw
void
ViewLayer::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping, bool deep)
{
	// we can only draw within our own area
	BRegion redraw(ScreenClipping());
	// add the current clipping
	redraw.IntersectWith(effectiveClipping);

	if (drawingEngine->Lock()) {
		// fill visible region with white
		drawingEngine->SetHighColor(255, 255, 255);
		BRect b(Bounds());
		ConvertToTop(&b);
		drawingEngine->FillRegion(&redraw);

		drawingEngine->MarkDirty(&redraw);
		drawingEngine->Unlock();
	}

	// let children draw
	if (deep) {
		// before passing the clipping on to children, exclude our
		// own region from the available clipping
		effectiveClipping->Exclude(&ScreenClipping());

		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->Draw(drawingEngine, effectiveClipping, deep);
		}
	}
}

// ClientDraw
void
ViewLayer::ClientDraw(DrawingEngine* drawingEngine, BRegion* effectiveClipping)
{
	if (drawingEngine->Lock()) {
		drawingEngine->ConstrainClippingRegion(effectiveClipping);

		// draw a frame with the view color
		BRect b(Bounds());
		b.OffsetTo(0.0, 0.0);
		ConvertToTop(&b);
		drawingEngine->SetHighColor(fViewColor);
		drawingEngine->StrokeRect(b);
		drawingEngine->StrokeLine(b.LeftTop(), b.RightBottom());

		drawingEngine->ConstrainClippingRegion(NULL);

		drawingEngine->MarkDirty(effectiveClipping);
		drawingEngine->Unlock();
	}
}

// IsHidden
bool
ViewLayer::IsHidden() const
{
	// if we're explicitely hidden, then we're hidden...
	if (fShowLevel < 1)
		return true;

	// ...but if we're not hidden, we might still be hidden if our parent is
	if (fParent)
		return fParent->IsHidden();

	// nope, definitely not hidden
	return false;
}

// Hide
void
ViewLayer::Hide()
{
	fShowLevel--;

	// TODO: track regions
}

// Show
void
ViewLayer::Show()
{
	fShowLevel++;

	// TODO: track regions
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
	// remember current local clipping in dirty region
	BRegion oldLocalClipping(fLocalClipping);

	// the clipping spans over the bounds area
	fLocalClipping.Set(Bounds());

	// exclude all childs from the clipping
	for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
		fLocalClipping.Exclude(child->Frame());

		if (deep)
			child->RebuildClipping(deep);
	}

	fScreenClippingValid = false;
}

// ScreenClipping
BRegion&
ViewLayer::ScreenClipping() const
{
	if (!fScreenClippingValid) {
		fScreenClipping = fLocalClipping;
		ConvertToTop(&fScreenClipping);
		fScreenClippingValid = true;
	}
	return fScreenClipping;
}

// _InvalidateScreenClipping
void
ViewLayer::_InvalidateScreenClipping(bool deep)
{
	fScreenClippingValid = false;
	if (deep) {
		// invalidate the childrens screen clipping as well
		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->_InvalidateScreenClipping(deep);
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
		// invalidate the childrens screen clipping as well
		for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
			child->_MoveScreenClipping(x, y, deep);
		}
	}
}

