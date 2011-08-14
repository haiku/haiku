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


// DiagramItem.h (Cortex/DiagramView)
//
// * PURPOSE
//   DiagramItem subclass providing a basic implementation
//   of 'connectable' points inside a DiagramBox
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramEndPoint_H__
#define __DiagramEndPoint_H__

#include <Looper.h>
#include <Region.h>

#include "DiagramItem.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramWire;

class DiagramEndPoint : public DiagramItem
{

public:					// *** ctor/dtor

						DiagramEndPoint(
							BRect frame);

	virtual				~DiagramEndPoint();

public:					// *** accessors

	DiagramWire		   *wire() const
						{ return m_wire; }

public:					// *** hook functions

	// is called from Draw() to do the actual drawing
	virtual void		drawEndPoint() = 0;

	// should return the BPoint that a connected wire uses as
	// a drawing offset; this implementation returns the center 
	// point of the EndPoint
	virtual BPoint		connectionPoint() const;

	// is called from MessageDragged() and MessageDropped() to 
	// let you verify that a connection is okay
	virtual bool		connectionRequested(
							DiagramEndPoint *fromWhich);
	
	// is called whenever a connection has been made or a
	// temporary wire drag has been accepted
	virtual void		connected()
						{ /* does nothing */ }

	// is called whenever a connection has been broken or a
	// temporary wire drag has finished
	virtual void		disconnected()
						{ /* does nothing */ }

public:					// *** derived from DiagramItem

	// returns the EndPoints frame rectangle
	BRect				Frame() const
						{ return m_frame; }

	// prepares the drawing stack and clipping region, then
	// calls drawEndPoint
	void				Draw(
							BRect updateRect);

	// is called from the parent DiagramBox's MouseDown() 
	// implementation if the EndPoint was hit; this version
	// initiates selection and dragging (i.e. connecting)
	virtual void		MouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	// is called from the parent DiagramBox's MouseOver()
	// implementation if the mouse is over the EndPoint
	virtual void		MouseOver(
							BPoint point,
							uint32 transit)
						{ /* does nothing */ }

	// is called from the parent DiagramBox's MessageDragged()
	// implementation of a message is being dragged of the
	// EndPoint; this implementation handles only wire drags
	// (i.e. M_WIRE_DRAGGED messages)
	virtual void		MessageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);
							
	// is called from the parent DiagramBox's MessageDropped()
	// implementation if the message was dropped on the EndPoint;
	// this version handles wire dropping (connecting)
	virtual void		MessageDropped(
							BPoint point,
							BMessage *message);
								
	// moves the EndPoint by the specified amount and returns in
	// updateRegion the frames of connected wires if there are any
	// NOTE: no drawing/refreshing is done in this method, that's
	// up to the parent
	void				MoveBy(
							float x,
							float y,
							BRegion *updateRegion);
		
	// resize the EndPoints frame rectangle without redraw
	virtual void		ResizeBy(
							float horizontal,
							float vertical);

public:					// *** connection methods

	// connect/disconnect and set the wire pointer accordingly
	void				connect(
							DiagramWire *wire);
	void				disconnect();

	// returns true if the EndPoint is currently connected
	bool				isConnected() const
						{ return m_connected; }

	// returns true if the EndPoint is currently in a connection
	// process (i.e. it is either being dragged somewhere, or a
	// M_WIRE_DRAGGED message is being dragged over it
	bool				isConnecting() const
						{ return m_connecting; }

private:				// *** data

	// the endpoints' frame rectangle
	BRect				m_frame;

	// pointer to the wire object this endpoint might be
	// connected to
	DiagramWire		   *m_wire;

	// indicates if the endpoint is currently connected
	// to another endpoint
	bool				m_connected;
	
	// indicates that a connection is currently being made
	// but has not been confirmed yet
	bool				m_connecting;
};

__END_CORTEX_NAMESPACE
#endif // __DiagramEndPoint_H__
