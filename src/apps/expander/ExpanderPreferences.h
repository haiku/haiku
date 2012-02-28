/*
 * Copyright 2004-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef _EXPANDERPREFERENCES_H
#define _EXPANDERPREFERENCES_H


#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <Message.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <Window.h>

#include "DirectoryFilePanel.h"


class ExpanderPreferences : public BWindow {
public:
						ExpanderPreferences(BMessage* settings);
	virtual 			~ExpanderPreferences();
	virtual void 		MessageReceived(BMessage* msg);

private:
	void				_ReadSettings();
	void				_WriteSettings();

	BButton				*fSelect;
	BCheckBox			*fAutoExpand, *fCloseWindow, *fAutoShow, *fOpenDest;
	BMessage			*fSettings;
	BRadioButton		*fDestUse, *fSameDest, *fLeaveDest;
	BTextControl		*fDestText;
	DirectoryFilePanel	*fUsePanel;
	entry_ref			fRef;
};

#endif // _EXPANDERPREFERENCES_H
