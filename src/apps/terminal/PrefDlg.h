/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PREFDLG_H_INCLUDED
#define PREFDLG_H_INCLUDED

#include <Box.h>
#include <Window.h>
#include "PrefHandler.h"

// local message, so these are in .cpp files....
const ulong MSG_SAVE_PRESSED = 'okok';
const ulong MSG_SAVEAS_PRESSED = 'canl';
const ulong MSG_REVERT_PRESSED = 'revt';
const ulong MSG_PREF_CLOSED = 'mspc';


// Notify PrefDlg closed to TermWindow

class BRect;
class BMessage;
class BTextControl;
class BButton;
class PrefHandler;
class BFilePanel;

class PrefDlg : public BWindow
{
	public:
		PrefDlg(BMessenger messenger);
		virtual ~PrefDlg();
		
		virtual void Quit();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *msg);

	private:
		void _Save();
		void _SaveAs();
		void _Revert();
		void _SaveRequested(BMessage *msg);

		static BRect CenteredRect(BRect r);

		PrefHandler		*fPrefTemp;
		BFilePanel		*fSavePanel;
	
		BButton			*fSaveAsFileButton,
						*fRevertButton,
						*fSaveButton;

		bool			fDirty;

		BMessenger		fPrefDlgMessenger;
};

#endif //PREFDLG_H_INCLUDED
