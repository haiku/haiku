/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderPreferences.h
//
//
// Copyright (c) 2004 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _ExpanderPreferences_h
#define _ExpanderPreferences_h

#include "DirectoryFilePanel.h"
#include <Window.h>
#include <Message.h>
#include <Button.h>
#include <Entry.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <RadioButton.h>

class ExpanderPreferences : public BWindow {
	public:
		ExpanderPreferences(BMessage *settings);
		virtual ~ExpanderPreferences();
		virtual void MessageReceived(BMessage *msg);

	private:
		BMessage *fSettings;
		BCheckBox *fAutoExpand, *fCloseWindow, *fAutoShow, *fOpenDest;
		BRadioButton *fDestUse, *fSameDest, *fLeaveDest;
		BButton *fSelect;
		BTextControl *fDestText;
		entry_ref fRef;
		DirectoryFilePanel *fUsePanel;
};

#endif /* _ExpanderPreferences_h */
