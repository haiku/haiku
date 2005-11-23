/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

/**	Class used for tracking drawing context and screen clipping.
 *	One Layer per client BWindow (WindowBorder) and each BView therein.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <AppDefs.h>
#include <Message.h>
#include <Region.h>
#include <View.h>

#include "DebugInfoManager.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "PortLink.h"
#include "RootLayer.h"
#include "ServerProtocol.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Layer.h"
#include "ServerBitmap.h"

//#define DEBUG_LAYER
#ifdef DEBUG_LAYER
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


Layer::Layer(BRect frame, const char* name, int32 token,
	uint32 resize, uint32 flags, DrawingEngine* driver)
	:
	fName(name),
	fFrame(frame),
	fScrollingOffset(0.0, 0.0),

	fDriver(driver),
	fRootLayer(NULL),
	fServerWin(NULL),
	fOwner(NULL),

	fDrawState(new DrawState),

	fParent(NULL),
	fPreviousSibling(NULL),
	fNextSibling(NULL),
	fFirstChild(NULL),
	fLastChild(NULL),

	fCurrent(NULL),

	fViewToken(token),
	fFlags(flags),
	fAdFlags(0),
	fResizeMode(resize),
	fEventMask(0UL),
	fEventOptions(0UL),

	fHidden(false),
	fIsTopLayer(false),
	fViewColor(255, 255, 255, 255),

	fBackgroundBitmap(NULL),
	fOverlayBitmap(NULL)
{
	if (!frame.IsValid()) {
		char helper[1024];
		sprintf(helper, "Layer::Layer(BRect(%.1f, %.1f, %.1f, %.1f), name: %s, token: %ld) - frame is invalid\n",
			frame.left, frame.top, frame.right, frame.bottom, name, token);
		CRITICAL(helper);
		fFrame.Set(0, 0, 1, 1);
	}

	if (!fDriver)
		CRITICAL("You MUST have a valid driver to init a Layer object\n");

	// NOTE: This flag is forwarded to a DrawState setting, even
	// though it is actually not part of a "state". However,
	// it is an important detail of a graphics context, and we
	// have no other means to pass this setting on to the DrawingEngine
	// other than through the DrawState. If we ever add a flag
	// B_ANTI_ALIASING to the view flags, it would have to be passed
	// in the same way. Though when breaking binary compatibility,
	// we might want to make this an actual part of a "state" (with
	// a different API to set these).
	// Note that the flag is also tested (updated) in Push/PopState and
	// SetFlags().
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);

	STRACE(("Layer(%s) successfuly created\n", Name()));
}


Layer::~Layer()
{
	delete fDrawState;

	Layer* child = fFirstChild;

	while (child != NULL) {
		Layer* nextChild = child->fNextSibling;

		delete child;
		child = nextChild;
	}
}


/*!
	\brief Adds a child layer to the current one
	\param layer a new child layer
	\param serverWin the serverwindow to which the layer will belong
	
	Unlike the BView version, if the layer already belongs to another, then
	it spits an error to stdout and returns.
*/
void
Layer::AddChild(Layer* layer, ServerWindow* serverWin)
{
	STRACE(("Layer(%s)::AddChild(%s) START\n", Name(), layer->Name()));
	
	if (layer->fParent != NULL) {
		printf("ERROR: AddChild(): Layer already has a parent\n");
		return;
	}
	
	// 1) attach layer to the tree structure
	layer->fParent = this;
	
	// if we have children already, bump the current last child back one and
	// make the new child the last layer
	if (fLastChild) {
		layer->fPreviousSibling = fLastChild;
		fLastChild->fNextSibling = layer;
	} else {
		fFirstChild = layer;
	}
	fLastChild = layer;

	// if we have no RootLayer yet, then there is no need to set any parameters --
	// they will be set when the RootLayer for this tree will be added
	// to the main tree structure.
	if (!fRootLayer) {
		STRACE(("Layer(%s)::AddChild(%s) END\n", Name(), layer->Name()));
		return;
	}

	// 2) Iterate over the newly-added layer and all its children, setting the 
	//	root layer and server window and also rebuilding the full-size region
	//	for every descendant of the newly-added layer
	
	//c = short for: current
	Layer* c = layer;
	Layer* stop = layer;
	while (true) {
		// action block

		// 2.1) set the RootLayer for this object.
		c->SetRootLayer(c->fParent->fRootLayer);
		
		// 2.2) this Layer must know if it has a ServerWindow object attached.
		c->fServerWin=serverWin;
		
		// tree parsing algorithm
		if (c->fFirstChild) {
			// go deep
			c = c->fFirstChild;
		} else {
			// go right or up
			
			if (c == stop) // our trip is over
				break;
				
			if (c->fNextSibling) {
				// go right
				c = c->fNextSibling;
			} else {
				// go up
				while (!c->fParent->fNextSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that's enough!
					break;
				
				c = c->fParent->fNextSibling;
			}
		}
	}

	STRACE(("Layer(%s)::AddChild(%s) END\n", Name(), layer->Name()));
}

/*!
	\brief Removes a child layer from the current one
	\param layer the layer to remove
	
	If the layer does not belong to the the current layer, then this function 
	spits out an error to stdout and returns
*/
void
Layer::RemoveChild(Layer *layer)
{
	STRACE(("Layer(%s)::RemoveChild(%s) START\n", Name(), layer->Name()));
	
	if (!layer->fParent) {
		printf("ERROR: RemoveChild(): Layer doesn't have a parent\n");
		return;
	}
	
	if (layer->fParent != this) {
		printf("ERROR: RemoveChild(): Layer is not a child of this layer\n");
		return;
	}

	// 1) remove this layer from the main tree.
	
	// Take care of fParent
	layer->fParent = NULL;
	
	if (fFirstChild == layer)
		fFirstChild = layer->fNextSibling;
	
	if (fLastChild == layer)
		fLastChild = layer->fPreviousSibling;
	
	// Take care of siblings
	if (layer->fPreviousSibling != NULL)
		layer->fPreviousSibling->fNextSibling	= layer->fNextSibling;
	
	if (layer->fNextSibling != NULL)
		layer->fNextSibling->fPreviousSibling = layer->fPreviousSibling;
	
	layer->fPreviousSibling = NULL;
	layer->fNextSibling = NULL;
	layer->_ClearVisibleRegions();

	// 2) Iterate over all of the removed-layer's descendants and unset the
	//	root layer, server window, and all redraw-related regions
	
	Layer* c = layer; //c = short for: current
	Layer* stop = layer;
	
	while (true) {
		// action block
		{
			// 2.1) set the RootLayer for this object.
			c->SetRootLayer(NULL);
			// 2.2) this Layer must know if it has a ServerWindow object attached.
			c->fServerWin = NULL;
		}

		// tree parsing algorithm
		if (c->fFirstChild) {	
			// go deep
			c = c->fFirstChild;
		} else {	
			// go right or up
			if (c == stop) // out trip is over
				break;

			if (c->fNextSibling) {
				// go right
				c = c->fNextSibling;
			} else {
				// go up
				while(!c->fParent->fNextSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that enough!
					break;
				
				c = c->fParent->fNextSibling;
			}
		}
	}
	STRACE(("Layer(%s)::RemoveChild(%s) END\n", Name(), layer->Name()));
}

//! Removes the calling layer from the tree
void
Layer::RemoveSelf()
{
	// A Layer removes itself from the tree (duh)
	if (fParent == NULL) {
		printf("ERROR: RemoveSelf(): Layer doesn't have a parent\n");
		return;
	}
	fParent->RemoveChild(this);
}

/*!
	\return true if the child is owned by this Layer, false if not
*/
bool
Layer::HasChild(Layer* layer)
{
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (child == layer)
			return true;
	}
	return false;
}

//! Returns the number of children
uint32
Layer::CountChildren() const
{
	uint32 count = 0;
	Layer* child = FirstChild();
	while (child != NULL) {
		child = NextChild();
		count++;
	}
	return count;
}

/*!
	\brief Finds a child of the caller based on its token ID
	\param token ID of the layer to find
	\return Pointer to the layer or NULL if not found
*/
Layer*
Layer::FindLayer(const int32 token)
{
	// (recursive) search for a layer based on its view token
	
	// iterate only over direct child layers first
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (child->fViewToken == token)
			return child;
	}
	
	// try a recursive search
	for (Layer* child = FirstChild(); child; child = NextChild()) {
		if (Layer* layer = child->FindLayer(token))
			return layer;
	}
	
	return NULL;
}

/*!
	\brief Returns the layer at the given point
	\param pt The point to look the layer at
	\return The layer containing the point or NULL if no layer found
*/
Layer*
Layer::LayerAt(const BPoint &pt, bool recursive)
{
	//printf("%p:%s:LayerAt(x = %g, y = %g)\n", this, Name(), pt.x, pt.y);
	if (!recursive)	{
		if (VisibleRegion().Contains(pt))
			return this;

		for (Layer* child = LastChild(); child; child = PreviousChild())
			if (child->FullVisible().Contains(pt))
				return child;

		return NULL;
	}

	if (fVisible.Contains(pt))
		return this;

	if (fFullVisible.Contains(pt)) {
		for (Layer* child = LastChild(); child; child = PreviousChild()) {
			if (Layer* layer = child->LayerAt(pt))
				return layer;
		}
	}

	return NULL;
}

// FirstChild
Layer*
Layer::FirstChild() const
{
	fCurrent = fFirstChild;
	return fCurrent;
}

// NextChild
Layer*
Layer::NextChild() const
{
	fCurrent = fCurrent->fNextSibling;
	return fCurrent;
}

// PreviousChild
Layer*
Layer::PreviousChild() const
{
	fCurrent = fCurrent->fPreviousSibling;
	return fCurrent;
}

// LastChild
Layer*
Layer::LastChild() const
{
	fCurrent = fLastChild;
	return fCurrent;
}

// SetName
void
Layer::SetName(const char* name)
{
	fName.SetTo(name);
}


void
Layer::SetUserClipping(const BRegion& region)
{
	fDrawState->SetClippingRegion(region);
	
	// rebuild clipping
	_RebuildDrawingRegion();
}


void
Layer::SetFlags(uint32 flags)
{
	fFlags = flags;
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
Layer::SetEventMask(uint32 eventMask, uint32 options)
{
	fEventMask = eventMask;
	fEventOptions = options;
}


void
Layer::Draw(const BRect &rect)
{
#ifdef DEBUG_LAYER
	printf("Layer(%s)::Draw: ", Name());
	rect.PrintToStream();
#endif	

	if (!ViewColor().IsTransparentMagic())
		fDriver->FillRect(rect, ViewColor());
}

/*!
	\brief Shows the layer
	\param invalidate Invalidate the region when showing the layer. defaults to true
*/
void
Layer::Show(bool invalidate)
{
	STRACE(("Layer(%s)::Show()\n", Name()));
	if(!IsHidden()) {
		// an ancestor may be hidden. OK, we're not visible,
		// but we're changing our visibility state
		fHidden	= false;
		return;
	}
	
	fHidden	= false;

	if (invalidate) {
		// compute the region this layer wants for itself
		BRegion	invalid;
		GetOnScreenRegion(invalid);
		if (invalid.CountRects() > 0) {
			fParent->MarkForRebuild(invalid);
			GetRootLayer()->MarkForRedraw(invalid);

			fParent->TriggerRebuild();
			GetRootLayer()->TriggerRedraw();
		}
	}
}

/*!
	\brief Shows the layer
	\param invalidate Invalidate the region when hiding the layer. defaults to true
*/
void
Layer::Hide(bool invalidate)
{
	STRACE(("Layer(%s)::Hide()\n", Name()));
	if (IsHidden()) {
		// an ancestor may be hidden. OK, we're not visible,
		// but we're changing our visibility state
		fHidden	= true;		
		return;
	}

	fHidden	= true;
	if (invalidate && fFullVisible.CountRects() > 0) {
		BRegion invalid(fFullVisible);

		fParent->MarkForRebuild(invalid);
		GetRootLayer()->MarkForRedraw(invalid);

		fParent->TriggerRebuild();
		GetRootLayer()->TriggerRedraw();
	}
}

//! Returns true if the layer is hidden
bool
Layer::IsHidden(void) const
{
	if (fHidden)
		return true;

	if (fParent)
			return fParent->IsHidden();
	
	return fHidden;
}


void
Layer::PushState()
{
	fDrawState = fDrawState->PushState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}


void
Layer::PopState()
{
	if (fDrawState->PreviousState() == NULL) {
		fprintf(stderr, "WARNING: User called BView(%s)::PopState(), but there is NO state on stack!\n", Name());
		return;
	}

	bool rebuildClipping = fDrawState->ClippingRegion() != NULL;

	fDrawState = fDrawState->PopState();
	fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);

	// rebuild clipping
	// (the clipping from the popped state is not effective anymore)
	if (rebuildClipping)
		_RebuildDrawingRegion();
}


//! Matches the BView call of the same name
BRect
Layer::Bounds() const
{
	BRect r(fFrame);
	r.OffsetTo(ScrollingOffset());
	return r;
}


//! Matches the BView call of the same name
BRect
Layer::Frame() const
{
	return fFrame;
}


//! Moves the layer by specified values, complete with redraw
void
Layer::MoveBy(float x, float y)
{
	STRACE(("Layer(%s)::MoveBy()\n", Name()));

	if (x == 0.0f && y == 0.0f)
		return;

	// must lock, even if we change frame coordinates
	if (fParent && !IsHidden() && GetRootLayer() && GetRootLayer()->Lock()) {
		fFrame.OffsetBy(x, y);

		BRegion oldFullVisible(fFullVisible);

		// we'll invalidate the old position and the new, maxmial one.
		BRegion invalid;
		GetOnScreenRegion(invalid);
		invalid.Include(&fFullVisible);

		fParent->MarkForRebuild(invalid);
		fParent->TriggerRebuild();

		// done rebuilding regions, now copy common parts and redraw regions that became visible

		// include the actual and the old fullVisible regions. later, we'll exclude the common parts.
		BRegion	redrawReg(fFullVisible);
		redrawReg.Include(&oldFullVisible);

		// offset to layer's new location so that we can calculate the common region.
		oldFullVisible.OffsetBy(x, y);

		// finally we have the region that needs to be redrawn.
		redrawReg.Exclude(&oldFullVisible);

		// by intersecting the old fullVisible offseted to layer's new location, with the current
		// fullVisible, we'll have the common region which can be copied using HW acceleration.
		oldFullVisible.IntersectWith(&fFullVisible);

		// offset back and instruct the HW to do the actual copying.
		oldFullVisible.OffsetBy(-x, -y);
		GetDrawingEngine()->CopyRegion(&oldFullVisible, x, y);

		GetRootLayer()->MarkForRedraw(redrawReg);
		GetRootLayer()->TriggerRedraw();

		GetRootLayer()->Unlock();
	}
	else {
		// just offset to the new position
		fFrame.OffsetBy(x, y);
	}

	MovedByHook(x, y);

	_SendViewCoordUpdateMsg();
}


//! Resize the layer by the specified amount, complete with redraw
void
Layer::ResizeBy(float x, float y)
{
	STRACE(("Layer(%s)::ResizeBy()\n", Name()));

	if (x == 0.0f && y == 0.0f)
		return;

	// must lock, even if we change frame coordinates
	if (fParent && !IsHidden() && GetRootLayer() && GetRootLayer()->Lock()) {
		fFrame.Set(fFrame.left, fFrame.top, fFrame.right+x, fFrame.bottom+y);

// TODO: you should call this hook function AFTER all region rebuilding
//		 and redrawing stuff
		ResizedByHook(x, y, false);

// TODO: ResizedByHook(x,y,true) is called from inside _ResizeLayerFrameBy
//		 Should call this AFTER region rebuilding and redrawing.
		// resize children using their resize_mask.
		for (Layer *child = LastChild(); child != NULL; child = PreviousChild())
			child->_ResizeLayerFrameBy(x, y);

		BRegion oldFullVisible(fFullVisible);
		// this is required to invalidate the old border
		BRegion oldVisible(fVisible);

		// in case they moved, bottom, right and center aligned layers must be redrawn
		BRegion redrawMore;
		_RezizeLayerRedrawMore(redrawMore, x, y);

		// we'll invalidate the old area and the new, maxmial one.
		BRegion invalid;
		GetOnScreenRegion(invalid);
		invalid.Include(&fFullVisible);

		fParent->MarkForRebuild(invalid);
		fParent->TriggerRebuild();

		// done rebuilding regions, now redraw regions that became visible

		// what's invalid, are the differences between to old and the new fullVisible region
		// 1) in case we grow.
		BRegion redrawReg(fFullVisible);
		redrawReg.Exclude(&oldFullVisible);
		// 2) in case we shrink
		BRegion redrawReg2(oldFullVisible);
		redrawReg2.Exclude(&fFullVisible);
		// 3) combine.
		redrawReg.Include(&redrawReg2);

		// for center, right and bottom alligned layers, redraw their old positions
		redrawReg.Include(&redrawMore);

		// layers that had their frame modified must be entirely redrawn.
		_RezizeLayerRedrawMore(redrawReg, x, y);

		// include layer's visible region in case we want a full update on resize
		if (fFlags & B_FULL_UPDATE_ON_RESIZE && fVisible.Frame().IsValid()) {
			_ResizeLayerFullUpdateOnResize(redrawReg, x, y);

			redrawReg.Include(&fVisible);
			redrawReg.Include(&oldVisible);
		}

		GetRootLayer()->MarkForRedraw(redrawReg);
		GetRootLayer()->TriggerRedraw();

		GetRootLayer()->Unlock();
	}
	// just resize our frame and those of out descendants if their resize mask says so
	else {
		fFrame.Set(fFrame.left, fFrame.top, fFrame.right+x, fFrame.bottom+y);

// TODO: you should call this hook function AFTER all region rebuilding
//		 and redrawing stuff
		ResizedByHook(x, y, false);

// TODO: ResizedByHook(x,y,true) is called from inside _ResizeLayerFrameBy
//		 Should call this AFTER region rebuilding and redrawing.
		// resize children using their resize_mask.
		for (Layer *child = LastChild(); child != NULL; child = PreviousChild())
			child->_ResizeLayerFrameBy(x, y);
	}

	_SendViewCoordUpdateMsg();
}


//! scrolls the layer by the specified amount, complete with redraw
void
Layer::ScrollBy(float x, float y)
{
	STRACE(("Layer(%s)::ScrollBy()\n", Name()));

	if (x == 0.0f && y == 0.0f)
		return;

	fScrollingOffset.x += x;
	fScrollingOffset.y += y;

	// must lock, even if we change frame/origin coordinates
	if (!IsHidden() && GetRootLayer() && GetRootLayer()->Lock()) {

		// set the region to be invalidated.
		BRegion invalid(fFullVisible);

		MarkForRebuild(invalid);

		TriggerRebuild();

		// for the moment we say that the whole surface needs to be redraw.
		BRegion redrawReg(fFullVisible);

		// offset old region so that we can start comparing.
		invalid.OffsetBy(x, y);

		// compute the common region. we'll use HW acc to copy this to the new location.
		invalid.IntersectWith(&fFullVisible);
		GetDrawingEngine()->CopyRegion(&invalid, -x, -y);

		// common region goes back to its original location. then, by excluding
		// it from curent fullVisible we'll obtain the region that needs to be redrawn.
		invalid.OffsetBy(-x, -y);
// TODO: a quick fix for the scrolling problem!!! FIX THIS!
//		redrawReg.Exclude(&invalid);

		GetRootLayer()->MarkForRedraw(redrawReg);
		GetRootLayer()->TriggerRedraw();

		GetRootLayer()->Unlock();
	}

	ScrolledByHook(x, y);

// TODO: I think we should update the client-side that bounds rect has modified
//	SendViewCoordUpdateMsg();
}

void
Layer::CopyBits(BRect& src, BRect& dst, int32 xOffset, int32 yOffset)
{
	// NOTE: The correct behaviour is this:
	// * The region that is copied is the
	//   src rectangle, no matter if it fits
	//   into the dst rectangle. It is copied
	//   by the offset dst.LeftTop() - src.LeftTop()
	// * The dst rectangle is used for invalidation:
	//   Any area in the dst rectangle that could
	//   not be copied from src (because either the
	//   src rectangle was not big enough, or because there
	//   were parts cut off by the current layer clipping),
	//   are triggering BView::Draw() to be called
	//   and for these parts only.

	if (!GetRootLayer())
		return;

	if (!IsHidden() && GetRootLayer()->Lock()) {
		// the region that is going to be copied
		BRegion copyRegion(src);

		// apply the current clipping of the layer
		copyRegion.IntersectWith(&fVisible);

		// offset the region to the destination
		// and apply the current clipping there as well
		copyRegion.OffsetBy(xOffset, yOffset);
		copyRegion.IntersectWith(&fVisible);

		// the region at the destination that needs invalidation
		BRegion redrawReg(dst);
		// exclude the region drawn by the copy operation
// TODO: quick fix for our scrolling problem. FIX THIS!
//		redrawReg.Exclude(&copyRegion);
		// apply the current clipping as well
		redrawReg.IntersectWith(&fVisible);

		// move the region back for the actual operation
		copyRegion.OffsetBy(-xOffset, -yOffset);

		GetDrawingEngine()->CopyRegion(&copyRegion, xOffset, yOffset);

		// trigger the redraw			
		GetRootLayer()->MarkForRedraw(redrawReg);
		GetRootLayer()->TriggerRedraw();

		GetRootLayer()->Unlock();
	}
}		


void
Layer::MouseDown(const BMessage *msg)
{
}


void
Layer::MouseUp(const BMessage *msg)
{
}


void
Layer::MouseMoved(const BMessage *msg)
{
}


void
Layer::MouseWheelChanged(const BMessage *msg)
{
}


void
Layer::WorkspaceActivated(int32 index, bool active)
{
}


void
Layer::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
}


void
Layer::Activated(bool active)
{
}


BPoint
Layer::ScrollingOffset() const
{
	return fScrollingOffset;
}


void
Layer::SetDrawingOrigin(BPoint origin)
{
	fDrawState->SetOrigin(origin);

	// rebuild clipping
	if (fDrawState->ClippingRegion())
		_RebuildDrawingRegion();
}


BPoint
Layer::DrawingOrigin() const
{
	BPoint origin(fDrawState->Origin());
	float scale = Scale();

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}


void
Layer::SetScale(float scale)
{
	fDrawState->SetScale(scale);

	// rebuild clipping
	if (fDrawState->ClippingRegion())
		_RebuildDrawingRegion();
}


float
Layer::Scale() const
{
	return CurrentState()->Scale();
}

void
Layer::SetViewColor(const RGBColor& color)
{
	fViewColor = color;
}


void
Layer::SetBackgroundBitmap(const ServerBitmap* bitmap)
{
	// TODO: What about reference counting?
	// "Release" old fBackgroundBitmap and "Aquire" new one?
	fBackgroundBitmap = bitmap;
}

// SetOverlayBitmap
void
Layer::SetOverlayBitmap(const ServerBitmap* bitmap)
{
	// TODO: What about reference counting?
	// "Release" old fOverlayBitmap and "Aquire" new one?
	fOverlayBitmap = bitmap;
}

void
Layer::MovedByHook(float dx, float dy)
{
	if (Window() && !IsTopLayer())
		_AddToViewsWithInvalidCoords();
}

void
Layer::ResizedByHook(float dx, float dy, bool automatic)
{
	if (Window() && !IsTopLayer())
		_AddToViewsWithInvalidCoords();
}

void
Layer::ScrolledByHook(float dx, float dy)
{
	// empty.
}

//! converts a point from local to parent's coordinate system 
void
Layer::ConvertToParent(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		pt->x -= origin.x;
		pt->y -= origin.y;
		pt->x += fFrame.left;
		pt->y += fFrame.top;
	}
}

//! converts a rect from local to parent's coordinate system 
void
Layer::ConvertToParent(BRect* rect) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		rect->OffsetBy(-origin.x, -origin.y);
		rect->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a region from local to parent's coordinate system 
void
Layer::ConvertToParent(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		reg->OffsetBy(-origin.x, -origin.y);
		reg->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a point from parent's to local coordinate system 
void
Layer::ConvertFromParent(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		pt->x += origin.x;
		pt->y += origin.y;
		pt->x -= fFrame.left;
		pt->y -= fFrame.top;
	}
}

//! converts a rect from parent's to local coordinate system 
void
Layer::ConvertFromParent(BRect* rect) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		rect->OffsetBy(origin.x, origin.y);
		rect->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a region from parent's to local coordinate system 
void
Layer::ConvertFromParent(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = ScrollingOffset();
		reg->OffsetBy(origin.x, origin.y);
		reg->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a point from local to screen coordinate system 
void
Layer::ConvertToScreen(BPoint* pt) const
{
	if (fParent) {
		ConvertToParent(pt);
		fParent->ConvertToScreen(pt);
	}
}

//! converts a rect from local to screen coordinate system 
void
Layer::ConvertToScreen(BRect* rect) const
{
	if (fParent) {
		ConvertToParent(rect);
		fParent->ConvertToScreen(rect);
	}
}

//! converts a region from local to screen coordinate system 
void
Layer::ConvertToScreen(BRegion* reg) const
{
	if (fParent) {
		ConvertToParent(reg);
		fParent->ConvertToScreen(reg);
	}
}

//! converts a point from screen to local coordinate system 
void
Layer::ConvertFromScreen(BPoint* pt) const
{
	if (fParent) {
		ConvertFromParent(pt);
		fParent->ConvertFromScreen(pt);
	}
}

//! converts a rect from screen to local coordinate system 
void
Layer::ConvertFromScreen(BRect* rect) const
{
	if (fParent) {
		ConvertFromParent(rect);
		fParent->ConvertFromScreen(rect);
	}
}

//! converts a region from screen to local coordinate system 
void
Layer::ConvertFromScreen(BRegion* reg) const
{
	if (fParent) {
		ConvertFromParent(reg);
		fParent->ConvertFromScreen(reg);
	}
}

//! converts a point from local *drawing* to screen coordinate system 
void
Layer::ConvertToScreenForDrawing(BPoint* pt) const
{
	if (fParent) {
		fDrawState->Transform(pt);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToParent(pt);
		fParent->ConvertToScreen(pt);
	}
}

//! converts a rect from local *drawing* to screen coordinate system 
void
Layer::ConvertToScreenForDrawing(BRect* rect) const
{
	if (fParent) {
		fDrawState->Transform(rect);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToParent(rect);
		fParent->ConvertToScreen(rect);
	}
}

//! converts a region from local *drawing* to screen coordinate system 
void
Layer::ConvertToScreenForDrawing(BRegion* region) const
{
	if (fParent) {
		fDrawState->Transform(region);
		// NOTE: from here on, don't use the
		// "*ForDrawing()" versions of the parent!
		ConvertToParent(region);
		fParent->ConvertToScreen(region);
	}
}

//! converts a point from screen to local coordinate system 
void
Layer::ConvertFromScreenForDrawing(BPoint* pt) const
{
	if (fParent) {
		ConvertFromParent(pt);
		fParent->ConvertFromScreen(pt);

		fDrawState->InverseTransform(pt);
	}
}

void
Layer::_ResizeLayerFrameBy(float x, float y)
{
	uint16 rm = fResizeMode & 0x0000FFFF;
	BRect newFrame = fFrame;

	if ((rm & 0x0F00U) == _VIEW_LEFT_ << 8)
		newFrame.left += 0.0f;
	else if ((rm & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((rm & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x/2;

	if ((rm & 0x000FU) == _VIEW_LEFT_)
		newFrame.right += 0.0f;
	else if ((rm & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((rm & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x/2;

	if ((rm & 0xF000U) == _VIEW_TOP_ << 12)
		newFrame.top += 0.0f;
	else if ((rm & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((rm & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y/2;

	if ((rm & 0x00F0U) == _VIEW_TOP_ << 4)
		newFrame.bottom += 0.0f;
	else if ((rm & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((rm & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y/2;

	if (newFrame != fFrame) {
		float offsetX, offsetY;
		float dx, dy;

		dx = newFrame.Width() - fFrame.Width();
		dy = newFrame.Height() - fFrame.Height();
		offsetX = newFrame.left - fFrame.left;
		offsetY = newFrame.top - fFrame.top;

		fFrame = newFrame;

		if (offsetX != 0.0f || offsetY != 0.0f) {
			MovedByHook(offsetX, offsetY);
		}

		if (dx != 0.0f || dy != 0.0f) {
			// call hook function
			ResizedByHook(dx, dy, true); // automatic

			for (Layer* child = LastChild(); child; child = PreviousChild())
				child->_ResizeLayerFrameBy(dx, dy);
		}
	}
}


void
Layer::_RezizeLayerRedrawMore(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer* child = LastChild(); child; child = PreviousChild()) {
		uint16 rm = child->fResizeMode & 0x0000FFFF;

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			// NOTE: this is not exactly corect, but it works :-)
			// Normaly we shoud've used the child's old, required region - the one returned
			// from get_user_region() with the old frame, and the current one. child->Bounds()
			// works for the moment so we leave it like this.

			// calculate the old bounds.
			BRect	oldBounds(child->Bounds());		
			if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT)
				oldBounds.right -=dx;
			if ((rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM)
				oldBounds.bottom -=dy;
			
			// compute the region that became visible because we got bigger OR smaller.
			BRegion	regZ(child->Bounds());
			regZ.Include(oldBounds);
			regZ.Exclude(oldBounds & child->Bounds());

			child->ConvertToScreen(&regZ);

			// intersect that with this'(not child's) fullVisible region
			regZ.IntersectWith(&fFullVisible);
			reg.Include(&regZ);

			child->_RezizeLayerRedrawMore(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);

			// above, OR this:
			// reg.Include(&child->fFullVisible);
		} else if (((rm & 0x0F0F) == (uint16)B_FOLLOW_RIGHT && dx != 0)
			|| ((rm & 0x0F0F) == (uint16)B_FOLLOW_H_CENTER && dx != 0)
			|| ((rm & 0xF0F0) == (uint16)B_FOLLOW_BOTTOM && dy != 0)
			|| ((rm & 0xF0F0) == (uint16)B_FOLLOW_V_CENTER && dy != 0)) {
			reg.Include(&child->fFullVisible);
		}
	}
}


void
Layer::_ResizeLayerFullUpdateOnResize(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer* child = LastChild(); child; child = PreviousChild()) {
		uint16 rm = child->fResizeMode & 0x0000FFFF;		

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			if (child->fFlags & B_FULL_UPDATE_ON_RESIZE && child->fVisible.CountRects() > 0)
				reg.Include(&child->fVisible);

			child->_ResizeLayerFullUpdateOnResize(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);
		}
	}
}


/*!
	\brief Returns the region of the layer that is within the screen region
*/
void
Layer::GetOnScreenRegion(BRegion &region)
{
	// 1) set to frame in screen coords
	BRect frame(Bounds());
	ConvertToScreen(&frame);
	region.Set(frame);

	// 2) intersect with screen region
	BRegion screenRegion(GetRootLayer()->Bounds());
	region.IntersectWith(&screenRegion);

/*
	// 3) impose user constrained regions
	DrawState *stackData = fDrawState;
	while (stackData) {
		if (stackData->ClippingRegion()) {
			reg.IntersectWith(stackData->ClippingRegion());
		}
		stackData = stackData->PreviousState();
	}*/
}


void
Layer::_RebuildVisibleRegions(const BRegion &invalid,
	const BRegion &parentLocalVisible, const Layer *startFrom)
{
/*
	// no point in continuing if this layer is hidden.
	if (fHidden)
		return;

	// no need to go deeper if the parent doesn't have a visible region anymore
	// and our fullVisible region is also empty.
	if (!parentLocalVisible.Frame().IsValid() && !(fFullVisible.CountRects() > 0))
		return;

	bool fullRebuild = false;

	// intersect maximum wanted region with the invalid region
	BRegion common;
	GetOnScreenRegion(common);
	common.IntersectWith(&invalid);

	// if the resulted region is not valid, this layer is not in the catchment area
	// of the region being invalidated
	if (!common.CountRects() > 0)
		return;

	// now intersect with parent's visible part of the region that was/is invalidated
	common.IntersectWith(&parentLocalVisible);

	// exclude the invalid region
	fFullVisible.Exclude(&invalid);
	fVisible.Exclude(&invalid);

	// put in what's really visible
	fFullVisible.Include(&common);

	// allow this layer to hide some parts from its children
	_ReserveRegions(common);

	for (Layer *child = LastChild(); child; child = PreviousChild()) {
		if (child == startFrom)
			fullRebuild = true;

		if (fullRebuild)
			child->_RebuildVisibleRegions(invalid, common, child->LastChild());

		// to let children know much they can take from parent's visible region
		common.Exclude(&child->fFullVisible);
	}

	// include what's left after all children took what they could.
	fVisible.Include(&common);
*/

// NOTE: I modified this method for the moment because of some issues that I have
//		 with the new public methods that I recently introduced.
//		 This code works very well, the single problem that it has it's that it
//		 rebuilds all the visible regions of its descendants.
// TODO: only rebuild what's needed. See above code.
// NOTE2: this does not affect the redrawing code.

	// no point in continuing if this layer is hidden.
	if (fHidden)
		return;

	// no need to go deeper if the parent doesn't have a visible region anymore
	if (!parentLocalVisible.Frame().IsValid())
		return;

	BRegion common;
	GetOnScreenRegion(common);

	// see how much you can take
	common.IntersectWith(&parentLocalVisible);
	fFullVisible = common;
	fVisible.MakeEmpty();

	// allow this layer to hide some parts from its children
	_ReserveRegions(common);

	for (Layer *child = LastChild(); child; child = PreviousChild()) {
		child->_RebuildVisibleRegions(invalid, common, child->LastChild());

		// to let children know much they can take from parent's visible region
		common.Exclude(&child->fFullVisible);
	}

	// include what's left after all children took what they could.
	fVisible.Include(&common);

	_RebuildDrawingRegion();
}

// _RebuildDrawingRegion
void
Layer::_RebuildDrawingRegion()
{
	fDrawingRegion = fVisible;
	// apply user clipping which is in native coordinate system
	if (const BRegion* userClipping = fDrawState->ClippingRegion()) {
		BRegion screenUserClipping(*userClipping);
		ConvertToScreenForDrawing(&screenUserClipping);
		fDrawingRegion.IntersectWith(&screenUserClipping);
	}
}

void
Layer::_ReserveRegions(BRegion &reg)
{
	// Empty for Layer objects
}

void
Layer::_ClearVisibleRegions()
{
	fDrawingRegion.MakeEmpty();

	fVisible.MakeEmpty();
	fFullVisible.MakeEmpty();
	for (Layer *child = LastChild(); child; child = PreviousChild())
		child->_ClearVisibleRegions();
}

// mark a region dirty so that the next region rebuild for us
// and our children will take this into account
void
Layer::MarkForRebuild(const BRegion &dirty)
{
	fDirtyForRebuild.Include(&dirty);
}

// this will trigger visible region recalculation for us and
// our descendants.
void 
Layer::TriggerRebuild()
{
	BRegion totalInvalidReg;

	_GetAllRebuildDirty(&totalInvalidReg);

	if (totalInvalidReg.CountRects() > 0) {
		BRegion localFullVisible(fFullVisible);

//		localFullVisible.IntersectWith(&totalInvalidReg);

		_ClearVisibleRegions();

		_RebuildVisibleRegions(totalInvalidReg, localFullVisible, LastChild());
	}
}

// find out the region for which we must rebuild the visible regions
void
Layer::_GetAllRebuildDirty(BRegion *totalReg)
{
	totalReg->Include(&fDirtyForRebuild);

	for (Layer *child = LastChild(); child; child = PreviousChild())
		child->_GetAllRebuildDirty(totalReg);

	fDirtyForRebuild.MakeEmpty();
}

void
Layer::_AllRedraw(const BRegion &invalid)
{
	// couldn't find a simpler way to send _UPDATE_ message to client.
	WinBorder *wb = dynamic_cast<WinBorder*>(this);
	if (wb)
		wb->RequestClientRedraw(invalid);

	if (fVisible.CountRects() > 0) {
		BRegion	updateReg(fVisible);
		updateReg.IntersectWith(&invalid);

		if (updateReg.CountRects() > 0) {
			fDriver->ConstrainClippingRegion(&updateReg);
			Draw(updateReg.Frame());
			fDriver->ConstrainClippingRegion(NULL);
		}
	}

	for (Layer *child = LastChild(); child != NULL; child = PreviousChild()) {
		if (!(child->IsHidden())) {
			BRegion common(child->fFullVisible);
			common.IntersectWith(&invalid);
			
			if (common.CountRects() > 0)
				child->_AllRedraw(invalid);
		}
	}
}


void
Layer::_AddToViewsWithInvalidCoords() const
{
	if (fServerWin) {
		fServerWin->ClientViewsWithInvalidCoords().AddInt32("_token", fViewToken);
		fServerWin->ClientViewsWithInvalidCoords().AddPoint("where", fFrame.LeftTop());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("width", fFrame.Width());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("height", fFrame.Height());
	}
}


void
Layer::_SendViewCoordUpdateMsg() const
{
	if (fServerWin && !fServerWin->ClientViewsWithInvalidCoords().IsEmpty()) {
		fServerWin->SendMessageToClient(&fServerWin->ClientViewsWithInvalidCoords());
		fServerWin->ClientViewsWithInvalidCoords().MakeEmpty();
	}
}
