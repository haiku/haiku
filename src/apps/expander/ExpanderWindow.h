/*
 * Copyright 2004-2006 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Karsten Heimrich, host.haiku@gmx.de
 */
#ifndef _EXPANDER_WINDOW_H
#define _EXPANDER_WINDOW_H


#include <Window.h>


#include "DirectoryFilePanel.h"
#include "ExpanderRules.h"


class BCheckBox;
class BMenu;
class BLayout;
class BScrollView;
class BStringView;
class BTextControl;
class BTextView;

class ExpanderThread;
class ExpanderPreferences;


class ExpanderWindow : public BWindow {
public:
								ExpanderWindow(BRect frameRect,
									const entry_ref* ref, BMessage* settings);
	virtual						~ExpanderWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

//			void				SetRef(const entry_ref* ref);
			void				RefsReceived(BMessage* message);

private:
			void				_CreateMenuBar();
			bool				CanQuit();
				// returns true if the window can be closed safely, false if not
			void				CloseWindowOrKeepOpen();
			void				OpenDestFolder();
			void				AutoListing();
			void				AutoExpand();
			void				StartExpanding();
			void				StopExpanding();
			void				_UpdateWindowSize(bool showContents);
			void				_ExpandListingText();
			void				StartListing();
			void				StopListing();
			void				SetStatus(BString text);
			bool				ValidateDest();

private:
			BFilePanel*			fSourcePanel;
			DirectoryFilePanel*	fDestPanel;
			BMenuBar*			fBar;
			BMenu*				fMenu;
			entry_ref			fSourceRef;
			entry_ref			fDestRef;
			bool				fSourceChanged;

			BButton*			fSourceButton;
			BButton*			fDestButton;
			BButton*			fExpandButton;
			BMenuItem*			fExpandItem;
			BMenuItem*			fShowItem;
			BMenuItem*			fStopItem;
			BMenuItem*			fSourceItem;
			BMenuItem*			fDestItem;
			BMenuItem*			fPreferencesItem;
			BCheckBox*			fShowContents;
			BTextControl*		fSourceText;
			BTextControl*		fDestText;
			BStringView*		fStatusView;
			BTextView*			fListingText;
			BScrollView*		fScrollView;

			ExpanderThread*		fListingThread;
			bool				fListingStarted;

			ExpanderThread*		fExpandingThread;
			bool				fExpandingStarted;

			BMessage			fSettings;
			ExpanderPreferences*	fPreferences;
			ExpanderRules		fRules;

			float				fLongestLine;
			float				fLineHeight;
			float				fSizeLimit;
			float				fPreviousHeight;
};


#endif	// _EXPANDER_WINDOW_H
