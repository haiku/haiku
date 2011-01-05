/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "DescriptionPage.h"

#include <string.h>

#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <TextView.h>


DescriptionPage::DescriptionPage(const char* name,
	const char* description, bool hasHeading)
	:
	WizardPageView(NULL, name)
{
	_BuildUI(description, hasHeading);
}


DescriptionPage::~DescriptionPage()
{
}


void
DescriptionPage::_BuildUI(const char* description, bool hasHeading)
{
	fDescription = CreateDescription("description", description);
	if (hasHeading)
		MakeHeading(fDescription);
	fDescription->SetTabWidth(120);

	SetLayout(new BGroupLayout(B_VERTICAL));

	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.Add(fDescription);
}
