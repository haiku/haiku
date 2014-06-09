/*
 * Copyright 2004-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef _EXPANDER_PREFERENCES_H
#define _EXPANDER_PREFERENCES_H


#include <Entry.h>
#include <Window.h>


class BButton;
class BCheckBox;
class BMessage;
class BRadioButton;
class BTextControl;
class DirectoryFilePanel;


class ExpanderPreferences : public BWindow {
public:
								ExpanderPreferences(BMessage* settings);
	virtual						~ExpanderPreferences();

	virtual	void 				MessageReceived(BMessage* message);

private:
			void				_ReadSettings();
			void				_WriteSettings();

			BButton*			fSelect;
			BCheckBox*			fAutoExpand;
			BCheckBox*			fCloseWindow;
			BCheckBox*			fAutoShow;
			BCheckBox*			fOpenDest;
			BMessage*			fSettings;
			BRadioButton*		fDestUse;
			BRadioButton*		fSameDest;
			BRadioButton*		fLeaveDest;
			BTextControl*		fDestText;
			DirectoryFilePanel*	fUsePanel;
			entry_ref			fRef;
};


#endif	// _EXPANDER_PREFERENCES_H
