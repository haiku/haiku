/*
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "SettingsWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Locker.h>
#include <SeparatorView.h>

#include "Logger.h"
#include "Model.h"
#include "UserUsageConditionsWindow.h"
#include "ServerHelper.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsWindow"

#define WINDOW_FRAME BRect(0, 0, 500, 280)


enum {
	MSG_APPLY =									'aply',
};


SettingsWindow::SettingsWindow(BWindow* parent, Model* model)
	:
	BWindow(WINDOW_FRAME, B_TRANSLATE("Settings"),
		B_FLOATING_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE ),
	fModel(model)
{
	AddToSubset(parent);
	_InitUiControls();
	_UpdateUiFromModel();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fCanShareAnonymousUsageDataCheckBox)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
			// rule off
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(0, B_USE_DEFAULT_SPACING,
				B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fApplyButton)
		.End();

	CenterOnScreen();
}


SettingsWindow::~SettingsWindow()
{
}


void
SettingsWindow::_InitUiControls()
{
	fCanShareAnonymousUsageDataCheckBox = new BCheckBox(
		"share anonymous usage data",
		B_TRANSLATE("Share anonymous usage data with HaikuDepotServer"), NULL);

	fApplyButton = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(MSG_APPLY));
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
}


void
SettingsWindow::_UpdateUiFromModel()
{
	fCanShareAnonymousUsageDataCheckBox->SetValue(
		fModel->CanShareAnonymousUsageData() ? 1 : 0);
}


void
SettingsWindow::_UpdateModelFromUi()
{
	fModel->SetCanShareAnonymousUsageData(
		0 != fCanShareAnonymousUsageDataCheckBox->Value());
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_APPLY:
			_UpdateModelFromUi();
			BMessenger(this).SendMessage(B_QUIT_REQUESTED);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
