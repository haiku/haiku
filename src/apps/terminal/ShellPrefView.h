/*
 * Copyright (c) 2003-2004 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT license.
 */
#ifndef SHELLPREFVIEW_H
#define SHELLPREFVIEW_H_


#include "PrefView.h"


class TermWindow;
class TTextControl;

class ShellPrefView : public PrefView {
	public:
		ShellPrefView(BRect frame, const char* name, TermWindow* window);

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage* message);

		void Revert();
		void SetControlLabels(PrefHandler& Labels);

		virtual void SaveIfModified();

	private:
		TTextControl*	mCols;
		TTextControl*	mRows;

		TTextControl*	mShell;
		TTextControl*	mHistory;

		TermWindow*		fTermWindow;
};

#endif	// SHELLPREFVIEW_H
