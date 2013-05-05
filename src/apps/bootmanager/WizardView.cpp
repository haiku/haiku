/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "WizardView.h"

#include <LayoutBuilder.h>
#include <Button.h>
#include <Catalog.h>
#include <SeparatorView.h>

#include "WizardPageView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "WizardView"


WizardView::WizardView(const char* name)
	:
	BGroupView(name, B_VERTICAL, 0),
	fPrevious(NULL),
	fNext(NULL),
	fPage(NULL)
{
	_BuildUI();
	SetPreviousButtonHidden(true);
}


WizardView::~WizardView()
{
}


void
WizardView::SetPage(WizardPageView* page)
{
	if (fPage == page)
		return;

	if (fPage != NULL) {
		fPageContainer->RemoveChild(fPage);
		delete fPage;
	}

	fPage = page;
	if (page == NULL)
		return;

	fPageContainer->AddChild(page);
}


void
WizardView::PageCompleted()
{
	if (fPage != NULL)
		fPage->PageCompleted();

	// Restore initial state
	SetNextButtonLabel(B_TRANSLATE_COMMENT("Next", "Button"));
	SetPreviousButtonLabel(B_TRANSLATE_COMMENT("Previous", "Button"));
	SetNextButtonEnabled(true);
	SetPreviousButtonEnabled(true);
	SetPreviousButtonHidden(false);
}


void
WizardView::SetPreviousButtonEnabled(bool enabled)
{
	fPrevious->SetEnabled(enabled);
}


void
WizardView::SetNextButtonEnabled(bool enabled)
{
	fNext->SetEnabled(enabled);
}


void
WizardView::SetPreviousButtonLabel(const char* text)
{
	fPrevious->SetLabel(text);
}


void
WizardView::SetNextButtonLabel(const char* text)
{
	fNext->SetLabel(text);
}


void
WizardView::SetPreviousButtonHidden(bool hide)
{
	if (hide) {
		if (!fPrevious->IsHidden())
			fPrevious->Hide();
	} else {
		if (fPrevious->IsHidden())
			fPrevious->Show();
	}
}


void
WizardView::_BuildUI()
{
	fPageContainer = new BGroupView("page container");
	fPageContainer->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fPrevious = new BButton("previous",
		B_TRANSLATE_COMMENT("Previous", "Button"),
		new BMessage(kMessagePrevious));
	fNext = new BButton("next", B_TRANSLATE_COMMENT("Next", "Button"),
		new BMessage(kMessageNext));

	BLayoutBuilder::Group<>(this)
		.Add(fPageContainer)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(fPrevious)
			.Add(fNext)
			.End()
		.End();
}
