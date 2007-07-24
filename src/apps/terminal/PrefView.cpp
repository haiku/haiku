/*
 * Copyright 2007 Haiku
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "PrefView.h"

#include "PrefHandler.h"
#include "TermConst.h"
#include "TTextControl.h"

#include <ColorControl.h>
#include <Message.h>
#include <String.h>


////////////////////////////////////////////////////////////////////////////
// PrefView ()
//	Constructor.
////////////////////////////////////////////////////////////////////////////
PrefView::PrefView(BRect frame, const char *name)
	:BView(frame, name, B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fLabel.SetTo(name);
}


////////////////////////////////////////////////////////////////////////////
// ~PrefView ()
//	Destructor.
////////////////////////////////////////////////////////////////////////////
PrefView::~PrefView()
{
}


const char *
PrefView::ViewLabel() const
{
	return fLabel.String();
}


////////////////////////////////////////////////////////////////////////////
// CanApply()
// Determines whether view can reply to Apply command or not.
////////////////////////////////////////////////////////////////////////////
bool
PrefView::CanApply()
{
	return true;
}


void
PrefView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_TEXT_MODIFIED: {
			TTextControl* textControl;
			BControl *control;
			if (msg->FindPointer("source", (void**)&control) >= B_OK
				&& (textControl = dynamic_cast<TTextControl*>(control))) {
				textControl->ModifiedText(true);
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
	}
}


//////////////////////////////////////////////////////////////////////////////
// SetupBColorControl
// Make BColorControl.
//////////////////////////////////////////////////////////////////////////////
BColorControl *
PrefView::SetupBColorControl(BPoint point, color_control_layout layout, float cellSize, ulong message)
{
	BColorControl *control = new BColorControl(point, layout, cellSize, "", new BMessage(message));
	AddChild(control);
	return control;
}

