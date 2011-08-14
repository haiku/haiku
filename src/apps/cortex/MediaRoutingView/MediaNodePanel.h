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


// MediaNodePanel.h
// c.lenz 9oct99
//
// HISTORY
//   c.lenz		9oct99		Begun

#ifndef __MediaNodePanel_H__
#define __MediaNodePanel_H__

// DiagramView
#include "DiagramBox.h"
// MediaRoutingView
#include "MediaJack.h"

// STL
#include <vector>
// Support Kit
#include <String.h>

#include "IStateArchivable.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

int compareID(const void *lValue, const void *rValue);

class MediaIcon;
class NodeRef;

class MediaNodePanel : public DiagramBox,
					   public BHandler,
					   public IStateArchivable
{
	typedef	DiagramBox _inherited;

public:					// *** constants

	// [e.moon 26oct99] moved definitions to MediaNodePanel.cpp
	static float	M_DEFAULT_WIDTH;
	static float	M_DEFAULT_HEIGHT;
	static float	M_LABEL_H_MARGIN;
	static float	M_LABEL_V_MARGIN;
	static float	M_BODY_H_MARGIN;
	static float	M_BODY_V_MARGIN;

public:					// *** accessors

	NodeRef* const								ref;

public:					// *** ctor/dtor

						MediaNodePanel(
							BPoint position,
							NodeRef *nodeRef);

	virtual				~MediaNodePanel();
	
public:					// *** derived from DiagramItem

	virtual void		attachedToDiagram();
	
	virtual void		detachedFromDiagram();

	virtual void		DrawBox();
	
	virtual void		MouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	virtual void		MouseOver(
							BPoint point,
							uint32 transit);

	virtual void		MessageDropped(
							BPoint point,
							BMessage *message);

	virtual void		selected();
	
	virtual void		deselected();

public:					// *** derived from BHandler

	virtual void		MessageReceived(
							BMessage *message);

public:					// *** updating

	// is called by the MediaRoutingView when the layout
	// (i.e. icon size, orientation, default sizes) have 
	// changed
	void				layoutChanged(
							int32 layout);

	// query the NodeManager for all free inputs & outputs
	// and add a MediaJack instance for each; (connected
	// inputs are added when the connection is reported or
	// queried)
	void				populateInit();

	// completely update the list of free input/output jacks
	void				updateIOJacks();

	// arrange the MediaJacks in order of their IDs, resize
	// the panel if more space is needed
	void				arrangeIOJacks();

	// display popup-menu at the given point
	void				showContextMenu(
							BPoint point);

public:					// *** sorting methods

	// used for sorting the panels by media_node_id
	friend int			compareID(
							const void *lValue,
							const void *rValue);
							
public:					// *** IStateArchivable

	status_t importState(
		const BMessage*						archive); //nyi
	
	status_t exportState(
		BMessage*									archive) const; //nyi
		
private:				// *** internal operations

	// fetch node name (shortening as necessary to fit)
	// and update label placement
	void				_prepareLabel();
	
	// update the offscreen bitmap
	void				_updateBitmap();

	void				_drawInto(
							BView *target,
							BRect targetRect,
							int32 layout);
	
	void				_updateIcon(
							int32 layout);

private:				// *** data

	// a pointer to the panel's offscreen bitmap
	BBitmap			   *m_bitmap;

	BBitmap			   *m_icon;

	BString				m_label; // truncated
	
	BString				m_fullLabel; // not truncated
	
	bool				m_labelTruncated;
	
	BPoint				m_labelOffset;

	BRect				m_labelRect;

	BRect				m_bodyRect;

	// cached position in the "other" layout
	BPoint				m_alternatePosition;

	bool				m_mouseOverLabel;
	
	// [e.moon 7dec99]
	static const BPoint		s_invalidPosition;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaNodePanel_H__ */
