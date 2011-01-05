/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "EntryPage.h"

#include <string.h>

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <TextView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "EntryPage"


EntryPage::EntryPage(BMessage* settings, const char* name)
	:
	WizardPageView(settings, name)
{
	_BuildUI();
}


EntryPage::~EntryPage()
{
}


void
EntryPage::PageCompleted()
{
	fSettings->ReplaceBool("install", fInstallButton->Value() != 0);
}


void
EntryPage::_BuildUI()
{
	fInstallButton = new BRadioButton("install", NULL, NULL);

	BString text;
	text << B_TRANSLATE_COMMENT("Install boot menu", "Title") << "\n"
		<< B_TRANSLATE("Choose this option to install a boot menu, "
			"allowing you to select which operating "
			"system to boot when you turn on your "
			"computer.") << "\n";
	fInstallText = CreateDescription("installText", text);
	MakeHeading(fInstallText);

	fUninstallButton = new BRadioButton("uninstall", NULL, NULL);

	text.Truncate(0);
	text << B_TRANSLATE_COMMENT("Uninstall boot menu", "Title") << "\n"
		<< B_TRANSLATE("Choose this option to remove the boot menu "
			"previously installed by this program.\n");
	fUninstallText = CreateDescription("uninstallText", text);
	MakeHeading(fUninstallText);

	SetLayout(new BGroupLayout(B_VERTICAL));

	BLayoutBuilder::Group<>((BGroupLayout*)GetLayout())
		.AddGroup(B_HORIZONTAL, 0)
			.AddGroup(B_VERTICAL, 0, 0)
				.Add(fInstallButton)
				.AddGlue()
				.End()
			.Add(fInstallText, 1)
			.End()
		.AddGroup(B_HORIZONTAL, 0)
			.AddGroup(B_VERTICAL, 0, 0)
				.Add(fUninstallButton)
				.AddGlue()
				.End()
			.Add(fUninstallText)
			.End()
		.AddGlue();

	if (fSettings->FindBool("install"))
		fInstallButton->SetValue(1);
	else
		fUninstallButton->SetValue(1);
}
