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