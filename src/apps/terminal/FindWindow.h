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


class FindDlg : public BWindow {
	public:
		FindDlg (BRect frame, BMessenger messenger, BString &str, 
			bool findSelection, bool matchWord, bool matchCase, bool forwardSearch);
		virtual ~FindDlg();

		virtual void Quit();
		virtual void MessageReceived(BMessage *msg);

	private:
		void _SendFindMessage();

	private:
		BView 			*fFindView;
		BTextControl 	*fFindLabel;
		BRadioButton 	*fTextRadio;
		BRadioButton 	*fSelectionRadio;
		BBox 			*fSeparator;
		BCheckBox		*fForwardSearchBox;
		BCheckBox		*fMatchCaseBox;
		BCheckBox		*fMatchWordBox;
		BButton			*fFindButton;

		BString	*fFindString;
		BMessenger fFindDlgMessenger;
};

#endif	// FINDDLG_H_INCLUDED
