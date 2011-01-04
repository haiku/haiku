/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef WIZARD_VIEW_H
#define WIZARD_VIEW_H


#include <GroupView.h>


class BButton;
class WizardPageView;


const uint32 kMessageNext = 'next';
const uint32 kMessagePrevious = 'prev';


class WizardView : public BGroupView {
public:
								WizardView(const char* name);
	virtual						~WizardView();

	virtual	void				SetPage(WizardPageView* page);

	virtual	void				PageCompleted();

	virtual	void				SetPreviousButtonEnabled(bool enabled);
	virtual	void				SetNextButtonEnabled(bool enabled);
	virtual	void				SetPreviousButtonLabel(const char* text);
	virtual	void				SetNextButtonLabel(const char* text);
	virtual	void				SetPreviousButtonHidden(bool hide);

private:
			void				_BuildUI();

private:
			BGroupView*			fPageContainer;
			BButton*			fPrevious;
			BButton*			fNext;

			WizardPageView*		fPage;
};


#endif	// WIZARD_VIEW_H
