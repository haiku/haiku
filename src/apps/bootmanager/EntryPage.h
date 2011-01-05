/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef ENTRY_PAGE_H
#define ENTRY_PAGE_H


#include "WizardPageView.h"


class BRadioButton;
class BTextView;


class EntryPage : public WizardPageView {
public:
								EntryPage(BMessage* settings, const char* name);
	virtual						~EntryPage();

	virtual	void				PageCompleted();

private:
			void				_BuildUI();

private:
			BRadioButton*		fInstallButton;
			BTextView*			fInstallText;
			BRadioButton*		fUninstallButton;
			BTextView*			fUninstallText;
};


#endif	// ENTRY_PAGE_H
