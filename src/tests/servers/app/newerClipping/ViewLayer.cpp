
#include <stdio.h>

#include <View.h> // for resize modes

#include "Desktop.h"
#include "DrawingEngine.h"

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

	  fFirstChild(NULL),
	  fPreviousSibling(NULL),
	  fNextSibling(NULL),
	  fLastChild(NULL),

	  fCurrentChild(NULL),

	  fLocalClipping(Bounds()),
	  fScreenClipping(),
	  fScreenClippingValid(false)
{
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

	if (fParent) {
		RebuildClipping(false);
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

	// TODO: track regions
	RebuildClipping(false);

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
ViewLayer::CountChildren() const
{
	uint32 count = 0;
	if (ViewLayer* layer = fFirstChild) {
		count++;
		while (layer->fNextSibling) {
			count++;
			layer = layer->fNextSibling;
		}
	}
	return count;	
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BPoint* point) const
{
	// remove scrolling offset and convert to parent coordinate space
	point->x += fFrame.left - fScrollingOffset.x;
	point->y += fFrame.top - fScrollingOffset.y;

	if (fParent)
		fParent->ConvertToTop(point);
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BRect* rect) const
{
	// remove scrolling offset and convert to parent coordinate space
	rect->OffsetBy(fFrame.left - fScrollingOffset.x,
				   fFrame.top - fScrollingOffset.y);

	if (fParent)
		fParent->ConvertToTop(rect);
}

// ConvertToTop
void
ViewLayer::ConvertToTop(BRegion* region) const
{
	// remove scrolling offset and convert to parent coordinate space
	region->OffsetBy(fFrame.left - fScrollingOffset.x,
					 fFrame.top - fScrollingOffset.y);

	if (fParent)
		fParent->ConvertToTop(region);
}

// SetName
void
ViewLayer::SetName(const char* string)
{
	fName.SetTo(string);
}

// MoveBy
void
ViewLayer::MoveBy(int32 x, int32 y)
{
	fFrame.OffsetBy(x, y);

	_InvalidateScreenClipping(true);
	// TODO: ...
}

// ResizeBy
void
ViewLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	BRect oldBounds(Bounds());

	fFrame.right += x;
	fFrame.bottom += y;

	// TODO: broken when getting smaller
	// ... either here or in WindowLayer::ResizeBy()

	BRegion dirty(Bounds());
	dirty.Exclude(oldBounds);
	if (dirty.CountRects() > 0) {
		ConvertToTop(&dirty);
		dirtyRegion->Include(&dirty);
	}

	RebuildClipping(false);
	_InvalidateScreenClipping(false);
	// TODO: layout children
	// TODO: ...
}

// ScrollBy
void
ViewLayer::ScrollBy(int32 x, int32 y)
{
	fScrollingOffset.x += x;
	fScrollingOffset.y += y;
	// TODO: CopyRegion...
	// TODO: ...
}

// Draw
void
ViewLayer::Draw(DrawingEngine* drawingEngine, BRegion* effectiveClipping, bool deep)
{
	if (drawingEngine->Lock()) {
		// TODO intersect with local dirtyRegion
		// fill visible region with view color
		drawingEngine->SetHighColor(fViewColor);
		drawingEngine->FillRegion(effectiveClipping);

		drawingEngine->MarkDirty(effectiveClipping);
		drawingEngine->Unlock();
	
		// let children draw
		if (deep) {
			for (ViewLayer* child = FirstChild(); child; child = NextChild()) {
				child->Draw(drawingEngine, effectiveClipping, deep);
			}
		}
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

