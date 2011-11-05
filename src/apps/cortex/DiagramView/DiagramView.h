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


// DiagramView.h (Cortex/DiagramView)
//
// * PURPOSE
//   BView and DiagramItemGroup derived class providing the
//   one and only drawing context all child DiagramItems will
//   use.
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramView_H__
#define __DiagramView_H__

#include "DiagramItemGroup.h"

#include <Region.h>
#include <View.h>

//class BBitmap;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

//class DiagramBox;
class DiagramWire;
class DiagramEndPoint;

class DiagramView : public BView,
					public DiagramItemGroup
{

public:				// *** ctor/dtor

						DiagramView(
							BRect frame,
							const char *name,
							bool multiSelection,
							uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW);

	virtual				~DiagramView();

public:					// *** hook functions

	// is called from MessageReceived() if a wire has been dropped
	// on the background or on an incompatible endpoint
	virtual void		connectionAborted(
							DiagramEndPoint *fromWhich)
						{ /* does nothing */ }

	// is called from MessageReceived() if an endpoint has accepted
	// a wire-drop (i.e. connection)
	virtual void		connectionEstablished(
							DiagramEndPoint *fromWhich,
							DiagramEndPoint *toWhich)
						{ /* does nothing */ }

	// must be implemented to return an instance of the DiagramWire
	// derived class
	virtual DiagramWire *createWire(
							DiagramEndPoint *fromWhich,
							DiagramEndPoint *woWhich) = 0;

	// must be implemented to return an instance of the DiagramWire
	// derived class; this version is for temporary wires used in
	// drag & drop connecting
	virtual DiagramWire *createWire(
							DiagramEndPoint *fromWhich) = 0;
					
	// hook called from BackgroundMouseDown() if the background was hit
	virtual void		BackgroundMouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks)
						{ /* does nothing */ }

	// hook called from MouseMoved() if the mouse is floating over
	// the background (i.e. with no message attached)
	virtual void		MouseOver(
							BPoint point,
							uint32 transit)
						{ /* does nothing */ }

	// hook called from MouseMoved() if a message is being dragged
	// over the background
	virtual void		MessageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// hook called from MessageReceived() if a message has been
	// dropped over the background
	virtual void		MessageDropped(
							BPoint point,
							BMessage *message);

public:					// derived from BView

	// initial scrollbar update [e.moon 16nov99]
	virtual void		AttachedToWindow();

	// draw the background and all items
	virtual void		Draw(
							BRect updateRect);

	// updates the scrollbars
	virtual void		FrameResized(
							float width,
							float height);

	// return data rect [c.lenz 1mar2000]
	virtual void		GetPreferredSize(
							float *width,
							float *height);

	// handles the messages M_SELECTION_CHANGED and M_WIRE_DROPPED
	// and passes a dropped message on to the item it was dropped on
	virtual void		MessageReceived(
							BMessage *message);

	// handles the arrow keys for moving DiagramBoxes
	virtual void		KeyDown(
							const char *bytes,
							int32 numBytes);

	// if an item is located at the click point, this function calls
	// that items MouseDown() method; else a deselectAll() command is
	// made and rect-tracking is initiated
	virtual void		MouseDown(
							BPoint point);

	// if an item is located under the given point, this function
	// calls that items MessageDragged() hook if a message is being
	// dragged, and MouseOver() if not
	virtual void		MouseMoved(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// ends rect-tracking and wire-tracking
	virtual void		MouseUp(
							BPoint point);

public:					// *** derived from DiagramItemGroup

	// extends the DiagramItemGroup implementation by setting
	// the items owner and calling the attachedToDiagram() hook
	// on it
	virtual bool		AddItem(
							DiagramItem *item);

	// extends the DiagramItemGroup implementation by calling 
	// the detachedToDiagram() hook on the item
	virtual bool		RemoveItem(
							DiagramItem *item);

public:					// *** operations

	// update the temporary wire to follow the mouse cursor
	void				trackWire(
							BPoint point);

	bool				isWireTracking() const
						{ return m_draggedWire; }

protected:				// *** internal operations

	// do the actual background drawing
	void				drawBackground(
							BRect updateRect);

	// returns the current background color
	rgb_color			backgroundColor() const
						{ return m_backgroundColor; }

	// set the background color; does not refresh the display
	void				setBackgroundColor(
							rgb_color color);
	
	// set the background bitmap; does not refresh the display
	void				setBackgroundBitmap(
							BBitmap *bitmap);

	// updates the region containing the rects of all boxes in
	// the view (and thereby the "data-rect") and then adapts
	// the scrollbars if necessary
	void				updateDataRect();

private:				// *** internal operations

	// setup a temporary wire for "live" dragging and attaches
	// a message to the mouse
	void				_beginWireTracking(
							DiagramEndPoint *fromEndPoint);

	// delete the temporary dragged wire and invalidate display
	void				_endWireTracking();
	
	// setups rect-tracking to additionally drag a message for
	// easier identification in MouseMoved()
	void				_beginRectTracking(
								BPoint origin);

	// takes care of actually selecting/deselecting boxes when
	// they intersect with the tracked rect
	void				_trackRect(
							BPoint origin,
							BPoint current);

	// updates the scrollbars (if there are any) to represent
	// the current data-rect
	void				_updateScrollBars();

private:				// *** data

	// the button pressed at the last mouse event
	int32				m_lastButton;

	// the number of clicks with the last button
	int32				m_clickCount;

	// the point last clicked in this view
	BPoint				m_lastClickPoint;

	// the button currently pressed (reset to 0 on mouse-up)
	// [e.moon 16nov99]
	int32					m_pressedButton;

	// last mouse position in screen coordinates [e.moon 16nov99]
	// only valid while m_pressedButton != 0
	BPoint				m_lastDragPoint;

	// a pointer to the temporary wire used for wire
	// tracking
	DiagramWire		   *m_draggedWire;
	
	// contains the rects of all DiagramBoxes in this view
	BRegion				m_boxRegion;

	// contains the rect of the view actually containing something
	// (i.e. DiagramBoxes) and all free space left/above of that
	BRect				m_dataRect;

	// true if a bitmap is used for the background; false
	// if a color is used
	bool				m_useBackgroundBitmap;

	// the background color of the view
	rgb_color			m_backgroundColor;
	
	// the background bitmap of the view
	BBitmap			   *m_backgroundBitmap;
};

__END_CORTEX_NAMESPACE
#endif // __DiagramView_H__

