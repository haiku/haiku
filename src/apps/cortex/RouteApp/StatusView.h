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


// StatusView.h (Cortex/ParameterWindow)
//
// * PURPOSE
//	 a small view to display a short message string with
//   an icon. for error messages, it will play a sound for
//   catching the users attention. messages will decay slowly
//   (become fully transparent). all this to avoid modal alert
//   boxes for error notification.
//
// * TODO
//   reduce flicker while message decay
//
// * HISTORY
//   c.lenz		21may00		Begun
//

#ifndef __StatusView_H__
#define __StatusView_H__

// Interface Kit
#include <StringView.h>
// Support Kit
#include <String.h>

#include "cortex_defs.h"

class BBitmap;
class BMessageRunner;

__BEGIN_CORTEX_NAMESPACE

class RouteAppNodeManager;

class StatusView :
	public BStringView {
	
public:					// *** ctor/dtor

						StatusView(
							BRect frame,
							RouteAppNodeManager *manager,
							BScrollBar *scrollBar = 0);

	virtual				~StatusView();

public:					// *** BScrollView impl.

	virtual void		AttachedToWindow();

	virtual void		Draw(
							BRect updateRect);

	virtual void		FrameResized(
							float width,
							float height);

	virtual void		MessageReceived(
							BMessage *message);

	virtual void		MouseDown(
							BPoint point);

	virtual void		MouseMoved(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	virtual void		MouseUp(
							BPoint point);

//	virtual void		Pulse();

public:					// *** operations

	void				drawInto(
							BView *view,
							BRect updateRect);

	void				setMessage(
							BString &title,
							BString &details,
							status_t error = B_OK);

	void				setErrorMessage(
							BString text,
							bool error = false);
							
	void				startFade();
	
	void				fadeTick();
	
	void				allocBackBitmap(
							float width,
							float height);
	
	void				freeBackBitmap();

private:					// *** data members

	// the sibling scrollbar which should be resized by the 
	// status view
	BScrollBar *			m_scrollBar;

	BBitmap	*				m_icon;

	// from 0.0 to 1.0
	float					m_opacity;
	int32					m_decayDelay;
	BMessageRunner *		m_clock;

	// untruncated string
	BString					m_fullText;

	// is being resized
	bool					m_dragging;
	
	// offscreen buffer
	BBitmap *				m_backBitmap;
	BView *					m_backView;
	bool					m_dirty;

	RouteAppNodeManager *	m_manager;
};

__END_CORTEX_NAMESPACE
#endif /* __StatusView_H__ */
