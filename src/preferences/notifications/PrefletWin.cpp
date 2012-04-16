/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Application.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Button.h>
#include <Catalog.h>

#include "PrefletWin.h"
#include "PrefletView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrefletWin"


const int32 kRevert = '_RVT';
const int32 kSave = '_SAV';


PrefletWin::PrefletWin()
	:
	BWindow(BRect(0, 0, 1, 1), B_TRANSLATE_SYSTEM_NAME("Notifications"),
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE
		| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	// Preflet container view
	fMainView = new PrefletView(this);

	// Save and revert buttons
	fRevert = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kRevert));
	fRevert->SetEnabled(false);
	fSave = new BButton("save", B_TRANSLATE("Save"), new BMessage(kSave));
	fSave->SetEnabled(false);

	// Calculate inset
	float inset = ceilf(be_plain_font->Size() * 0.7f);

	// Build the layout
	SetLayout(new BGroupLayout(B_VERTICAL));

	// Add childs
	AddChild(BGroupLayoutBuilder(B_VERTICAL, inset)
		.Add(fMainView)

		.AddGroup(B_HORIZONTAL, inset)
			.AddGlue()
			.Add(fRevert)
			.Add(fSave)
		.End()

		.SetInsets(inset, inset, inset, inset)
	);

	// Center this window on screen and show it
	CenterOnScreen();
	Show();
}


void
PrefletWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kSave:
			for (int32 i = 0; i < fMainView->CountPages(); i++) {
				SettingsPane* pane =
					dynamic_cast<SettingsPane*>(fMainView->PageAt(i));
				if (pane) {
					if (pane->Save() == B_OK) {
						fSave->SetEnabled(false);
						fRevert->SetEnabled(true);
					} else
						break;
				}
			}
			break;
		case kRevert:
			for (int32 i = 0; i < fMainView->CountPages(); i++) {
				SettingsPane* pane =
					dynamic_cast<SettingsPane*>(fMainView->PageAt(i));
				if (pane) {
					if (pane->Revert() == B_OK)
						fRevert->SetEnabled(false);
				}
			}
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}


bool
PrefletWin::QuitRequested()
{
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
PrefletWin::SettingChanged()
{
	fSave->SetEnabled(true);
}
