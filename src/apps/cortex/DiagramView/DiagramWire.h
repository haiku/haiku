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


// DiagramWire.h (Cortex/DiagramView)
//
// * PURPOSE
//   DiagramItem subclass serving as a graphical connection
//   between two DiagramEndPoints
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramWire_H__
#define __DiagramWire_H__

#include "DiagramItem.h"

#include <Region.h>
#include <Window.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramView;
class DiagramEndPoint;

class DiagramWire : public DiagramItem
{
	friend class DiagramView;

public:					// *** ctor/dtor

	// both endpoints are set connected by this constructor
	// so be careful to only pass in valid pointers
						DiagramWire(
							DiagramEndPoint *fromWhich,
							DiagramEndPoint *toWhich);

	// special constructor used only in drag&drop sessions for
	// temporary visual indication of the connection process
	// the isStartPoint lets you specify if the given EndPoint
	// is supposed to be treated as start or end point
						DiagramWire(
							DiagramEndPoint *fromWhich,
							bool isStartPoint = true);

	virtual				~DiagramWire();

public:					// *** accessors

	bool				isDragging() const
						{ return m_temporary; }

	// returns a pointer to the "start/source" endpoint
	DiagramEndPoint	   *startPoint() const
						{ return m_fromEndPoint; }

	// returns the point from the m_fromEndPoints connectionPoint() method
	BPoint				startConnectionPoint() const;
	
	// returns a pointer the "end/target" endpoint
	DiagramEndPoint	   *endPoint() const
						{ return m_toEndPoint; }

	// returns the point from the m_toEndPoints connectionPoint() method
	BPoint				endConnectionPoint() const;

public:					// *** hook functions

	// is called from Draw() to do the actual drawing
	virtual void		drawWire() = 0;

	// is called by the connected diagramEndPoints whenever one is moved
	// if the argument is NULL then both endpoints should be evaluated
	virtual void		endPointMoved(
							DiagramEndPoint *which = 0)
						{ /* does nothing */ }

public:					//  *** derived from DiagramItem

	// returns the area in which the wire displays
	virtual BRect		Frame() const
						{ return BRect(startConnectionPoint(), endConnectionPoint()); }

	// returns how close a given point is to the wire; the current
	// implementation returns a wide array of values, those above
	// approx. 0.5 are quite close to the wire
	float				howCloseTo(
							BPoint point) const;

	// prepares the drawing stack and clipping region, then
	// calls drawWire
	void				Draw(
							BRect updateRect);

	// is called from the parent DiagramViews MouseDown() implementation 
	// if the Wire was hit; this version initiates selection and dragging
	virtual void		MouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	// is called from the DiagramViews MouseMoved() when no message is being 
	// dragged, but the mouse has moved above the wire
	virtual void		MouseOver(
							BPoint point,
							uint32 transit)
						{ /* does nothing */ }
		

	// is called from the DiagramViews MouseMoved() when a message is being 
	// dragged over the DiagramWire
	virtual void		MessageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// is called from the DiagramViews MessageReceived() function when a 
	// message has been dropped on the DiagramWire
	virtual void		MessageDropped(
							BPoint point,
							BMessage *message)
						{ /* does nothing */ }

private:				// *** data

	// pointer to the "from" EndPoint as assigned in the ctor
	DiagramEndPoint	   *m_fromEndPoint;

	// pointer to the "to" EndPoint as assigned in the ctor
	DiagramEndPoint	   *m_toEndPoint;

	// indicates that this is a connection used in a drag&drop session
	// and will be deleted after that
	bool				m_temporary;

	// contains a BPoint for "temporary connections"
	BPoint				m_dragEndPoint;
};

__END_CORTEX_NAMESPACE
#endif // __DiagramWire_H__
