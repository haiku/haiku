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


// TextControlFloater.h
// * PURPOSE
//   Display an editable text field in a simple pop-up window
//   (which must automatically close when the user hits 'enter'
//   or the window loses focus).
// * TO DO +++++
//   escape key -> cancel
//
// * HISTORY
//   e.moon		23aug99		Begun

#ifndef __TextControlFloater_H__
#define __TextControlFloater_H__

#include <Messenger.h>
#include <Window.h>

class BFont;
class BTextControl;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TextControlFloater :
	public	BWindow {
	typedef	BWindow _inherited;
	
public:												// dtor/ctors
	virtual ~TextControlFloater();

	TextControlFloater(
		BRect											frame,
		alignment									align,
		const BFont*							font,
		const char*								text,
		const BMessenger&					target,
		BMessage*									message,
		BMessage*									cancelMessage=0);

public:												// BWindow
	virtual void WindowActivated(
		bool											activated);

	virtual bool QuitRequested();
		
public:												// BHandler
	virtual void MessageReceived(
		BMessage*									message);
		
private:
	BTextControl*								m_control;

	BMessenger									m_target;	
	const BMessage*							m_message;
	BMessage*										m_cancelMessage;
	
	// true if a message has been sent indicating the
	// user modified the text
	bool												m_sentUpdate;
};

__END_CORTEX_NAMESPACE
#endif /*__TextControlFloater_H__*/

