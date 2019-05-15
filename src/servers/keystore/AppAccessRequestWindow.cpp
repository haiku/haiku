/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AppAccessRequestWindow.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>

#include <new>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AppAccessRequestWindow"


static const uint32 kMessageDisallow = 'btda';
static const uint32 kMessageOnce = 'btao';
static const uint32 kMessageAlways = 'btaa';


AppAccessRequestWindow::AppAccessRequestWindow(const char* keyringName,
	const char* signature, const char* path, const char* accessString,
	bool appIsNew, bool appWasUpdated)
	:
	BWindow(BRect(50, 50, 100, 100), B_TRANSLATE("Application keyring access"),
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS
			| B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fDoneSem(-1),
	fResult(kMessageDisallow)
{
	fDoneSem = create_sem(0, "application keyring access dialog");
	if (fDoneSem < 0)
		return;


	BBitmap icon = GetIcon(32 * icon_layout_scale());
	fStripeView = new BStripeView(icon);

	float inset = ceilf(be_plain_font->Size() * 0.7);

	BTextView* message = new(std::nothrow) BTextView("Message");
	if (message == NULL)
		return;

	BString details;
	details <<  B_TRANSLATE("The application:\n"
		"%signature% (%path%)\n\n");
	details.ReplaceFirst("%signature%", signature);
	details.ReplaceFirst("%path%", path);

	if (keyringName != NULL) {
		details <<  B_TRANSLATE("requests access to keyring:\n"
			"%keyringName%\n\n");
		details.ReplaceFirst("%keyringName%", keyringName);
	}

	if (accessString != NULL) {
		details <<  B_TRANSLATE("to perform the following action:\n"
			"%accessString%\n\n");
		details.ReplaceFirst("%accessString%", accessString);
	}

	if (appIsNew)
		details <<  B_TRANSLATE("This application hasn't been granted "
		"access before.");
	else if (appWasUpdated) {
		details <<  B_TRANSLATE("This application has been updated since "
		"it was last granted access.");
	} else {
		details <<  B_TRANSLATE("This application doesn't yet have the "
		"required privileges.");
	}

	message->SetText(details);
	message->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	message->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	message->MakeEditable(false);
	message->MakeSelectable(false);
	message->SetWordWrap(true);

	message->SetExplicitMinSize(BSize(message->StringWidth(
		"01234567890123456789012345678901234567890123456789") + inset,
		B_SIZE_UNSET));

	fDisallowButton = new(std::nothrow) BButton(B_TRANSLATE("Disallow"),
		new BMessage(kMessageDisallow));
	fOnceButton = new(std::nothrow) BButton(B_TRANSLATE("Allow once"),
		new BMessage(kMessageOnce));
	fAlwaysButton = new(std::nothrow) BButton(B_TRANSLATE("Allow always"),
		new BMessage(kMessageAlways));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, B_USE_ITEM_SPACING)
		.Add(fStripeView)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
			.AddGroup(new BGroupView(B_VERTICAL, B_USE_ITEM_SPACING))
				.Add(message)
			.End()
			.AddStrut(B_USE_SMALL_SPACING)
			.AddGroup(new BGroupView(B_HORIZONTAL))
				.Add(fDisallowButton)
				.AddGlue()
				.Add(fOnceButton)
				.Add(fAlwaysButton)
			.End()
		.End()
	.End();
}


AppAccessRequestWindow::~AppAccessRequestWindow()
{
	if (fDoneSem >= 0)
		delete_sem(fDoneSem);
}


bool
AppAccessRequestWindow::QuitRequested()
{
	fResult = kMessageDisallow;
	release_sem(fDoneSem);
	return false;
}


void
AppAccessRequestWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageDisallow:
		case kMessageOnce:
		case kMessageAlways:
			fResult = message->what;
			release_sem(fDoneSem);
			return;
	}

	BWindow::MessageReceived(message);
}


status_t
AppAccessRequestWindow::RequestAppAccess(bool& allowAlways)
{
	ResizeToPreferred();
	CenterOnScreen();
	Show();

	while (acquire_sem(fDoneSem) == B_INTERRUPTED)
		;

	status_t result;
	switch (fResult) {
		default:
		case kMessageDisallow:
			result = B_NOT_ALLOWED;
			allowAlways = false;
			break;
		case kMessageOnce:
			result = B_OK;
			allowAlways = false;
			break;
		case kMessageAlways:
			result = B_OK;
			allowAlways = true;
			break;
	}

	LockLooper();
	Quit();
	return result;
}


BBitmap
AppAccessRequestWindow::GetIcon(int32 iconSize)
{
	BBitmap icon(BRect(0, 0, iconSize - 1, iconSize - 1), 0, B_RGBA32);
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, &icon, icon_size(iconSize));
	return icon;
}
