/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef EXPANDER_WINDOW_H
#define EXPANDER_WINDOW_H


#include <CheckBox.h>
#include <Window.h>
#include <FilePanel.h>
#include <TextControl.h>
#include <ScrollView.h>
#include "DirectoryFilePanel.h"
#include "ExpanderRules.h"

class BMenu;

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

#endif /* EXPANDER_WINDOW_H */
