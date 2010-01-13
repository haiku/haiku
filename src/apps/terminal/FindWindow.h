/*
 * Copyright 2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef FINDDLG_H_INCLUDED
#define FINDDLG_H_INCLUDED


#include <Messenger.h>
#include <Window.h>


const ulong MSG_FIND = 'msgf';
const ulong MSG_FIND_START = 'msac';
const ulong MSG_FIND_CLOSED = 'mfcl';


class BTextControl;
class BRadioButton;
class BCheckBox;


class FindWindow : public BWindow {
public:
							FindWindow (BMessenger messenger, const BString& str,
								bool findSelection, bool matchWord,
								bool matchCase, bool forwardSearch);
	virtual					~FindWindow();

	virtual	void			Quit();
	virtual	void			MessageReceived(BMessage* msg);

private:
			void			_SendFindMessage();

private:
			BTextControl*	fFindLabel;
			BRadioButton*	fTextRadio;

			BCheckBox*		fForwardSearchBox;
			BCheckBox*		fMatchCaseBox;
			BCheckBox*		fMatchWordBox;
			BButton*		fFindButton;

			BMessenger		fFindDlgMessenger;
};

#endif	// FINDDLG_H_INCLUDED
