//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Layer.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used for rendering to the frame buffer. One layer per 
//					view on screen and also for window decorators
//  
//------------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <AppDefs.h>
#include <Message.h>
#include <Region.h>
#include <View.h>

#include "DebugInfoManager.h"
#include "DisplayDriver.h"
#include "LayerData.h"
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

//#define DEBUG_LAYER_REBUILD
#ifdef DEBUG_LAYER_REBUILD
#	define RBTRACE(x) printf x
#else
#	define RBTRACE(x) ;
#endif

enum {
	B_LAYER_ACTION_NONE = 0,
	B_LAYER_ACTION_MOVE,
	B_LAYER_ACTION_RESIZE
};

Layer::Layer(BRect frame, const char* name, int32 token,
			 uint32 resize, uint32 flags, DisplayDriver* driver)
	: 
	fFrame(frame), // in parent coordinates
//	fBoundsLeftTop(0.0, 0.0),

	// Layer does not start out as a part of the tree
	fOwner(NULL),
	fParent(NULL),
	fUpperSibling(NULL),
	fLowerSibling(NULL),
	fTopChild(NULL),
	fBottomChild(NULL),
	fCurrent(NULL),

	// all regions (fVisible, fFullVisible, fFull) start empty
#ifndef NEW_CLIPPING
	fVisible(),
	fFullVisible(),
	fFull(),
	fFrameAction(B_LAYER_ACTION_NONE),
#else
	fVisible2(),
	fFullVisible2(),
#endif

#ifndef NEW_CLIPPING
	fClipReg(&fVisible),
#else
	fClipReg(&fVisible2),
#endif 
	fServerWin(NULL),
	fName(name),
	fViewToken(token),

	fFlags(flags),
	fResizeMode(resize),
	fEventMask(0UL),
	fEventOptions(0UL),
	fHidden(false),
	fIsTopLayer(false),

	fAdFlags(0),
	fClassID(AS_LAYER_CLASS),

	fDriver(driver),
	fLayerData(new LayerData()),

	fRootLayer(NULL),

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

	STRACE(("Layer(%s) successfuly created\n", Name()));
}

//! Destructor frees all allocated heap space
Layer::~Layer()
{
	delete fLayerData;

	// TODO: uncomment!
	//PruneTree();
	
//	fServerWin->RemoveChild(fDriver);
//	delete fDriver;
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
	
	// if we have children already, bump the current front child back one and
	// make the new child the frontmost layer
	if (fBottomChild) {
		layer->fUpperSibling = fBottomChild;
		fBottomChild->fLowerSibling	= layer;
	} else {
		fTopChild = layer;
	}
	fBottomChild = layer;

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
		
		// 2.3) we are attached to the main tree so build our full region.
#ifndef NEW_CLIPPING
		c->RebuildFullRegion();
#endif
		// tree parsing algorithm
		if (c->fTopChild) {
			// go deep
			c = c->fTopChild;
		} else {
			// go right or up
			
			if (c == stop) // out trip is over
				break;
				
			if (c->fLowerSibling) {
				// go right
				c = c->fLowerSibling;
			} else {
				// go up
				while (!c->fParent->fLowerSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that's enough!
					break;
				
				c = c->fParent->fLowerSibling;
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
		printf("ERROR: RemoveChild(): Layer doesn't have a fParent\n");
		return;
	}
	
	if (layer->fParent != this) {
		printf("ERROR: RemoveChild(): Layer is not a child of this layer\n");
		return;
	}

	// 1) remove this layer from the main tree.
	
	// Take care of fParent
	layer->fParent = NULL;
	
	if (fTopChild == layer)
		fTopChild = layer->fLowerSibling;
	
	if (fBottomChild == layer)
		fBottomChild = layer->fUpperSibling;
	
	// Take care of siblings
	if (layer->fUpperSibling != NULL)
		layer->fUpperSibling->fLowerSibling	= layer->fLowerSibling;
	
	if (layer->fLowerSibling != NULL)
		layer->fLowerSibling->fUpperSibling = layer->fUpperSibling;
	
	layer->fUpperSibling = NULL;
	layer->fLowerSibling = NULL;

#ifdef NEW_CLIPPING
	layer->clear_visible_regions();
#endif

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
			// 2.3) we were removed from the main tree so clear our full region.
#ifndef NEW_CLIPPING
			c->fFull.MakeEmpty();
			// 2.4) clear fullVisible region.
			c->fFullVisible.MakeEmpty();
			// 2.5) we don't have a visible region anymore.
			c->fVisible.MakeEmpty();
#endif
		}

		// tree parsing algorithm
		if (c->fTopChild) {	
			// go deep
			c = c->fTopChild;
		} else {	
			// go right or up
			if (c == stop) // out trip is over
				break;

			if (c->fLowerSibling) {
				// go right
				c = c->fLowerSibling;
			} else {
				// go up
				while(!c->fParent->fLowerSibling && c->fParent != stop)
					c = c->fParent;
				
				if (c->fParent == stop) // that enough!
					break;
				
				c = c->fParent->fLowerSibling;
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
		printf("ERROR: RemoveSelf(): Layer doesn't have a fParent\n");
		return;
	}
	fParent->RemoveChild(this);
}

/*!
	\brief Determins if the calling layer has the passed layer as a child
	\return true if the child is owned by the caller, false if not
*/
bool
Layer::HasChild(Layer* layer)
{
	for (Layer *lay = TopChild(); lay; lay = LowerSibling()) {
		if (lay == layer)
			return true;
	}
	return false;
}

//! Returns the number of children
uint32
Layer::CountChildren(void) const
{
	uint32 count = 0;
	Layer *lay = TopChild();
	while (lay != NULL) {
		lay	= LowerSibling();
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
	// recursive search for a layer based on its view token
	Layer* lay;
	Layer* trylay;
	
	// Search child layers first
	for (lay = TopChild(); lay; lay = LowerSibling()) {
		if (lay->fViewToken == token)
			return lay;
	}
	
	// Hmmm... not in this layer's children. Try lower descendants
	for (lay = TopChild(); lay != NULL; lay = LowerSibling()) {
		trylay = lay->FindLayer(token);
		if (trylay)
			return trylay;
	}
	
	// Well, we got this far in the function,
	// so apparently there is no match to be found
	return NULL;
}

/*!
	\brief Returns the layer at the given point
	\param pt The point to look the layer at
	\return The layer containing the point or NULL if no layer found
*/
Layer*
Layer::LayerAt(const BPoint &pt)
{
#ifndef NEW_CLIPPING
	if (fVisible.Contains(pt))
		return this;

	if (fFullVisible.Contains(pt)) {
		Layer *lay = NULL;
		for (Layer* child = BottomChild(); child; child = UpperSibling()) {
			lay = child->LayerAt(pt);
			if (lay)
				return lay;
		}
	}
#else
	if (fVisible2.Contains(pt))
		return this;

	if (fFullVisible2.Contains(pt)) {
		Layer *lay = NULL;
		for (Layer* child = BottomChild(); child; child = UpperSibling()) {
			lay = child->LayerAt(pt);
			if (lay)
				return lay;
		}
	}
#endif	
	return NULL;
}

// TopChild
Layer*
Layer::TopChild() const
{
	fCurrent = fTopChild;
	return fCurrent;
}

// LowerSibling
Layer*
Layer::LowerSibling() const
{
	fCurrent = fCurrent->fLowerSibling;
	return fCurrent;
}

// UpperSibling
Layer*
Layer::UpperSibling() const
{
	fCurrent = fCurrent->fUpperSibling;
	return fCurrent;
}

// BottomChild
Layer*
Layer::BottomChild() const
{
	fCurrent = fBottomChild;
	return fCurrent;
}

#ifndef NEW_CLIPPING

//! Rebuilds the layer's "completely visible" region
void
Layer::RebuildFullRegion(void)
{
	STRACE(("Layer(%s)::RebuildFullRegion()\n", Name()));
	
	if (fParent)
		fFull.Set(fParent->ConvertToTop(fFrame ));
	else
		fFull.Set(fFrame);

	// TODO: restrict to screen coordinates
	
	// TODO: Convert to screen coordinates

	LayerData *ld;
	ld = fLayerData;
	do {
		// clip to user region
		if (const BRegion* userClipping = ld->ClippingRegion())
			fFull.IntersectWith(userClipping);
		
	} while ((ld = ld->prevState));
}

// StartRebuildRegions
void
Layer::StartRebuildRegions( const BRegion& reg, Layer *target, uint32 action, BPoint& pt)
{
	STRACE(("Layer(%s)::StartRebuildRegions() START\n", Name()));
	RBTRACE(("\n\nLayer(%s)::StartRebuildRegions() START\n", Name()));
	if (!fParent)
		fFullVisible = fFull;
	
	BRegion oldVisible = fVisible;
	
	fVisible = fFullVisible;

	// Rebuild regions for children...
	for (Layer *lay = BottomChild(); lay; lay = UpperSibling()) {
		if (lay == target)
			lay->RebuildRegions(reg, action, pt, BPoint(0.0f, 0.0f));
		else
			lay->RebuildRegions(reg, B_LAYER_NONE, pt, BPoint(0.0f, 0.0f));
	}
	
	#ifdef DEBUG_LAYER_REBUILD
		printf("\nSRR: Layer(%s) ALMOST done regions:\n", Name());
		printf("\tVisible Region:\n");
		fVisible.PrintToStream();
		printf("\tFull Visible Region:\n");
		fFullVisible.PrintToStream();
	#endif
	
	BRegion redrawReg(fVisible);
	
	// if this is the first time
	if (oldVisible.CountRects() > 0)
		redrawReg.Exclude(&oldVisible);

	if (redrawReg.CountRects() > 0)
		fRootLayer->fRedrawReg.Include(&redrawReg);
	
	#ifdef DEBUG_LAYER_REBUILD
		printf("\nLayer(%s)::StartRebuildRegions() DONE. Results:\n", Name());
		printf("\tRedraw Region:\n");
		fRootLayer->fRedrawReg.PrintToStream();
		printf("\tCopy Region:\n");
		for (int32 k=0; k<fRootLayer->fCopyRegList.CountItems(); k++) {
			((BRegion*)(fRootLayer->fCopyRegList.ItemAt(k)))->PrintToStream();
			((BPoint*)(fRootLayer->fCopyList.ItemAt(k)))->PrintToStream();
		}
		printf("\n");
	#endif

	STRACE(("Layer(%s)::StartRebuildRegions() END\n", Name()));
	RBTRACE(("Layer(%s)::StartRebuildRegions() END\n", Name()));
}

// RebuildRegions
void
Layer::RebuildRegions( const BRegion& reg, uint32 action, BPoint pt, BPoint ptOffset)
{
	STRACE(("Layer(%s)::RebuildRegions() START\n", Name()));

	// TODO:/NOTE: this method must be executed as quickly as possible.
	
	// Currently SendView[Moved/Resized]Msg() simply constructs a message and calls
	// ServerWindow::SendMessageToClient(). This involves the alternative use of 
	// kernel and this code in the CPU, so there are a lot of context switches. 
	// This is NOT good at all!
	
	// One alternative would be the use of a BMessageQueue per ServerWindows OR only
	// one for app_server which will be emptied as soon as this critical operation ended.
	// Talk to DW, Gabe.
	
	BRegion	oldRegion;
	uint32 newAction = action;
	BPoint newPt = pt;
	BPoint newOffset = ptOffset; // used for resizing only
	
	BPoint dummyNewLocation;

	RRLabel1:
	switch(action) {
		case B_LAYER_NONE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_NONE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_NONE\n", Name()));
			oldRegion = fVisible;
			break;
		}
		case B_LAYER_MOVE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_MOVE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_MOVE\n", Name()));
			oldRegion = fFullVisible;
			fFrame.OffsetBy(pt.x, pt.y);
			fFull.OffsetBy(pt.x, pt.y);
			
			// TODO: investigate combining frame event messages for efficiency
			//SendViewMovedMsg();
			AddToViewsWithInvalidCoords();

			newAction	= B_LAYER_SIMPLE_MOVE;
			break;
		}
		case B_LAYER_SIMPLE_MOVE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_SIMPLE_MOVE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_SIMPLE_MOVE\n", Name()));
			fFull.OffsetBy(pt.x, pt.y);
			
			break;
		}
		case B_LAYER_RESIZE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
			oldRegion	= fVisible;
			
			fFrame.right	+= pt.x;
			fFrame.bottom	+= pt.y;
			RebuildFullRegion();
			
			// TODO: investigate combining frame event messages for efficiency
			//SendViewResizedMsg();
			AddToViewsWithInvalidCoords();
			
			newAction = B_LAYER_MASK_RESIZE;
			break;
		}
		case B_LAYER_MASK_RESIZE: {
			RBTRACE(("1) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
			STRACE(("1) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
			oldRegion = fVisible;
			
			BPoint offset, rSize;
			BPoint coords[2];
			
			ResizeOthers(pt.x, pt.y, coords, NULL);
			offset = coords[0];
			rSize = coords[1];
			newOffset = offset + ptOffset;
			
			if (!(rSize.x == 0.0f && rSize.y == 0.0f)) {
				fFrame.OffsetBy(offset);
				fFrame.right += rSize.x;
				fFrame.bottom += rSize.y;
				RebuildFullRegion();
				
				// TODO: investigate combining frame event messages for efficiency
				//SendViewResizedMsg();
				AddToViewsWithInvalidCoords();
				
				newAction = B_LAYER_MASK_RESIZE;
				newPt = rSize;
				dummyNewLocation = newOffset;
			} else {
				if (!(offset.x == 0.0f && offset.y == 0.0f)) {
					pt = newOffset;
					action = B_LAYER_MOVE;
					newPt = pt;
					goto RRLabel1;
				} else {
					pt = ptOffset;
					action = B_LAYER_MOVE;
					newPt = pt;
					goto RRLabel1;
				}
			}
			break;
		}
	}

	if (!IsHidden()) {
		#ifdef DEBUG_LAYER_REBUILD
			printf("Layer(%s) real action START\n", Name());
			fFull.PrintToStream();
		#endif
		fFullVisible.MakeEmpty();
		fVisible = fFull;
		
		if (fParent && fVisible.CountRects() > 0) {
			// not the usual case, but support fot this is needed.
			if (fParent->fAdFlags & B_LAYER_CHILDREN_DEPENDANT) {
				#ifdef DEBUG_LAYER_REBUILD
					printf("   B_LAYER_CHILDREN_DEPENDANT Parent\n");
				#endif
				
				// because we're skipping one level, we need to do out
				// parent business as well.
				
				// our visible area is relative to our parent's parent.
				if (fParent->fParent)
					fVisible.IntersectWith(&(fParent->fParent->fVisible));
					
				// exclude parent's visible area which could be composed by
				// prior siblings' visible areas.
				if (fVisible.CountRects() > 0)
					fVisible.Exclude(&(fParent->fVisible));
				
				// we have a final visible area. Include it to our parent's one,
				// exclude from parent's parent.
				if (fVisible.CountRects() > 0) {
					fParent->fFullVisible.Include(&fVisible);
						
					if (fParent->fParent)
						fParent->fParent->fVisible.Exclude(&fVisible);
				}
			} else {
				// for 95+% of cases
				
				#ifdef DEBUG_LAYER_REBUILD
					printf("   (!)B_LAYER_CHILDREN_DEPENDANT Parent\n");
				#endif
				
				// the visible area is the one common with parent's one.
				fVisible.IntersectWith(&(fParent->fVisible));
				
				// exclude from parent's visible area. we're the owners now.
				if (fVisible.CountRects() > 0)
					fParent->fVisible.Exclude(&fVisible);
			}
		}
		fFullVisible = fVisible;
	}
	
	// Rebuild regions for children...
	for(Layer *lay = BottomChild(); lay != NULL; lay = UpperSibling())
		lay->RebuildRegions(reg, newAction, newPt, newOffset);

	#ifdef DEBUG_LAYER_REBUILD
		printf("\nLayer(%s) ALMOST done regions:\n", Name());
		printf("\tVisible Region:\n");
		fVisible.PrintToStream();
		printf("\tFull Visible Region:\n");
		fFullVisible.PrintToStream();
	#endif

	if(!IsHidden()) {
		switch(action) {
			case B_LAYER_NONE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_NONE\n", Name()));
				BRegion r(fVisible);
				if (oldRegion.CountRects() > 0)
					r.Exclude(&oldRegion);
				
				if(r.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&r);
				break;
			}
			case B_LAYER_MOVE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_MOVE\n", Name()));
				BRegion redrawReg;
				BRegion	*copyReg = new BRegion();
				BRegion	screenReg(fRootLayer->Bounds());
				
				oldRegion.OffsetBy(pt.x, pt.y);
				oldRegion.IntersectWith(&fFullVisible);
				
				*copyReg = oldRegion;
				copyReg->IntersectWith(&screenReg);
				if (copyReg->CountRects() > 0 && !(pt.x == 0.0f && pt.y == 0.0f)) {
					copyReg->OffsetBy(-pt.x, -pt.y);
					BPoint		*point = new BPoint(pt);
					fRootLayer->fCopyRegList.AddItem(copyReg);
					fRootLayer->fCopyList.AddItem(point);
				} else {
					delete copyReg;
				}
				
				redrawReg	= fFullVisible;
				redrawReg.Exclude(&oldRegion);
				if (redrawReg.CountRects() > 0 && !(pt.x == 0.0f && pt.y == 0.0f)) {
					fRootLayer->fRedrawReg.Include(&redrawReg);
				}
	
				break;
			}
			case B_LAYER_RESIZE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_RESIZE\n", Name()));
				BRegion redrawReg;
				
				redrawReg = fVisible;
				redrawReg.Exclude(&oldRegion);
				if(redrawReg.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&redrawReg);
	
				break;
			}
			case B_LAYER_MASK_RESIZE: {
				RBTRACE(("2) Layer(%s): Action B_LAYER_MASK_RESIZE\n", Name()));
				BRegion redrawReg;
				BRegion	*copyReg = new BRegion();
				
				oldRegion.OffsetBy(dummyNewLocation.x, dummyNewLocation.y);
				
				redrawReg	= fVisible;
				redrawReg.Exclude(&oldRegion);
				if (redrawReg.CountRects() > 0)
					fRootLayer->fRedrawReg.Include(&redrawReg);
				
				*copyReg = fVisible;
				copyReg->IntersectWith(&oldRegion);
				copyReg->OffsetBy(-dummyNewLocation.x, -dummyNewLocation.y);
				if (copyReg->CountRects() > 0
					&& !(dummyNewLocation.x == 0.0f && dummyNewLocation.y == 0.0f)) {
					fRootLayer->fCopyRegList.AddItem(copyReg);
					fRootLayer->fCopyList.AddItem(new BPoint(dummyNewLocation));
				}
				
				break;
			}
			default:
				RBTRACE(("2) Layer(%s): Action default\n", Name()));
				break;
		}
	}
/*	if (IsHidden()) {
		fFullVisible.MakeEmpty();
		fVisible.MakeEmpty();
	}
*/

	STRACE(("Layer(%s)::RebuildRegions() END\n", Name()));
}

// ResizeOthers
uint32
Layer::ResizeOthers(float x, float y, BPoint coords[], BPoint *ptOffset)
{
	STRACE(("Layer(%s)::ResizeOthers() START\n", Name()));
	uint32 rmask = fResizeMode;
	
	// offset
	coords[0].x	= 0.0f;
	coords[0].y	= 0.0f;
	
	// resize by width/height
	coords[1].x	= 0.0f;
	coords[1].y	= 0.0f;

	if ((rmask & 0x00000f00UL)>>8 == _VIEW_LEFT_ &&
		(rmask & 0x0000000fUL)>>0 == _VIEW_RIGHT_) {
		coords[1].x		= x;
	} else if ((rmask & 0x00000f00UL)>>8 == _VIEW_LEFT_) {
	} else if ((rmask & 0x0000000fUL)>>0 == _VIEW_RIGHT_) {
		coords[0].x		= x;
	} else if ((rmask & 0x00000f00UL)>>8 == _VIEW_CENTER_) {
		coords[0].x		= x/2;
	} else {
		// illegal flag. Do nothing.
	}


	if ((rmask & 0x0000f000UL)>>12 == _VIEW_TOP_ &&
		(rmask & 0x000000f0UL)>>4 == _VIEW_BOTTOM_) {
		coords[1].y		= y;
	} else if ((rmask & 0x0000f000UL)>>12 == _VIEW_TOP_) {
	} else if ((rmask & 0x000000f0UL)>>4 == _VIEW_BOTTOM_) {
		coords[0].y		= y;
	} else if ((rmask & 0x0000f000UL)>>12 == _VIEW_CENTER_) {
		coords[0].y		= y/2;
	} else {
		// illegal flag. Do nothing.
	}

	STRACE(("Layer(%s)::ResizeOthers() END\n", Name()));
	return 0UL;
}

#endif

// Redraw
void
Layer::Redraw(const BRegion& reg, Layer *startFrom)
{
	STRACE(("Layer(%s)::Redraw();\n", Name()));
	if (IsHidden())
		// this layer has nothing visible on screen, so bail out.
		return;

	BRegion *pReg = const_cast<BRegion*>(&reg);

	if (pReg->CountRects() > 0)
		RequestDraw(reg, startFrom);
	
	STRACE(("Layer(%s)::Redraw() ENDED\n", Name()));
}

// Draw
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

// EmptyGlobals
void
Layer::EmptyGlobals()
{
	fRootLayer->fRedrawReg.MakeEmpty();

	int32 count = fRootLayer->fCopyRegList.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BRegion*)fRootLayer->fCopyRegList.ItemAt(i);
	fRootLayer->fCopyRegList.MakeEmpty();
	
	count = fRootLayer->fCopyList.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (BPoint*)fRootLayer->fCopyList.ItemAt(i);
	fRootLayer->fCopyList.MakeEmpty();
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

// NOTE: I added this here and it solves the invalid region problem
// for Windows that have been resized before they were shown. -Stephan
#ifndef NEW_CLIPPING
RebuildFullRegion();
SendViewCoordUpdateMsg();

	if (invalidate)
		GetRootLayer()->GoInvalidate(this, fFull);
#else

#endif
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
#ifndef NEW_CLIPPING	
	if (invalidate)
		GetRootLayer()->GoInvalidate(this, fFullVisible);
#endif
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
	LayerData *data = new LayerData(*fLayerData);
	data->prevState = fLayerData;
	fLayerData = data;
}


void
Layer::PopState()
{
	if (fLayerData->prevState == NULL) {
		fprintf(stderr, "WARNING: User called BView(%s)::PopState(), but there is NO state on stack!\n", Name());
		return;
	}
	
	LayerData *data = fLayerData;
	fLayerData = fLayerData->prevState;
	data->prevState = NULL;
	delete data;
}


//! Matches the BView call of the same name
BRect
Layer::Bounds(void) const
{
	BRect r(fFrame);
//	r.OffsetTo(fBoundsLeftTop);
	r.OffsetTo(BoundsOrigin());
	return r;
}

//! Matches the BView call of the same name
BRect
Layer::Frame(void) const
{
	return fFrame;
}

//! Moves the layer by specified values, complete with redraw
void
Layer::MoveBy(float x, float y)
{
	STRACE(("Layer(%s)::MoveBy() START\n", Name()));
	if (!fParent) {
		CRITICAL("ERROR: in Layer::MoveBy()! - No parent!\n");
		return;
	}

	BPrivate::PortLink msg(-1, -1);
	msg.StartMessage(AS_ROOTLAYER_LAYER_MOVE);
	msg.Attach<Layer*>(this);
	msg.Attach<float>(x);
	msg.Attach<float>(y);
	GetRootLayer()->EnqueueMessage(msg);

	STRACE(("Layer(%s)::MoveBy() END\n", Name()));
}

//! Resize the layer by the specified amount, complete with redraw
void
Layer::ResizeBy(float x, float y)
{
	STRACE(("Layer(%s)::ResizeBy() START\n", Name()));

	if (!fParent) {
		printf("ERROR: in Layer::ResizeBy()! - No parent!\n");
		return;
	}

	BPrivate::PortLink msg(-1, -1);
	msg.StartMessage(AS_ROOTLAYER_LAYER_RESIZE);
	msg.Attach<Layer*>(this);
	msg.Attach<float>(x);
	msg.Attach<float>(y);
	GetRootLayer()->EnqueueMessage(msg);

	STRACE(("Layer(%s)::ResizeBy() END\n", Name()));
}

// BoundsOrigin
BPoint
Layer::BoundsOrigin() const
{
	BPoint origin(0,0);
	float scale = Scale();

	LayerData *ld = fLayerData;
	do {
		origin += ld->Origin();
	} while ((ld = ld->prevState));

	origin.x *= scale;
	origin.y *= scale;

	return origin;
}

float
Layer::Scale() const
{
	float scale = 1.0f;

	LayerData *ld = fLayerData;
	do {
		scale *= ld->Scale();
	} while ((ld = ld->prevState));

	return scale;
}

//! Converts the passed point to parent coordinates
BPoint
Layer::ConvertToParent(BPoint pt)
{
	pt -= BoundsOrigin();
	pt += fFrame.LeftTop();
	return pt;
}

//! Converts the passed rectangle to parent coordinates
BRect
Layer::ConvertToParent(BRect rect)
{
//	rect.OffsetBy(fFrame.LeftTop());
//	return rect;
	rect.OffsetBy(-BoundsOrigin().x, -BoundsOrigin().y);
	rect.OffsetBy(fFrame.LeftTop());
	return rect;
}

//! Converts the passed region to parent coordinates
BRegion
Layer::ConvertToParent(BRegion* reg)
{
	// TODO: wouldn't it be more efficient to use the copy
	// constructor for BRegion and then call OffsetBy()?
	BRegion newreg;
	for (int32 i = 0; i < reg->CountRects(); i++)
		newreg.Include(ConvertToParent(reg->RectAt(i)));
	return newreg;
}

//! Converts the passed point from parent coordinates
BPoint
Layer::ConvertFromParent(BPoint pt)
{
//	return pt - fFrame.LeftTop();
	pt -= fFrame.LeftTop();
	pt += BoundsOrigin();
	return pt;
}

//! Converts the passed rectangle from parent coordinates
BRect
Layer::ConvertFromParent(BRect rect)
{
//	rect.OffsetBy(-fFrame.left, -fFrame.top);
//	return rect;
	rect.OffsetBy(-fFrame.left, -fFrame.top);
	rect.OffsetBy(BoundsOrigin());
	return rect;
}

//! Converts the passed region from parent coordinates
BRegion
Layer::ConvertFromParent(BRegion *reg)
{
	BRegion newreg;
	for(int32 i=0; i<reg->CountRects();i++)
		newreg.Include(ConvertFromParent(reg->RectAt(i)));
	return newreg;
}

// ConvertToTop
BPoint
Layer::ConvertToTop(BPoint pt)
{
	if (fParent) {
//		return (fParent->ConvertToTop(pt + fFrame.LeftTop()));
		pt = ConvertToParent(pt);
		return fParent->ConvertToTop(pt);
	} else
		return pt;
}

//! Converts the passed rectangle to screen coordinates
BRect
Layer::ConvertToTop(BRect rect)
{
	if (fParent) {
//		return fParent->ConvertToTop(rect.OffsetByCopy(fFrame.LeftTop()));
		rect = ConvertToParent(rect);
		return fParent->ConvertToTop(rect);
	} else
		return rect;
}

//! Converts the passed region to screen coordinates
BRegion
Layer::ConvertToTop(BRegion *reg)
{
	BRegion newreg;
	for (int32 i = 0; i < reg->CountRects();i++)
		newreg.Include(ConvertToTop(reg->RectAt(i)));
	return newreg;
}

// ConvertFromTop
BPoint
Layer::ConvertFromTop(BPoint pt)
{
	if (fParent) {
//		return fParent->ConvertFromTop(pt-fFrame.LeftTop());
		pt = ConvertFromParent(pt);
		return fParent->ConvertFromTop(pt);
	} else
		return pt;
}

//! Converts the passed rectangle from screen coordinates
BRect
Layer::ConvertFromTop(BRect rect)
{
	if (fParent) {
//		return fParent->ConvertFromTop(rect.OffsetByCopy(-fFrame.LeftTop().x,
//														 -fFrame.LeftTop().y));
		rect = ConvertFromParent(rect);
		return fParent->ConvertFromTop(rect);
	} else
		return rect;
}

//! Converts the passed region from screen coordinates
BRegion
Layer::ConvertFromTop(BRegion *reg)
{
	BRegion newreg;
	
	for (int32 i = 0; i < reg->CountRects(); i++)
		newreg.Include(ConvertFromTop(reg->RectAt(i)));
	
	return newreg;
}

//! Recursively deletes all children of the calling layer
void
Layer::PruneTree(void)
{
	Layer* lay;
	Layer* nextlay;
	
	lay = fTopChild;
	fTopChild = NULL;
	
	while (lay != NULL) {
		if (lay->fTopChild != NULL)
			lay->PruneTree();
		
		nextlay = lay->fLowerSibling;
		lay->fLowerSibling = NULL;
		
		delete lay;
		lay = nextlay;
	}
	// Man, this thing is short. Elegant, ain't it? :P
}

//! Prints information about the layer's current state
void
Layer::PrintToStream()
{
	printf("\n *** Layer %s:\n", Name());
	printf("\t Parent: %s", fParent ? fParent->Name() : "<no parent>");

	printf("\t us: %s\t ls: %s\n",
		fUpperSibling ? fUpperSibling->Name() : "<none>",
		fLowerSibling ? fLowerSibling->Name() : "<none>");

	printf("\t topChild: %s\t bottomChild: %s\n",
		fTopChild ? fTopChild->Name() : "<none>",
		fBottomChild ? fBottomChild->Name() : "<none>");
	
	printf("Frame: (%f, %f, %f, %f)\n", fFrame.left, fFrame.top, fFrame.right, fFrame.bottom);
	printf("LocalOrigin: (%f, %f)\n", BoundsOrigin().x, BoundsOrigin().y);
	printf("Token: %ld\n", fViewToken);
	printf("Hidden - direct: %s ", fHidden?"true":"false");
	printf("Hidden - indirect: %s\n", IsHidden()?"true":"false");
	printf("ResizingMode: %lx ", fResizeMode);
	printf("Flags: %lx\n", fFlags);

	if (fLayerData)
		fLayerData->PrintToStream();
	else
		printf(" NO LayerData valid pointer\n");
}

//! Prints pointer info kept by the current layer
void
Layer::PrintNode()
{
	printf("-----------\nLayer %s\n", Name());
	if (fParent)
		printf("Parent: %s (%p)\n", fParent->Name(), fParent);
	else
		printf("Parent: NULL\n");

	if (fUpperSibling)
		printf("Upper sibling: %s (%p)\n", fUpperSibling->Name(), fUpperSibling);
	else
		printf("Upper sibling: NULL\n");

	if (fLowerSibling)
		printf("Lower sibling: %s (%p)\n", fLowerSibling->Name(), fLowerSibling);
	else
		printf("Lower sibling: NULL\n");

	if (fTopChild)
		printf("Top child: %s (%p)\n", fTopChild->Name(), fTopChild);
	else
		printf("Top child: NULL\n");

	if (fBottomChild)
		printf("Bottom child: %s (%p)\n", fBottomChild->Name(), fBottomChild);
	else
		printf("Bottom child: NULL\n");
#ifndef NEW_CLIPPING
	printf("Visible Areas: "); fVisible.PrintToStream();
#endif
}

//! Prints the tree hierarchy from the current layer down
void
Layer::PrintTree()
{
	printf("\n Tree structure:\n");
	printf("\t%s\t%s\n", Name(), IsHidden()? "Hidden": "NOT hidden");
	for(Layer *lay = BottomChild(); lay != NULL; lay = UpperSibling())
		printf("\t%s\t%s\n", lay->Name(), lay->IsHidden()? "Hidden": "NOT hidden");
}

// RequestDraw
void
Layer::RequestDraw(const BRegion &reg, Layer *startFrom)
{
	STRACE(("Layer(%s)::RequestDraw()\n", Name()));

	// do not redraw any child until you must
	int redraw = false;
	if (!startFrom)
		redraw = true;

#ifndef NEW_CLIPPING
	if (HasClient() && IsTopLayer()) {
		// calculate the minimum region/rectangle to be updated with
		// a single message to the client.
		BRegion	updateReg(fFullVisible);

		if (fFlags & B_FULL_UPDATE_ON_RESIZE
			&& fFrameAction	== B_LAYER_ACTION_RESIZE)
		{
			// do nothing
		} else {
			updateReg.IntersectWith(&reg);
		}
		if (updateReg.CountRects() > 0) {
			fOwner->fCumulativeRegion.Include(&updateReg);			
			if (!fOwner->InUpdate() && !fOwner->fRequestSent) {
				fOwner->fInUpdateRegion = fOwner->fCumulativeRegion;
fOwner->cnt++;
if (fOwner->cnt != 1)
	CRITICAL("Layer::RequestDraw(): fOwner->cnt != 1 -> Not Allowed!");
				fOwner->fCumulativeRegion.MakeEmpty();
				fOwner->fRequestSent = true;
				SendUpdateMsg(fOwner->fInUpdateRegion);
			}
		}
	}

	if (fVisible.CountRects() > 0) {
		BRegion	updateReg(fVisible);
		// calculate the update region
		if (fFlags & B_FULL_UPDATE_ON_RESIZE && fFrameAction == B_LAYER_ACTION_RESIZE) {
			// do nothing
		} else {
			updateReg.IntersectWith(&reg);
		}

		if (updateReg.CountRects() > 0) {
			fDriver->ConstrainClippingRegion(&updateReg);
			Draw(updateReg.Frame());
			fDriver->ConstrainClippingRegion(NULL);
		}
	}

	for (Layer *lay = BottomChild(); lay != NULL; lay = UpperSibling()) {
		if (lay == startFrom)
			redraw = true;

		if (redraw && !(lay->IsHidden())) {
			// no need to go deeper if not even the FullVisible region intersects
			// Update one.
			BRegion common(lay->fFullVisible);
			common.IntersectWith(&reg);
			
			if (common.CountRects() > 0)
				lay->RequestDraw(reg, NULL);
		}
	}
#else
	if (HasClient() && IsTopLayer()) {
		// calculate the minimum region/rectangle to be updated with
		// a single message to the client.
		BRegion	updateReg(fFullVisible2);

		updateReg.IntersectWith(&reg);

		if (updateReg.CountRects() > 0) {
			fOwner->fCumulativeRegion.Include(&updateReg);			
			if (!fOwner->InUpdate() && !fOwner->fRequestSent) {
				fOwner->fInUpdateRegion = fOwner->fCumulativeRegion;
fOwner->cnt++;
if (fOwner->cnt != 1)
	CRITICAL("Layer::RequestDraw(): fOwner->cnt != 1 -> Not Allowed!");
				fOwner->fCumulativeRegion.MakeEmpty();
				fOwner->fRequestSent = true;
				SendUpdateMsg(fOwner->fInUpdateRegion);
			}
		}
	}

	if (fVisible2.CountRects() > 0) {
		BRegion	updateReg(fVisible2);
			updateReg.IntersectWith(&reg);

		if (updateReg.CountRects() > 0) {
			fDriver->ConstrainClippingRegion(&updateReg);
			Draw(updateReg.Frame());
			fDriver->ConstrainClippingRegion(NULL);
		}
	}

	for (Layer *lay = BottomChild(); lay != NULL; lay = UpperSibling()) {
		if (lay == startFrom)
			redraw = true;

		if (redraw && !(lay->IsHidden())) {
			// no need to go deeper if not even the FullVisible region intersects
			// Update one.
			BRegion common(lay->fFullVisible2);
			common.IntersectWith(&reg);
			
			if (common.CountRects() > 0)
				lay->RequestDraw(reg, NULL);
		}
	}
#endif
}

#ifndef NEW_CLIPPING

// move_layer
void
Layer::move_layer(float x, float y)
{
/*	if (fClassID == AS_WINBORDER_CLASS) {
		WinBorder	*wb = (WinBorder*)this;
		wb->fCumulativeRegion.OffsetBy(x, y);
		wb->fInUpdateRegion.OffsetBy(x, y);
		wb->fSavedForUpdateRegion.OffsetBy(x, y);
	}*/
	
	fFrameAction = B_LAYER_ACTION_MOVE;

	BPoint pt(x, y);	
	BRect rect(fFull.Frame().OffsetByCopy(pt));

if (!fParent) {
printf("no parent in Layer::move_layer() (%s)\n", Name());
fFrameAction = B_LAYER_ACTION_NONE;
return;
}

	fParent->StartRebuildRegions(BRegion(rect), this, B_LAYER_MOVE, pt);

	fDriver->CopyRegionList(&fRootLayer->fCopyRegList,
							&fRootLayer->fCopyList,
							fRootLayer->fCopyRegList.CountItems(),
							&fFullVisible);

	fParent->Redraw(fRootLayer->fRedrawReg, this);

	SendViewCoordUpdateMsg();
	
	EmptyGlobals();

	fFrameAction = B_LAYER_ACTION_NONE;
}

// resize_layer
void
Layer::resize_layer(float x, float y)
{
	fFrameAction = B_LAYER_ACTION_RESIZE;

	BPoint pt(x,y);	

	BRect rect(fFull.Frame());
	rect.right += x;
	rect.bottom += y;

if (!fParent) {
printf("no parent in Layer::resize_layer() (%s)\n", Name());
fFrameAction = B_LAYER_ACTION_NONE;
return;
}

	fParent->StartRebuildRegions(BRegion(rect), this, B_LAYER_RESIZE, pt);

	fDriver->CopyRegionList(&fRootLayer->fCopyRegList, &fRootLayer->fCopyList, fRootLayer->fCopyRegList.CountItems(), &fFullVisible);


	fParent->Redraw(fRootLayer->fRedrawReg, this);

	SendViewCoordUpdateMsg();
	
	EmptyGlobals();

	fFrameAction = B_LAYER_ACTION_NONE;
}

// FullInvalidate
void
Layer::FullInvalidate(const BRect &rect)
{
	FullInvalidate(BRegion(rect));
}

// FullInvalidate
void
Layer::FullInvalidate(const BRegion& region)
{
	STRACE(("Layer(%s)::FullInvalidate():\n", Name()));
	
#ifdef DEBUG_LAYER
	region.PrintToStream();
	printf("\n");
#endif

	BPoint pt(0,0);
	StartRebuildRegions(region, NULL,/* B_LAYER_INVALIDATE, pt); */B_LAYER_NONE, pt);

	Redraw(fRootLayer->fRedrawReg);
	
	EmptyGlobals();
}

// Invalidate
void
Layer::Invalidate(const BRegion& region)
{
	STRACE(("Layer(%s)::Invalidate():\n", Name()));
#ifdef DEBUG_LAYER
	region.PrintToStream();
	printf("\n");
#endif
	
	fRootLayer->fRedrawReg	= region;
	
	Redraw(fRootLayer->fRedrawReg);
	
	EmptyGlobals();
}

#endif // 5 methods


/*!
	\brief Returns the layer's ServerWindow
	
	If the layer's ServerWindow has not been assigned, it attempts to find 
	the owning ServerWindow in the tree.
*/
ServerWindow*
Layer::SearchForServerWindow()
{
	if (!fServerWin)
		fServerWin=fParent->SearchForServerWindow();
	
	return fServerWin;
}

//! Sends an _UPDATE_ message to the client BWindow
void
Layer::SendUpdateMsg(BRegion& reg)
{
	BMessage msg;
	msg.what = _UPDATE_;
#ifndef NEW_CLIPPING
	msg.AddRect("_rect", ConvertFromTop(reg.Frame()));
#else
	BRect	rect(reg.Frame());
	ConvertFromScreen2(&rect);
	msg.AddRect("_rect", rect );
#endif
	msg.AddRect("debug_rect", reg.Frame());
//	msg.AddInt32("_token",fViewToken);
		
	fOwner->Window()->SendMessageToClient(&msg);
}

// AddToViewsWithInvalidCoords
void
Layer::AddToViewsWithInvalidCoords() const
{
	if (fServerWin) {
		fServerWin->ClientViewsWithInvalidCoords().AddInt32("_token", fViewToken);
		fServerWin->ClientViewsWithInvalidCoords().AddPoint("where", fFrame.LeftTop());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("width", fFrame.Width());
		fServerWin->ClientViewsWithInvalidCoords().AddFloat("height", fFrame.Height());
	}
}

// SendViewCoordUpdateMsg
void
Layer::SendViewCoordUpdateMsg() const
{
	if (fServerWin && !fServerWin->ClientViewsWithInvalidCoords().IsEmpty()) {
		fServerWin->SendMessageToClient(&fServerWin->ClientViewsWithInvalidCoords());
		fServerWin->ClientViewsWithInvalidCoords().MakeEmpty();
	}
}

// SetViewColor
void
Layer::SetViewColor(const RGBColor& color)
{
	fViewColor = color;
}

// SetBackgroundBitmap
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

#ifdef NEW_CLIPPING

//! converts a point from local to parent's coordinate system 
void
Layer::ConvertToParent2(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		pt->x -= origin.x;
		pt->y -= origin.y;
		pt->x += fFrame.left;
		pt->y += fFrame.top;
	}
}

//! converts a rect from local to parent's coordinate system 
void
Layer::ConvertToParent2(BRect* rect) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		rect->OffsetBy(-origin.x, -origin.y);
		rect->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a region from local to parent's coordinate system 
void
Layer::ConvertToParent2(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		reg->OffsetBy(-origin.x, -origin.y);
		reg->OffsetBy(fFrame.left, fFrame.top);
	}
}

//! converts a point from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BPoint* pt) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		pt->x += origin.x;
		pt->y += origin.y;
		pt->x -= fFrame.left;
		pt->y -= fFrame.top;
	}
}

//! converts a rect from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BRect* rect) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		rect->OffsetBy(origin.x, origin.y);
		rect->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a region from parent's to local coordinate system 
void
Layer::ConvertFromParent2(BRegion* reg) const
{
	if (fParent) {
		BPoint origin = BoundsOrigin();
		reg->OffsetBy(origin.x, origin.y);
		reg->OffsetBy(-fFrame.left, -fFrame.top);
	}
}

//! converts a point from local to screen coordinate system 
void
Layer::ConvertToScreen2(BPoint* pt) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			pt->x -= origin.x;
			pt->y -= origin.y;
			pt->x += fFrame.left;
			pt->y += fFrame.top;

			fParent->ConvertToScreen2(pt);
		}
}

//! converts a rect from local to screen coordinate system 
void
Layer::ConvertToScreen2(BRect* rect) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			rect->OffsetBy(-origin.x, -origin.y);
			rect->OffsetBy(fFrame.left, fFrame.top);

			fParent->ConvertToScreen2(rect);
		}
}

//! converts a region from local to screen coordinate system 
void
Layer::ConvertToScreen2(BRegion* reg) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			reg->OffsetBy(-origin.x, -origin.y);
			reg->OffsetBy(fFrame.left, fFrame.top);

			fParent->ConvertToScreen2(reg);
		}
}

//! converts a point from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BPoint* pt) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			pt->x += origin.x;
			pt->y += origin.y;
			pt->x -= fFrame.left;
			pt->y -= fFrame.top;

			fParent->ConvertToScreen2(pt);
		}
}

//! converts a rect from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BRect* rect) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			rect->OffsetBy(origin.x, origin.y);
			rect->OffsetBy(-fFrame.left, -fFrame.top);

			fParent->ConvertFromScreen2(rect);
		}
}

//! converts a region from screen to local coordinate system 
void
Layer::ConvertFromScreen2(BRegion* reg) const
{
	if (GetRootLayer())
		if (fParent) {
			BPoint origin = BoundsOrigin();
			reg->OffsetBy(origin.x, origin.y);
			reg->OffsetBy(-fFrame.left, -fFrame.top);

			fParent->ConvertFromScreen2(reg);
		}
}


void
Layer::do_Hide()
{
	fHidden = true;

	if (fParent && !fParent->IsHidden() && GetRootLayer()) {
		// save fullVisible so we know what to invalidate
		BRegion invalid(fFullVisible2);

		clear_visible_regions();

		if (invalid.Frame().IsValid())
			fParent->do_Invalidate(invalid, this);
	}
}

void
Layer::do_Show()
{
	fHidden = false;

	if (fParent && !fParent->IsHidden() && GetRootLayer()) {
		BRegion invalid;

		get_user_regions(invalid);

		if (invalid.CountRects() > 0)
			fParent->do_Invalidate(invalid, this);
	}
}

void
Layer::do_Invalidate(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);
	rebuild_visible_regions(invalid, localVisible,
		startFrom? startFrom: BottomChild());

	// add localVisible to our RootLayer's redraw region.
	GetRootLayer()->fRedrawReg.Include(&localVisible);
// TODO: ---
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, BottomChild());
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
}

void
Layer::do_Redraw(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);

	// add localVisible to our RootLayer's redraw region.
	GetRootLayer()->fRedrawReg.Include(&localVisible);
// TODO: ---
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, BottomChild());
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
}

inline void
Layer::resize_layer_frame_by(float x, float y)
{
	uint16		rm = fResizeMode & 0x0000FFFF;
	BRect		newFrame = fFrame;

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
		float		dx, dy;

		dx	= newFrame.Width() - fFrame.Width();
		dy	= newFrame.Height() - fFrame.Height();

		fFrame	= newFrame;

		if (dx != 0.0f || dy != 0.0f) {
			// call hook function
			ResizedByHook(dx, dy, true); // automatic

			for (Layer *lay = BottomChild(); lay; lay = UpperSibling())
				lay->resize_layer_frame_by(dx, dy);
		}
		else
			MovedByHook(dx, dy);
	}
}

inline void
Layer::rezize_layer_redraw_more(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer *lay = BottomChild(); lay; lay = UpperSibling()) {
		uint16		rm = lay->fResizeMode & 0x0000FFFF;

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			// NOTE: this is not exactly corect, but it works :-)
			// Normaly we shoud've used the lay's old, required region - the one returned
			// from get_user_region() with the old frame, and the current one. lay->Bounds()
			// works for the moment so we leave it like this.

			// calculate the old bounds.
			BRect	oldBounds(lay->Bounds());		
			if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT)
				oldBounds.right -=dx;
			if ((rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM)
				oldBounds.bottom -=dy;
			
			// compute the region that became visible because we got bigger OR smaller.
			BRegion	regZ(lay->Bounds());
			regZ.Include(oldBounds);
			regZ.Exclude(oldBounds&lay->Bounds());

			lay->ConvertToScreen2(&regZ);

			// intersect that with this'(not lay's) fullVisible region
			regZ.IntersectWith(&fFullVisible2);
			reg.Include(&regZ);

			lay->rezize_layer_redraw_more(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);

			// above, OR this:
			// reg.Include(&lay->fFullVisible2);
		}
		else
		if (((rm & 0x0F0F) == (uint16)B_FOLLOW_RIGHT && dx != 0) ||
			((rm & 0x0F0F) == (uint16)B_FOLLOW_H_CENTER && dx != 0) ||
			((rm & 0xF0F0) == (uint16)B_FOLLOW_BOTTOM && dy != 0)||
			((rm & 0xF0F0) == (uint16)B_FOLLOW_V_CENTER && dy != 0))
		{
			reg.Include(&lay->fFullVisible2);
		}
	}
}

inline void
Layer::resize_layer_full_update_on_resize(BRegion &reg, float dx, float dy)
{
	if (dx == 0 && dy == 0)
		return;

	for (Layer *lay = BottomChild(); lay; lay = UpperSibling()) {
		uint16		rm = lay->fResizeMode & 0x0000FFFF;		

		if ((rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT || (rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM) {
			if (lay->fFlags & B_FULL_UPDATE_ON_RESIZE && lay->fVisible2.CountRects() > 0)
				reg.Include(&lay->fVisible2);

			lay->resize_layer_full_update_on_resize(reg,
				(rm & 0x0F0F) == (uint16)B_FOLLOW_LEFT_RIGHT? dx: 0,
				(rm & 0xF0F0) == (uint16)B_FOLLOW_TOP_BOTTOM? dy: 0);
		}
	}
}

void
Layer::do_ResizeBy(float dx, float dy)
{
	fFrame.Set(fFrame.left, fFrame.top, fFrame.right+dx, fFrame.bottom+dy);

	// resize children using their resize_mask.
	for (Layer *lay = BottomChild(); lay; lay = UpperSibling())
			lay->resize_layer_frame_by(dx, dy);

	// call hook function
	if (dx != 0.0f || dy != 0.0f)
		ResizedByHook(dx, dy, false); // manual

	if (!IsHidden() && GetRootLayer()) {
		BRegion oldFullVisible(fFullVisible2);
		// this is required to invalidate the old border
		BRegion oldVisible(fVisible2);

		// in case they moved, bottom, right and center aligned layers must be redrawn
		BRegion redrawMore;
		rezize_layer_redraw_more(redrawMore, dx, dy);

		// we'll invalidate the old area and the new, maxmial one.
		BRegion invalid;
		get_user_regions(invalid);
		invalid.Include(&fFullVisible2);

		clear_visible_regions();

		fParent->do_RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now redraw regions that became visible

		// what's invalid, are the differences between to old and the new fullVisible region
		// 1) in case we grow.
		BRegion		redrawReg(fFullVisible2);
		redrawReg.Exclude(&oldFullVisible);
		// 2) in case we shrink
		BRegion		redrawReg2(oldFullVisible);
		redrawReg2.Exclude(&fFullVisible2);
		// 3) combine.
		redrawReg.Include(&redrawReg2);

		// for center, right and bottom alligned layers, redraw their old positions
		redrawReg.Include(&redrawMore);

		// layers that had their frame modified must be entirely redrawn.
		rezize_layer_redraw_more(redrawReg, dx, dy);

		// add redrawReg to our RootLayer's redraw region.
		GetRootLayer()->fRedrawReg.Include(&redrawReg);
		// include layer's visible region in case we want a full update on resize
		if (fFlags & B_FULL_UPDATE_ON_RESIZE && fVisible2.Frame().IsValid()) {
			resize_layer_full_update_on_resize(GetRootLayer()->fRedrawReg, dx, dy);

			GetRootLayer()->fRedrawReg.Include(&fVisible2);
			GetRootLayer()->fRedrawReg.Include(&oldVisible);
		}
		// clear canvas and set invalid regions for affected WinBorders
// TODO: ---
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, BottomChild());
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}
}

void Layer::do_MoveBy(float dx, float dy)
{
	if (dx == 0.0f && dy == 0.0f)
		return;

//	fFrame.Set(fFrame.left+dx, fFrame.top+dy, fFrame.right+dx, fFrame.bottom+dy);
	fFrame.OffsetBy(dx, dy);

	// call hook function
	MovedByHook(dx, dy);

	if (!IsHidden() && GetRootLayer()) {
		BRegion oldFullVisible(fFullVisible2);

		// we'll invalidate the old position and the new, maxmial one.
		BRegion invalid;
		get_user_regions(invalid);
		invalid.Include(&fFullVisible2);

		clear_visible_regions();

		fParent->do_RebuildVisibleRegions(invalid, this);

		// done rebuilding regions, now copy common parts and redraw regions that became visible

		// include the actual and the old fullVisible regions. later, we'll exclude the common parts.
		BRegion		redrawReg(fFullVisible2);
		redrawReg.Include(&oldFullVisible);

		// offset to layer's new location so that we can calculate the common region.
		oldFullVisible.OffsetBy(dx, dy);

		// finally we have the region that needs to be redrawn.
		redrawReg.Exclude(&oldFullVisible);

		// by intersecting the old fullVisible offseted to layer's new location, with the current
		// fullVisible, we'll have the common region which can be copied using HW acceleration.
		oldFullVisible.IntersectWith(&fFullVisible2);

		// offset back and instruct the HW to do the actual copying.
		oldFullVisible.OffsetBy(-dx, -dy);
// TODO: uncomment!!!
//		GetRootLayer()->CopyRegion(&oldFullVisible, dx, dy);

		// add redrawReg to our RootLayer's redraw region.
		GetRootLayer()->fRedrawReg.Include(&redrawReg);
// TODO: ---
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, BottomChild());
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}
}

void
Layer::do_ScrollBy(float dx, float dy)
{
	fLayerData->OffsetOrigin(BPoint(dx, dy));
//	fOrigin.Set(fOrigin.x + dx, fOrigin.y + dy);

	if (!IsHidden() && GetRootLayer()) {
		// set the region to be invalidated.
		BRegion		invalid(fFullVisible2);

		clear_visible_regions();

		rebuild_visible_regions(invalid, invalid, BottomChild());

		// for the moment we say that the whole surface needs to be redraw.
		BRegion		redrawReg(fFullVisible2);

		// offset old region so that we can start comparing.
		invalid.OffsetBy(dx, dy);

		// compute the common region. we'll use HW acc to copy this to the new location.
		invalid.IntersectWith(&fFullVisible2);
// TODO: uncomment!!!
//		GetRootLayer()->CopyRegion(&invalid, -dx, -dy);

		// common region goes back to its original location. then, by excluding
		// it from curent fullVisible we'll obtain the region that needs to be redrawn.
		invalid.OffsetBy(-dx, -dy);
		redrawReg.Exclude(&invalid);

		GetRootLayer()->fRedrawReg.Include(&redrawReg);
// TODO: ---
	GetRootLayer()->RequestDraw(GetRootLayer()->fRedrawReg, BottomChild());
//	GetRootLayer()->RequestRedraw(); // TODO: what if we pass (fParent, startFromTHIS, &redrawReg)?
	}

	if (dx != 0.0f || dy != 0.0f)
		ScrolledByHook(dx, dy);
}

void
Layer::get_user_regions(BRegion &reg)
{
	// 1) set to frame in screen coords
	BRect screenFrame(Bounds());
	ConvertToScreen2(&screenFrame);
	reg.Set(screenFrame);

	// 2) intersect with screen region
	BRegion screenReg(GetRootLayer()->Bounds());
	reg.IntersectWith(&screenReg);


	// 3) impose user constrained regions
	LayerData *stackData = fLayerData;
	while (stackData) {
		if (stackData->ClippingRegion()) {
			// transform in screen coords
			BRegion screenReg(*stackData->ClippingRegion());
			ConvertToScreen2(&screenReg);
			reg.IntersectWith(&screenReg);
		}
		stackData = stackData->prevState;
	}
}

void
Layer::do_RebuildVisibleRegions(const BRegion &invalid, const Layer *startFrom)
{
	BRegion		localVisible(fFullVisible2);
	localVisible.IntersectWith(&invalid);
	rebuild_visible_regions(invalid, localVisible, startFrom);
}

void
Layer::rebuild_visible_regions(const BRegion &invalid,
								const BRegion &parentLocalVisible,
								const Layer *startFrom)
{
	// no point in continuing if this layer is hidden. starting from here, all
	// descendants have (and will have) invalid visible regions.
	if (fHidden)
		return;

	// no need to go deeper if the parent doesn't have a visible region anymore
	// and our fullVisible region is also empty.
	if (!parentLocalVisible.Frame().IsValid() && !(fFullVisible2.CountRects() > 0))
		return;

	bool fullRebuild = false;

	// intersect maximum wanted region with the invalid region
	BRegion common;
	get_user_regions(common);
	common.IntersectWith(&invalid);

	// if the resulted region is not valid, this layer is not in the catchment area
	// of the region being invalidated
	if (!common.CountRects() > 0)
		return;

	// now intersect with parent's visible part of the region that was/is invalidated
	common.IntersectWith(&parentLocalVisible);

	// exclude the invalid region
	fFullVisible2.Exclude(&invalid);
	fVisible2.Exclude(&invalid);

	// put in what's really visible
	fFullVisible2.Include(&common);

	// this is to allow a layer to hide some parts of itself so children
	// won't take them.
	BRegion unalteredVisible(common);
	bool altered = alter_visible_for_children(common);

	for (Layer *lay = BottomChild(); lay; lay = UpperSibling()) {
		if (lay == startFrom)
			fullRebuild = true;

		if (fullRebuild)
			lay->rebuild_visible_regions(invalid, common, lay->BottomChild());

		// to let children know much they can take from parent's visible region
		common.Exclude(&lay->fFullVisible2);
		// we've hidden some parts of our visible region from our children,
		// and we must be in sysnc with this region too...
		if (altered)
			unalteredVisible.Exclude(&lay->fFullVisible2);
	}

	// the visible region of this layer is what left after all its children took
	// what they could.
	if (altered)
		fVisible2.Include(&unalteredVisible);
	else
		fVisible2.Include(&common);
}

bool
Layer::alter_visible_for_children(BRegion &reg)
{
	// Empty Hook function
	return false;
}

void
Layer::clear_visible_regions()
{
	// OPT: maybe we should uncomment these lines for performance
	//if (fFullVisible2.CountRects() <= 0)
	//	return;

	fVisible2.MakeEmpty();
	fFullVisible2.MakeEmpty();
	for (Layer *child = BottomChild(); child; child = UpperSibling())
		child->clear_visible_regions();
}

#endif

