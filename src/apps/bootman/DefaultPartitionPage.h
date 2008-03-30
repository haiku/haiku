/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEFAULT_PARTITON_PAGE_H
#define DEFAULT_PARTITON_PAGE_H


#include "WizardPageView.h"

class BMenuField;
class BMessage;
class BRadioButton;
class BPopUpMenu;
class BTextView;


class DefaultPartitionPage : public WizardPageView
{
public:
	DefaultPartitionPage(BMessage* settings, BRect frame, const char* name);
	virtual ~DefaultPartitionPage();
	
	virtual void FrameResized(float width, float height);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

private:

	void _BuildUI();
	BPopUpMenu* _CreatePopUpMenu();
	BRadioButton* _CreateWaitRadioButton(BRect frame, const char* name, const char* label, 
		int32 timeout, int32 defaultTimeout);
	void _Layout();
	
	BTextView* fDescription;
	BMenuField* fDefaultPartition;
	BRadioButton* fWait0;
	BRadioButton* fWait5;
	BRadioButton* fWait10;
	BRadioButton* fWait15;
};

#endif	// DEFAULT_PARTITON_PAGE_H
