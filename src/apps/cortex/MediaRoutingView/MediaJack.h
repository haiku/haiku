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


// MediaJack.h
// c.lenz 10oct99
//
// * PURPOSE
//	 DiagramEndPoint derived class implementing the drawing
//	 code and 
//
// HISTORY
//

#ifndef __MediaJack_H__
#define __MediaJack_H__

#include "DiagramEndPoint.h"

// Media Kit
#include <MediaDefs.h>
#include <MediaNode.h>
// Support Kit
#include <String.h>

class BBitmap;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

int compareTypeAndID(const void *lValue, const void *rValue);

class MediaJack : public DiagramEndPoint
{

public:					// *** jack types

	enum jack_t
	{
		M_INPUT,
		M_OUTPUT
	};

public:					// *** constants

	// [e.moon 26oct99] moved definitions to MediaJack.cpp
	static float M_DEFAULT_WIDTH;
	static float M_DEFAULT_HEIGHT;
	static const float M_DEFAULT_GAP;
	static const int32 M_MAX_ABBR_LENGTH;

public:					// *** ctor/dtor

	// Constructor for input jacks
						MediaJack(
							media_input input);

	// Constructor for output jacks
						MediaJack(
							media_output output);

	virtual				~MediaJack();
	
public:					// *** accessors

	// returns the full name of the input/output
	BString				name() const
						{ return m_label; }

	// return true if this is an input jack
	bool				isInput() const
						{ return (m_jackType == M_INPUT); }

	// copies the media_input struct into input; returns
	// B_ERROR if this isn't an input jack
	status_t			getInput(
							media_input *input) const;

	// return true if this is an output jack
	bool				isOutput() const
						{ return (m_jackType == M_OUTPUT); }

	// copies the media_output struct into input; returns
	// B_ERROR if this isn't an input jack
	status_t			getOutput(
							media_output *output) const;
							
public:					// *** derived from DiagramEndPoint/Item

	// is called by the parent DiagramBox after adding endpoint
	virtual void		attachedToDiagram();

	// is called by the parent DiagramBox just before the endpoint
	// will be removed
	virtual void		detachedFromDiagram();

	// the actual drawing code
	virtual void		drawEndPoint();
	
	// returns the coordinate at which a connected MediaWire is
	// supposed to start/end
	virtual BPoint		connectionPoint() const;
	
	// hook called by the base class; just verifies if the jack
	// type isn't equal, i.e. not connecting an input to an input
	virtual bool		connectionRequested(
							DiagramEndPoint *which);

	// displays the context menu for right-clicks
	virtual void		MouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	// changes the mouse cursor and prepares a tooltip
	virtual void		MouseOver(
							BPoint point,
							uint32 transit);

	// changes the mouse cursor
	virtual void		MessageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// updates the offscreen bitmap
	virtual void		selected();

	// updates the offscreen bitmap
	virtual void		deselected();

	// updates the offscreen bitmap
	virtual void		connected();

	// updates the offscreen bitmap
	virtual void		disconnected();

public:					// *** operations

	// updates the jacks bitmap
	void				layoutChanged(
							int32 layout);

	// special function to be called by the parent MediaNodePanel
	// for simple positioning; this method only needs to know the
	// vertical offset of the jack and the left/right frame coords
	// of the panel
	void				setPosition(
							float verticalOffset,
							float leftBoundary,
							float rightBoundary,
							BRegion *updateRegion = 0);

protected:					// *** operations

	// display a popup-menu at given point
	void				showContextMenu(
							BPoint point);

private:				// *** internal methods
	
	// update the offscreen bitmap
	void				_updateBitmap();

	// draw jack into the specified target view
	void				_drawInto(
							BView *target,
							BRect frame,
							int32 layout);

	// make/update an abbreviation for the jacks name
	void				_updateAbbreviation();

public:					// *** sorting methods

	// used for sorting; will put input jacks before output jacks
	// and inside those sort by index
	friend int			compareTypeAndID(
							const void *lValue,
							const void *rValue);

private:				// *** data

	int32				m_jackType;
	BBitmap			   *m_bitmap;
	int32				m_index;
	media_node			m_node;
	media_source		m_source;
	media_destination	m_destination;
	media_format		m_format;
	BString				m_label;
	BString				m_abbreviation;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaJack_H__ */
