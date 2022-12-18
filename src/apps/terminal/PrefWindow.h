/*
 * Copyright 2007, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai. 
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PREFDLG_H_INCLUDED
#define PREFDLG_H_INCLUDED


#include "PrefHandler.h"

#include <Box.h>
#include <Window.h>

// local messages
const uint32 MSG_DEFAULTS_PRESSED = 'defl';
const uint32 MSG_SAVEAS_PRESSED = 'canl';
const uint32 MSG_REVERT_PRESSED = 'revt';
const uint32 MSG_PREF_CLOSED = 'mspc';

class BButton;
class BFilePanel;
class BMessage;
class BRect;
class BTextControl;

class PrefHandler;


class PrefWindow : public BWindow
{
	public:
		PrefWindow(const BMessenger &messenger);
		virtual ~PrefWindow();

		virtual void Quit();
		virtual bool QuitRequested();
		virtual void MessageReceived(BMessage *msg);

	private:
		void _Save();
		void _SaveAs();
		void _Revert();
		void _SaveRequested(BMessage *msg);
		BRect _CenteredRect(BRect rect);

	private:
		PrefHandler		*fPreviousPref;
		BFilePanel		*fSavePanel;

		BButton			*fSaveAsFileButton,
						*fRevertButton,
						*fDefaultsButton;

		AppearancePrefView	*fAppearanceView;

		bool			fDirty;
		BMessenger		fTerminalMessenger;
};

#endif	// PREFDLG_H_INCLUDED
