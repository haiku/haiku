/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// DiagramBox.cpp

/*! \class DiagramBox
	DiagramItem subclass providing a basic framework for
	boxes in diagrams, i.e. objects that can contain various
	endpoints
*/

#include "DiagramBox.h"
#include "DiagramDefs.h"
#include "DiagramEndPoint.h"
#include "DiagramView.h"

#include <Message.h>
#include <Messenger.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)


//	#pragma mark -


DiagramBox::DiagramBox(BRect frame, uint32 flags)
	: DiagramItem(DiagramItem::M_BOX),
	DiagramItemGroup(DiagramItem::M_ENDPOINT),
	fFrame(frame),
	fFlags(flags)
{
	D_METHOD(("DiagramBox::DiagramBox()\n"));
	makeDraggable(true);
}


DiagramBox::~DiagramBox()
{
	D_METHOD(("DiagramBox::~DiagramBox()\n"));
}


//	#pragma mark -  derived from DiagramItemGroup (public)


/*! Extends the DiagramItemGroup implementation by setting
	the items owner and calling the attachedToDiagram() hook
	on it
*/
bool
DiagramBox::AddItem(DiagramItem *item)
{
	D_METHOD(("DiagramBox::AddItem()\n"));
	if (item) {
		if (DiagramItemGroup::AddItem(item)) {
			if (m_view) {
				item->_SetOwner(m_view);
				item->attachedToDiagram();
			}
			return true;
		}
	}
	return false;
}


/*! Extends the DiagramItemGroup implementation by calling 
	the detachedToDiagram() hook on the \a item
*/
bool
DiagramBox::RemoveItem(DiagramItem *item)
{
	D_METHOD(("DiagramBox::RemoveItem()\n"));
	if (item) {
		item->detachedFromDiagram();
		if (DiagramItemGroup::RemoveItem(item)) {
			item->_SetOwner(0);
			return true;
		}
	}
	return false;
}


//	#pragma mark -   derived from DiagramItem (public)


/*! Prepares the drawing stack and clipping region, then
	calls drawBox
*/
void
DiagramBox::Draw(BRect updateRect)
{
	D_DRAW(("DiagramBox::Draw()\n"));
	if (view()) {
		view()->PushState();
		{
			if (fFlags & M_DRAW_UNDER_ENDPOINTS) {
				BRegion region, clipping;
				region.Include(Frame());
				if (group()->GetClippingAbove(this, &clipping))
					region.Exclude(&clipping);
				view()->ConstrainClippingRegion(&region);
				DrawBox();
				for (uint32 i = 0; i < CountItems(); i++) {
					DiagramItem *item = ItemAt(i);
					if (region.Intersects(item->Frame()))
						item->Draw(item->Frame());
				}
			} else {
				BRegion region, clipping;
				region.Include(Frame());
				if (view()->GetClippingAbove(this, &clipping))
					region.Exclude(&clipping);
				for (uint32 i = 0; i < CountItems(); i++) {
					DiagramItem *item = ItemAt(i);
					BRect r;
					if (region.Intersects(r = item->Frame())) {
						item->Draw(r);
						region.Exclude(r);
					}
				}
				view()->ConstrainClippingRegion(&region);
				DrawBox();
			}
		}
		view()->PopState();
	}
}


/*! Is called from the parent DiagramViews MouseDown() implementation 
	if the Box was hit; this version initiates selection and dragging
*/
void
DiagramBox::MouseDown(BPoint point, uint32 buttons, uint32 clicks)
{
	D_MOUSE(("DiagramBox::MouseDown()\n"));
	DiagramItem *item = ItemUnder(point);
	if (item)
		item->MouseDown(point, buttons, clicks);
	else if (clicks == 1) {
		if (isSelectable()) {
			BMessage selectMsg(M_SELECTION_CHANGED);
			if (modifiers() & B_SHIFT_KEY)
				selectMsg.AddBool("replace", false);
			else
				selectMsg.AddBool("replace", true);
			selectMsg.AddPointer("item", reinterpret_cast<void *>(this));
			DiagramView* v = view();
			BMessenger(v).SendMessage(&selectMsg);
		}
		if (isDraggable() && (buttons == B_PRIMARY_MOUSE_BUTTON)) {
			BMessage dragMsg(M_BOX_DRAGGED);
			dragMsg.AddPointer("item", static_cast<void *>(this));
			dragMsg.AddPoint("offset", point - Frame().LeftTop());
			view()->DragMessage(&dragMsg, BRect(0.0, 0.0, -1.0, -1.0), view());
		}
	}
}



/*!	Is called from the DiagramViews MouseMoved() when no message is being 
	dragged, but the mouse has moved above the box
*/
void
DiagramBox::MouseOver(BPoint point, uint32 transit)
{
	D_MOUSE(("DiagramBox::MouseOver()\n"));
	DiagramItem *last = _LastItemUnder();
	if (last && (transit == B_EXITED_VIEW)) {
		last->MouseOver(point, B_EXITED_VIEW);
		_ResetItemUnder();
	} else {
		DiagramItem *item = ItemUnder(point);
		if (item) {
			if (item != last) {
				if (last)
					last->MouseOver(point, B_EXITED_VIEW);
				item->MouseOver(point, B_ENTERED_VIEW);
			} else
				item->MouseOver(point, B_INSIDE_VIEW);
		}
		else if (last)
			last->MouseOver(point, B_EXITED_VIEW);
	}
}


/*!	Is called from the DiagramViews MouseMoved() when a message is being 
	dragged over the DiagramBox
*/
void
DiagramBox::MessageDragged(BPoint point, uint32 transit, const BMessage *message)
{
	D_MOUSE(("DiagramBox::MessageDragged()\n"));
	DiagramItem *last = _LastItemUnder();
	if (last && (transit == B_EXITED_VIEW)) {
		last->MessageDragged(point, B_EXITED_VIEW, message);
		_ResetItemUnder();
	} else {
		DiagramItem *item = ItemUnder(point);
		if (item) {
			if (item != last) {
				if (last)
					last->MessageDragged(point, B_EXITED_VIEW, message);
				item->MessageDragged(point, B_ENTERED_VIEW, message);
			} else
				item->MessageDragged(point, B_INSIDE_VIEW, message);
			return;
		} else if (last)
			last->MessageDragged(point, B_EXITED_VIEW, message);
		if (message->what == M_WIRE_DRAGGED)
			view()->trackWire(point);
	}
}


/*!	Is called from the DiagramViews MessageReceived() function when a 
	message has been dropped on the DiagramBox
*/
void
DiagramBox::MessageDropped(BPoint point, BMessage *message)
{
	D_METHOD(("DiagramBox::MessageDropped()\n"));
	DiagramItem *item = ItemUnder(point);
	if (item) {
		item->MessageDropped(point, message);
		return;
	}
}


//	#pragma mark - operations (public)


/*!	Moves the box by a given amount, and returns in updateRegion the
	frames of wires impacted by the move
*/
void
DiagramBox::MoveBy(float x, float y, BRegion *wireRegion)
{
	D_METHOD(("DiagramBox::MoveBy()\n"));
	if (view()) {
		view()->PushState();
		{
			for (uint32 i = 0; i < CountItems(); i++) {
				DiagramEndPoint *endPoint = dynamic_cast<DiagramEndPoint *>(ItemAt(i));
				if (endPoint)
					endPoint->MoveBy(x, y, wireRegion);
			}
			if (wireRegion) {
				wireRegion->Include(fFrame);
				fFrame.OffsetBy(x, y);
				wireRegion->Include(fFrame);
			}
			else
				fFrame.OffsetBy(x, y);
		}
		view()->PopState();
	}
}


//! Resizes the boxes frame without doing any updating
void
DiagramBox::ResizeBy(float horizontal, float vertical)
{
	D_METHOD(("DiagramBox::ResizeBy()\n"));
	fFrame.right += horizontal;
	fFrame.bottom += vertical;
}


//	#pragma mark - internal operations (private)


//!	Is called by the DiagramView when added
void
DiagramBox::_SetOwner(DiagramView *owner)
{
	D_METHOD(("DiagramBox::_SetOwner()\n"));
	m_view = owner;
	for (uint32 i = 0; i < CountItems(DiagramItem::M_ENDPOINT); i++) {
		DiagramItem *item = ItemAt(i);
		item->_SetOwner(m_view);
		if (m_view)
			item->attachedToDiagram();
	}
}
