/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderWindow.h
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

#ifndef _ExpanderWindow_h
#define _ExpanderWindow_h

#include <CheckBox.h>
#include <Window.h>
#include <FilePanel.h>
#include <TextControl.h>
#include <ScrollView.h>
#include "DirectoryFilePanel.h"
#include "ExpanderRules.h"

class ExpanderThread;
class ExpanderPreferences;

class ExpanderWindow : public BWindow {
public:
	ExpanderWindow(BRect frameRect, const entry_ref *ref, BMessage *settings);
	virtual ~ExpanderWindow();

	virtual void FrameResized(float width, float height);
	virtual void MessageReceived(BMessage *msg);
	virtual bool QuitRequested();
	
	void SetRef(const entry_ref *ref);
	void RefsReceived(BMessage *msg);

private:
	bool CanQuit();
		// returns true if the window can be closed safely, false if not
	void CloseWindowOrKeepOpen();
	void OpenDestFolder();
	void AutoListing();
	void AutoExpand();
	void StartExpanding();
	void StopExpanding();
	void StartListing();
	void StopListing();

	BFilePanel *fSourcePanel;
	DirectoryFilePanel *fDestPanel;
	BMenuBar *fBar;
	BMenu *fMenu;
	entry_ref fSourceRef;
	entry_ref fDestRef;
	bool fSourceChanged;
	
	BButton *fSourceButton, *fDestButton, *fExpandButton;
	BMenuItem *fExpandItem, *fShowItem, *fStopItem, 
		*fSourceItem, *fDestItem, *fPreferencesItem;
	BCheckBox *fShowContents;
	BTextControl *fSourceText, *fDestText;
	BTextView *fExpandedText, *fListingText;
	BScrollView *fListingScroll;
	
	ExpanderThread *fListingThread;
	bool fListingStarted;
	
	ExpanderThread *fExpandingThread;
	bool fExpandingStarted;
	
	BMessage fSettings;
	ExpanderPreferences *fPreferences;
	ExpanderRules fRules;
};

#endif /* _ExpanderWindow_h */
