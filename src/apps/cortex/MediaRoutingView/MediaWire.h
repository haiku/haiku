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


// MediaWire.h
// c.lenz 10oct99
//
// HISTORY
//

#ifndef __MediaWire_H__
#define __MediaWire_H__

// DiagramView
#include "DiagramWire.h"
// NodeManager
#include "Connection.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaJack;

class MediaWire :
	public		DiagramWire {
	typedef		DiagramWire _inherited;

public:					// *** constans

	// [e.moon 26oct99] moved definition to MediaWire.cpp
	static const float		M_WIRE_OFFSET;

public:					// *** ctor/dtor

	// input and output jack are set connected by this constructor
	// so be careful to only pass in valid pointers
						MediaWire(
							Connection connection,
							MediaJack *outputJack,
							MediaJack *inputJack);

	// special constructor used only in drag&drop sessions for
	// temporary visual indication of the connection process
	// the isStartPoint specifies if the given EndPoint is 
	// supposed to be treated as start or end point
						MediaWire(
							MediaJack *jack,
							bool isStartPoint);

	virtual				~MediaWire();
	
public:					// *** accessors

	Connection			connection;

public:					// *** derived from DiagramWire/Item

	// init the cached points and the frame rect
	virtual void		attachedToDiagram();

	// make sure no tooltip is being displayed for this wire
	// +++ DOES NOT WORK (yet)
	virtual void		detachedFromDiagram();

	// calculates and returns the frame rectangle of the wire
	virtual BRect		Frame() const;

	// returns a value > 0.5 for points pretty much close to the
	// wire
	virtual float		howCloseTo(
							BPoint point) const;

	// does the actual drawing
	virtual void		drawWire();

	// displays the context-menu for right-clicks
	virtual void		MouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);	

	// changes the mouse cursor and starts a tooltip
	virtual void		MouseOver(
							BPoint point,
							uint32 transit);

	// also selects and invalidates the jacks connected by 
	// this wire
	virtual void		selected();
	
	// also deselectes and invalidates the jacks connected
	// by this wire
	virtual void		deselected();

	// updates the cached start & end points and the frame rect
	virtual void		endPointMoved(
							DiagramEndPoint *which = 0);

protected:					// *** operations

	// display a popup-menu at given point
	void				showContextMenu(
							BPoint point);

private:					// *** data members

	BPoint				m_startPoint;
	
	BPoint				m_endPoint;
	
	BPoint				m_startOffset;
	
	BPoint				m_endOffset;
	
	BRect				m_frame;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaWire_H__ */
