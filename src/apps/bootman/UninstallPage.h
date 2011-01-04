/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef UNINSTALL_PAGE_H
#define UNINSTALL_PAGE_H


#include "WizardPageView.h"


class BTextView;


class UninstallPage : public WizardPageView {
public:
								UninstallPage(BMessage* settings, BRect frame,
									const char* name);
	virtual						~UninstallPage();

	virtual	void				FrameResized(float width, float height);

private:
			void				_BuildUI();
			void				_Layout();

private:
			BTextView*			fDescription;
};


#endif	// UNINSTALL_PAGE_H
