/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DESCRIPTION_PAGE_H
#define DESCRIPTION_PAGE_H


#include "WizardPageView.h"


class BTextView;

class DescriptionPage : public WizardPageView
{
public:
	DescriptionPage(BRect frame, const char* name, const char* description, bool hasHeading);
	virtual ~DescriptionPage();
	
	virtual void FrameResized(float width, float height);

private:

	void _BuildUI(const char* description, bool hasHeading);
	void _Layout();
	
	BTextView* fDescription;
};

#endif	// DESCRIPTION_PAGE_H
