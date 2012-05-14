/*
 * Copyright 2002-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Alexander von Gluck, kallisti5@unixzen.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "APRWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <Messenger.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>

#include "AntialiasingSettingsView.h"
#include "APRView.h"
#include "DecorSettingsView.h"
#include "defs.h"
#include "FontView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "APRWindow"


static const uint32 kMsgSetDefaults = 'dflt';
static const uint32 kMsgRevert = 'rvrt';


APRWindow::APRWindow(BRect frame)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Appearance"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE,
		B_ALL_WORKSPACES)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	fDefaultsButton = new BButton("defaults", B_TRANSLATE("Defaults"),
		new BMessage(kMsgSetDefaults), B_WILL_DRAW);

	fRevertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevert), B_WILL_DRAW);

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	fFontSettings = new FontView(B_TRANSLATE("Fonts"));

	fColorsView = new APRView(B_TRANSLATE("Colors"));

	fDecorSettings = new DecorSettingsView(
		B_TRANSLATE("Decorators"));

	fAntialiasingSettings = new AntialiasingSettingsView(
		B_TRANSLATE("Antialiasing"));

	tabView->AddTab(fFontSettings);
	tabView->AddTab(fColorsView);
	tabView->AddTab(fDecorSettings);
	tabView->AddTab(fAntialiasingSettings);

	_UpdateButtons();

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(tabView)
		.Add(BSpaceLayoutItem::CreateVerticalStrut(5))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fDefaultsButton)
		)
		.SetInsets(5, 5, 5, 5)
	);
}


void
APRWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgUpdate:
			_UpdateButtons();
			break;

		case kMsgSetDefaults:
			fFontSettings->SetDefaults();
			fColorsView->SetDefaults();
			fAntialiasingSettings->SetDefaults();
			fDecorSettings->SetDefaults();

			_UpdateButtons();
			break;

		case kMsgRevert:
			fColorsView->Revert();
			fAntialiasingSettings->Revert();
			fFontSettings->Revert();
			fDecorSettings->Revert();

			_UpdateButtons();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
APRWindow::_UpdateButtons()
{
	fDefaultsButton->SetEnabled(_IsDefaultable());
	fRevertButton->SetEnabled(_IsRevertable());
}	


bool
APRWindow::_IsDefaultable() const
{
//	printf("fonts defaultable: %d\n", fFontSettings->IsDefaultable());
//	printf("colors defaultable: %d\n", fColorsView->IsDefaultable());
//	printf("AA defaultable: %d\n", fAntialiasingSettings->IsDefaultable());
//	printf("decor defaultable: %d\n", fDecorSettings->IsDefaultable());
	return fColorsView->IsDefaultable()
		|| fFontSettings->IsDefaultable()
		|| fAntialiasingSettings->IsDefaultable()
		|| fDecorSettings->IsDefaultable();
}


bool
APRWindow::_IsRevertable() const
{
//	printf("fonts revertable: %d\n", fFontSettings->IsRevertable());
//	printf("colors revertable: %d\n", fColorsView->IsRevertable());
//	printf("AA revertable: %d\n", fAntialiasingSettings->IsRevertable());
//	printf("decor revertable: %d\n", fDecorSettings->IsRevertable());
	return fColorsView->IsRevertable()
		|| fFontSettings->IsRevertable()
		|| fAntialiasingSettings->IsRevertable()
		|| fDecorSettings->IsRevertable();
}
