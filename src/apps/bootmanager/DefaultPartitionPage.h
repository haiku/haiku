/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef DEFAULT_PARTITON_PAGE_H
#define DEFAULT_PARTITON_PAGE_H


#include "WizardPageView.h"


class BMenuField;
class BMessage;
class BPopUpMenu;
class BRadioButton;
class BSlider;
class BTextView;


class DefaultPartitionPage : public WizardPageView {
public:
								DefaultPartitionPage(BMessage* settings,
									BRect frame, const char* name);
	virtual						~DefaultPartitionPage();

	virtual	void				FrameResized(float width, float height);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* msg);

private:
			void				_BuildUI();
			BPopUpMenu*			_CreatePopUpMenu();
			void				_GetTimeoutLabel(int32 timeout, BString& label);
			void				_Layout();

private:
			BTextView*			fDescription;
			BMenuField*			fDefaultPartition;
			BSlider*			fTimeoutSlider;
};


#endif	// DEFAULT_PARTITON_PAGE_H
