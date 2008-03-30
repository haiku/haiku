/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNINSTALL_PAGE_H
#define UNINSTALL_PAGE_H


#include "WizardPageView.h"


class BTextView;

class UninstallPage : public WizardPageView
{
public:
	UninstallPage(BMessage* settings, BRect frame, const char* name);
	virtual ~UninstallPage();
	
	virtual void FrameResized(float width, float height);

private:

	void _BuildUI();
	void _Layout();
	
	BTextView* fDescription;
};

#endif	// UNINSTALL_PAGE_H
