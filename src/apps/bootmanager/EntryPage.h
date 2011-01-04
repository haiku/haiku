/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
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
								EntryPage(BMessage* settings, BRect frame,
									const char* name);
	virtual						~EntryPage();

	virtual	void				FrameResized(float width, float height);

	virtual	void				PageCompleted();

private:
			void				_BuildUI();
			void				_Layout();

private:
			BRadioButton*		fInstall;
			BTextView*			fInstallText;
			BRadioButton*		fUninstall;
			BTextView*			fUninstallText;
};


#endif	// ENTRY_PAGE_H
