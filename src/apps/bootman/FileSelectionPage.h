/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_SELECTION_PAGE_H
#define FILE_SELECTION_PAGE_H


#include "WizardPageView.h"


class BButton;
class BTextControl;
class BTextView;

class FileSelectionPage : public WizardPageView
{
public:
	FileSelectionPage(BMessage* settings, BRect frame, const char* name, const char* description);
	virtual ~FileSelectionPage();
	
	virtual void FrameResized(float width, float height);
	
	virtual void PageCompleted();

private:

	void _BuildUI(const char* description);
	void _Layout();
	
	BTextView* fDescription;
	BTextControl* fFile;
	BButton* fSelect;
};

#endif	// FILE_SELECTION_PAGE_H
