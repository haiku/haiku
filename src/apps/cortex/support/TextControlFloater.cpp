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


// TextControlFloater.cpp

#include "TextControlFloater.h"

#include <Debug.h>
#include <TextControl.h>

__USE_CORTEX_NAMESPACE

// ---------------------------------------------------------------- //

class MomentaryTextControl :
	public	BTextControl {
	typedef	BTextControl _inherited;
	
public:
	MomentaryTextControl(
		BRect											frame,
		const char*								name,
		const char*								label,
		const char*								text,
		BMessage*									message,
		uint32										resizingMode=B_FOLLOW_LEFT|B_FOLLOW_TOP,
		uint32										flags=B_WILL_DRAW|B_NAVIGABLE) :
		BTextControl(frame,  name, label, text, message, resizingMode, flags) {
	
		SetEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS);	
	}
	
public:
	virtual void AllAttached() {
		TextView()->SelectAll();
		
		// size parent to fit me
		Window()->ResizeTo(
			Bounds().Width(),
			Bounds().Height());

		Window()->Show();
	}
	
	virtual void MouseDown(
		BPoint										point) {
		
		if(Bounds().Contains(point))
			_inherited::MouseDown(point);
		
		Invoke();
//		// +++++ shouldn't an out-of-bounds click send the changed value?
//		BMessenger(Window()).SendMessage(B_QUIT_REQUESTED);
	}
	
	virtual void KeyDown(
		const char*								bytes,
		int32											numBytes) {
		
		if(numBytes == 1 && *bytes == B_ESCAPE) {
			BWindow* w = Window(); // +++++ maui/2 workaround
			BMessenger(w).SendMessage(B_QUIT_REQUESTED);
			return;
		}
	}
};

// ---------------------------------------------------------------- //

// ---------------------------------------------------------------- //
// dtor/ctors
// ---------------------------------------------------------------- //

TextControlFloater::~TextControlFloater() {
	if(m_cancelMessage)
		delete m_cancelMessage;
}

TextControlFloater::TextControlFloater(
	BRect											frame,
	alignment									align,
	const BFont*							font,
	const char*								text,
	const BMessenger&					target,
	BMessage*									message,
	BMessage*									cancelMessage) :
	BWindow(
		frame, //.InsetBySelf(-3.0,-3.0), // expand frame to counteract control border
		"TextControlFloater",
		B_NO_BORDER_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL,
		0),
	m_target(target),
	m_message(message),
	m_cancelMessage(cancelMessage),
	m_sentUpdate(false) {
	
	m_control = new MomentaryTextControl(
		Bounds(),
		"textControl",
		0,
		text,
		message,
		B_FOLLOW_ALL_SIDES);

	Run();
	Lock();
	
	m_control->TextView()->SetFontAndColor(font);
	m_control->TextView()->SetAlignment(align);
	m_control->SetDivider(0.0);

	m_control->SetViewColor(B_TRANSPARENT_COLOR);
	m_control->TextView()->SelectAll();
	AddChild(m_control);
	m_control->MakeFocus();
	
	Unlock();
}

// ---------------------------------------------------------------- //
// BWindow
// ---------------------------------------------------------------- //

void TextControlFloater::WindowActivated(
	bool											activated) {

	if(!activated)
		// +++++ will the message get sent first?
		Quit();
}

// ---------------------------------------------------------------- //
// BHandler
// ---------------------------------------------------------------- //

void TextControlFloater::MessageReceived(
	BMessage*									message) {
	
	if(message->what == m_message->what) {
		// done; relay message to final target
		message->AddString("_value", m_control->TextView()->Text());
		m_target.SendMessage(message);
		m_sentUpdate = true;
		Quit();
		return;
	}
	
	switch(message->what) {
		default:
			_inherited::MessageReceived(message);
	}
}		

bool TextControlFloater::QuitRequested() {
	if(!m_sentUpdate && m_cancelMessage)
		m_target.SendMessage(m_cancelMessage);
		
	return true;
}

// END -- TextControlFloater.cpp --
