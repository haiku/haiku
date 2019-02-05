/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@tycho.email>
 */


#include "SoftwareUpdaterWindow.h"

#include <Alert.h>
#include <AppDefs.h>
#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>

#include "constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdaterWindow"


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 300, 10),
		B_TRANSLATE_SYSTEM_NAME("SoftwareUpdater"), B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fStripeView(NULL),
	fHeaderView(NULL),
	fDetailView(NULL),
	fUpdateButton(NULL),
	fCancelButton(NULL),
	fStatusBar(NULL),
	fCurrentState(STATE_HEAD),
	fWaitingSem(-1),
	fWaitingForButton(false),
	fUpdateConfirmed(false),
	fUserCancelRequested(false),
	fWarningAlertCount(0),
	fSettingsReadStatus(B_ERROR),
	fSaveFrameChanges(false),
	fMessageRunner(NULL),
	fFrameChangeMessage(kMsgWindowFrameChanged)
{
	// Layout
	BBitmap icon = GetIcon(32 * icon_layout_scale());
	fStripeView = new StripeView(icon);

	fUpdateButton = new BButton(B_TRANSLATE("Update now"),
		new BMessage(kMsgUpdateConfirmed));
	fUpdateButton->MakeDefault(true);
	fCancelButton = new BButton(B_TRANSLATE("Cancel"),
		new BMessage(kMsgCancel));

	fHeaderView = new BStringView("header",
		B_TRANSLATE("Checking for updates"), B_WILL_DRAW);
	fHeaderView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fHeaderView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	fDetailView = new BStringView("detail", B_TRANSLATE("Contacting software "
		"repositories to check for package updates."), B_WILL_DRAW);
	fDetailView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fDetailView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	fStatusBar = new BStatusBar("progress");
	fStatusBar->SetMaxValue(100);

	fListView = new PackageListView();
	fScrollView = new BScrollView("scrollview", fListView, B_WILL_DRAW,
		false, true);

	fDetailsCheckbox = new BCheckBox("detailscheckbox",
		B_TRANSLATE("Show more details"),
		new BMessage(kMsgMoreDetailsToggle));

	BFont font;
	fHeaderView->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	font.SetSize(font.Size() * 1.5);
	fHeaderView->SetFont(&font,
		B_FONT_FAMILY_AND_STYLE | B_FONT_SIZE | B_FONT_FLAGS);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, B_USE_ITEM_SPACING)
		.Add(fStripeView)
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(0, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING)
			.AddGroup(new BGroupView(B_VERTICAL, B_USE_ITEM_SPACING))
				.Add(fHeaderView)
				.Add(fDetailView)
				.Add(fStatusBar)
				.Add(fScrollView)
			.End()
			.AddStrut(B_USE_SMALL_SPACING)
			.AddGroup(new BGroupView(B_HORIZONTAL))
				.Add(fDetailsCheckbox)
				.AddGlue()
				.Add(fCancelButton)
				.Add(fUpdateButton)
			.End()
		.End()
	.End();

	fDetailsLayoutItem = layout_item_for(fDetailView);
	fProgressLayoutItem = layout_item_for(fStatusBar);
	fPackagesLayoutItem = layout_item_for(fScrollView);
	fCancelButtonLayoutItem = layout_item_for(fCancelButton);
	fUpdateButtonLayoutItem = layout_item_for(fUpdateButton);
	fDetailsCheckboxLayoutItem = layout_item_for(fDetailsCheckbox);

	_SetState(STATE_DISPLAY_STATUS);
	CenterOnScreen();
	SetFlags(Flags() ^ B_AUTO_UPDATE_SIZE_LIMITS);

	// Prevent resizing for now
	fDefaultRect = Bounds();
	SetSizeLimits(fDefaultRect.Width(), fDefaultRect.Width(),
		fDefaultRect.Height(), fDefaultRect.Height());

	// Read settings file
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath);
	if (status == B_OK) {
		fSettingsPath.Append(kSettingsFilename);
		fSettingsReadStatus = _ReadSettings(fInitialSettings);
	}
	// Move to saved setting position
	if (fSettingsReadStatus == B_OK) {
		BRect windowFrame;
		status = fInitialSettings.FindRect(kKeyWindowFrame, &windowFrame);
		if (status == B_OK) {
			BScreen screen(this);
			if (screen.Frame().Contains(windowFrame.LeftTop()))
				MoveTo(windowFrame.LeftTop());
		}
	}
	Show();

	BMessage registerMessage(kMsgRegister);
	registerMessage.AddMessenger(kKeyMessenger, BMessenger(this));
	be_app->PostMessage(&registerMessage);

	fCancelAlertResponse.SetMessage(new BMessage(kMsgCancelResponse));
	fCancelAlertResponse.SetTarget(this);
	fWarningAlertDismissed.SetMessage(new BMessage(kMsgWarningDismissed));
	fWarningAlertDismissed.SetTarget(this);

	// Common elements used for the zoom height and width calculations
	fZoomHeightBaseline = 6
		+ be_control_look->ComposeSpacing(B_USE_SMALL_SPACING)
		+ 2 * be_control_look->ComposeSpacing(B_USE_WINDOW_SPACING);
	fZoomWidthBaseline = fStripeView->PreferredSize().Width()
		+ be_control_look->ComposeSpacing(B_USE_ITEM_SPACING)
		+ fScrollView->ScrollBar(B_VERTICAL)->PreferredSize().Width()
		+ be_control_look->ComposeSpacing(B_USE_WINDOW_SPACING);
}


bool
SoftwareUpdaterWindow::QuitRequested()
{
	PostMessage(kMsgCancel);
	return false;
}


void
SoftwareUpdaterWindow::FrameMoved(BPoint newPosition)
{
	BWindow::FrameMoved(newPosition);

	// Create a message runner to consolidate all function calls from a
	// move into one message post after moving has ceased for .5 seconds
	if (fSaveFrameChanges) {
		if (fMessageRunner == NULL) {
			fMessageRunner = new BMessageRunner(this, &fFrameChangeMessage,
				500000, 1);
		} else
			fMessageRunner->SetInterval(500000);
	}
}


void
SoftwareUpdaterWindow::FrameResized(float newWidth, float newHeight)
{
	BWindow::FrameResized(newWidth, newHeight);

	// Create a message runner to consolidate all function calls from a
	// resize into one message post after resizing has ceased for .5 seconds
	if (fSaveFrameChanges) {
		if (fMessageRunner == NULL) {
			fMessageRunner = new BMessageRunner(this, &fFrameChangeMessage,
				500000, 1);
		} else
			fMessageRunner->SetInterval(500000);
	}
}


void
SoftwareUpdaterWindow::Zoom(BPoint origin, float width, float height)
{
	// Override default zoom behavior and keep window at same position instead
	// of centering on screen
	BWindow::Zoom(Frame().LeftTop(), width, height);
}


void
SoftwareUpdaterWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case kMsgTextUpdate:
		{
			if (fCurrentState == STATE_DISPLAY_PROGRESS)
				_SetState(STATE_DISPLAY_STATUS);
			else if (fCurrentState != STATE_DISPLAY_STATUS)
				break;

			BString header;
			BString detail;
			Lock();
			status_t result = message->FindString(kKeyHeader, &header);
			if (result == B_OK && header != fHeaderView->Text())
				fHeaderView->SetText(header.String());
			result = message->FindString(kKeyDetail, &detail);
			if (result == B_OK)
				fDetailView->SetText(detail.String());
			Unlock();
			break;
		}

		case kMsgProgressUpdate:
		{
			if (fCurrentState == STATE_DISPLAY_STATUS)
				_SetState(STATE_DISPLAY_PROGRESS);
			else if (fCurrentState != STATE_DISPLAY_PROGRESS)
				break;

			BString packageName;
			status_t result = message->FindString(kKeyPackageName,
				&packageName);
			if (result != B_OK)
				break;
			BString packageCount;
			result = message->FindString(kKeyPackageCount, &packageCount);
			if (result != B_OK)
				break;
			float percent;
			result = message->FindFloat(kKeyPercentage, &percent);
			if (result != B_OK)
				break;

			BString header;
			Lock();
			result = message->FindString(kKeyHeader, &header);
			if (result == B_OK && header != fHeaderView->Text())
				fHeaderView->SetText(header.String());
			fStatusBar->SetTo(percent, packageName.String(),
				packageCount.String());
			Unlock();

			fListView->UpdatePackageProgress(packageName.String(), percent);
			break;
		}

		case kMsgCancel:
		{
			if (_GetState() == STATE_FINAL_MESSAGE) {
				be_app->PostMessage(kMsgFinalQuit);
				break;
			}
			if (!fUpdateConfirmed) {
				// Downloads have not started yet, we will request to cancel
				// without confirming
				Lock();
				fHeaderView->SetText(B_TRANSLATE("Cancelling updates"));
				fDetailView->SetText(
					B_TRANSLATE("Attempting to cancel the updates"
						B_UTF8_ELLIPSIS));
				Unlock();
				fUserCancelRequested = true;

				if (fWaitingForButton) {
					fButtonResult = message->what;
					delete_sem(fWaitingSem);
					fWaitingSem = -1;
				}
				break;
			}

			// Confirm with the user to cancel
			BAlert* alert = new BAlert("cancel request", B_TRANSLATE("Updates"
				" have not been completed, are you sure you want to quit?"),
				B_TRANSLATE("Quit"), B_TRANSLATE("Don't quit"), NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go(&fCancelAlertResponse);
			break;
		}

		case kMsgCancelResponse:
		{
			// Verify whether the cancel alert was confirmed
			int32 selection = -1;
			message->FindInt32("which", &selection);
			if (selection != 0)
				break;

			Lock();
			fHeaderView->SetText(B_TRANSLATE("Cancelling updates"));
			fDetailView->SetText(
				B_TRANSLATE("Attempting to cancel the updates"
					B_UTF8_ELLIPSIS));
			Unlock();
			fUserCancelRequested = true;

			if (fWaitingForButton) {
				fButtonResult = message->what;
				delete_sem(fWaitingSem);
				fWaitingSem = -1;
			}
			break;
		}

		case kMsgUpdateConfirmed:
		{
			if (fWaitingForButton) {
				fButtonResult = message->what;
				delete_sem(fWaitingSem);
				fWaitingSem = -1;
				fUpdateConfirmed = true;
			}
			break;
		}

		case kMsgMoreDetailsToggle:
			fListView->SetMoreDetails(fDetailsCheckbox->Value() != 0);
			PostMessage(kMsgSetZoomLimits);
			_WriteSettings();
			break;

		case kMsgSetZoomLimits:
		{
			int32 count = fListView->CountItems();
			if (count < 1)
				break;
			// Convert last item's bottom point to its layout group coordinates
			BPoint zoomPoint = fListView->ZoomPoint();
			fScrollView->ConvertToParent(&zoomPoint);
			// Determine which BControl object height to use
			float controlHeight;
			if (fUpdateButtonLayoutItem->IsVisible())
				fUpdateButton->GetPreferredSize(NULL, &controlHeight);
			else
				fDetailsCheckbox->GetPreferredSize(NULL, &controlHeight);
			// Calculate height and width values
			float zoomHeight = fZoomHeightBaseline + zoomPoint.y
				+ controlHeight;
			float zoomWidth = fZoomWidthBaseline + zoomPoint.x;
			SetZoomLimits(zoomWidth, zoomHeight);
			break;
		}

		case kMsgWarningDismissed:
			fWarningAlertCount--;
			break;

		case kMsgWindowFrameChanged:
			delete fMessageRunner;
			fMessageRunner = NULL;
			_WriteSettings();
			break;

		case kMsgGetUpdateType:
		{
			BString text(
				B_TRANSLATE("Please choose from these update options:\n\n"
				"Update:\n"
				"	Updates all installed packages.\n"
				"Full sync:\n"
				"	Synchronizes the installed packages with the repositories."
				));
			BAlert* alert = new BAlert("update_type",
				text,
				B_TRANSLATE_COMMENT("Cancel", "Alert button label"),
				B_TRANSLATE_COMMENT("Full sync","Alert button label"),
				B_TRANSLATE_COMMENT("Update","Alert button label"),
				B_WIDTH_AS_USUAL, B_INFO_ALERT);
			int32 result = alert->Go();
			int32 action = INVALID_SELECTION;
			switch(result) {
				case 0:
					action = CANCEL_UPDATE;
					break;

				case 1:
					action = FULLSYNC;
					break;

				case 2:
					action = UPDATE;
					break;
			}
			BMessage reply;
			reply.AddInt32(kKeyAlertResult, action);
			message->SendReply(&reply);
			break;
		}

		case kMsgNoRepositories:
		{
			BString text(
				B_TRANSLATE_COMMENT(
				"No remote repositories are available. Please verify that some"
				" repositories are enabled using the Repositories preflet or"
				" the \'pkgman\' command.", "Error message"));
			BAlert* alert = new BAlert("repositories", text,
				B_TRANSLATE_COMMENT("Quit", "Alert button label"),
				B_TRANSLATE_COMMENT("Open Repositories","Alert button label"),
				NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			int32 result = alert->Go();
			BMessage reply;
			reply.AddInt32(kKeyAlertResult, result);
			message->SendReply(&reply);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
SoftwareUpdaterWindow::ConfirmUpdates()
{
	Lock();
	fHeaderView->SetText(B_TRANSLATE("Updates found"));
	fDetailView->SetText(B_TRANSLATE("The following changes will be made:"));
	fListView->SortItems();
	Unlock();

	uint32 priorState = _GetState();
	_SetState(STATE_GET_CONFIRMATION);

	_WaitForButtonClick();
	_SetState(priorState);
	return fButtonResult == kMsgUpdateConfirmed;
}


void
SoftwareUpdaterWindow::UpdatesApplying(const char* header, const char* detail)
{
	Lock();
	fHeaderView->SetText(header);
	fDetailView->SetText(detail);
	Unlock();
	_SetState(STATE_APPLY_UPDATES);
}


bool
SoftwareUpdaterWindow::UserCancelRequested()
{
	if (_GetState() > STATE_GET_CONFIRMATION)
		return false;

	return fUserCancelRequested;
}


void
SoftwareUpdaterWindow::AddPackageInfo(uint32 install_type,
	const char* package_name, const char* cur_ver, const char* new_ver,
	const char* summary, const char* repository, const char* file_name)
{
	Lock();
	fListView->AddPackage(install_type, package_name, cur_ver, new_ver,
		summary, repository, file_name);
	Unlock();
}


void
SoftwareUpdaterWindow::ShowWarningAlert(const char* text)
{
	BAlert* alert = new BAlert("warning", text, B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->Go(&fWarningAlertDismissed);
	alert->CenterIn(Frame());
	// Offset multiple alerts
	alert->MoveBy(fWarningAlertCount * 15, fWarningAlertCount * 15);
	fWarningAlertCount++;
}


BBitmap
SoftwareUpdaterWindow::GetIcon(int32 iconSize)
{
	BBitmap icon(BRect(0, 0, iconSize - 1, iconSize - 1), 0, B_RGBA32);
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
	BNodeInfo::GetTrackerIcon(&appInfo.ref, &icon, icon_size(iconSize));
	return icon;
}


void
SoftwareUpdaterWindow::FinalUpdate(const char* header, const char* detail)
{
	if (_GetState() == STATE_FINAL_MESSAGE)
		return;

	_SetState(STATE_FINAL_MESSAGE);
	Lock();
	fHeaderView->SetText(header);
	fDetailView->SetText(detail);
	Unlock();
}


BLayoutItem*
SoftwareUpdaterWindow::layout_item_for(BView* view)
{
	BLayout* layout = view->Parent()->GetLayout();
	int32 index = layout->IndexOfView(view);
	return layout->ItemAt(index);
}


uint32
SoftwareUpdaterWindow::_WaitForButtonClick()
{
	fButtonResult = 0;
	fWaitingForButton = true;
	fWaitingSem = create_sem(0, "WaitingSem");
	while (acquire_sem(fWaitingSem) == B_INTERRUPTED) {
	}
	fWaitingForButton = false;
	return fButtonResult;
}


void
SoftwareUpdaterWindow::_SetState(uint32 state)
{
	if (state <= STATE_HEAD || state >= STATE_MAX)
		return;

	Lock();

	// Initial settings
	if (fCurrentState == STATE_HEAD) {
		fProgressLayoutItem->SetVisible(false);
		fPackagesLayoutItem->SetVisible(false);
		fDetailsCheckboxLayoutItem->SetVisible(false);
		fCancelButtonLayoutItem->SetVisible(false);
	}
	fCurrentState = state;

	// Update confirmation button
	// Show only when asking for confirmation to update
	if (fCurrentState == STATE_GET_CONFIRMATION)
		fUpdateButtonLayoutItem->SetVisible(true);
	else
		fUpdateButtonLayoutItem->SetVisible(false);

	// View package info view and checkbox
	// Show at confirmation prompt, hide at final update
	if (fCurrentState == STATE_GET_CONFIRMATION) {
		fPackagesLayoutItem->SetVisible(true);
		fDetailsCheckboxLayoutItem->SetVisible(true);
		if (fSettingsReadStatus == B_OK) {
			bool showMoreDetails;
			status_t result = fInitialSettings.FindBool(kKeyShowDetails,
				&showMoreDetails);
			if (result == B_OK) {
				fDetailsCheckbox->SetValue(showMoreDetails ? 1 : 0);
				fListView->SetMoreDetails(showMoreDetails);
			}
		}
	} else if (fCurrentState == STATE_FINAL_MESSAGE) {
		fPackagesLayoutItem->SetVisible(false);
		fDetailsCheckboxLayoutItem->SetVisible(false);
	}

	// Progress bar and string view
	// Hide detail text while showing status bar
	if (fCurrentState == STATE_DISPLAY_PROGRESS) {
		fDetailsLayoutItem->SetVisible(false);
		fProgressLayoutItem->SetVisible(true);
	} else {
		fProgressLayoutItem->SetVisible(false);
		fDetailsLayoutItem->SetVisible(true);
	}

	// Resizing and zooming
	if (fCurrentState == STATE_GET_CONFIRMATION) {
		// Enable resizing and zooming
		float defaultWidth = fDefaultRect.Width();
		SetSizeLimits(defaultWidth, B_SIZE_UNLIMITED,
			fDefaultRect.Height() + 4 * fListView->ItemHeight(),
			B_SIZE_UNLIMITED);
		SetFlags(Flags() ^ (B_NOT_RESIZABLE | B_NOT_ZOOMABLE));
		PostMessage(kMsgSetZoomLimits);
		// Recall saved settings
		BScreen screen(this);
		BRect screenFrame = screen.Frame();
		bool windowResized = false;
		if (fSettingsReadStatus == B_OK) {
			BRect windowFrame;
			status_t result = fInitialSettings.FindRect(kKeyWindowFrame,
				&windowFrame);
			if (result == B_OK) {
				if (screenFrame.Contains(windowFrame)) {
					ResizeTo(windowFrame.Width(), windowFrame.Height());
					windowResized = true;
				}
			}
		}
		if (!windowResized)
			ResizeTo(defaultWidth, .75 * defaultWidth);
		// Check that the bottom of window is on screen
		float screenBottom = screenFrame.bottom;
		float windowBottom = DecoratorFrame().bottom;
		if (windowBottom > screenBottom)
			MoveBy(0, screenBottom - windowBottom);
		fSaveFrameChanges = true;
	} else if (fUpdateConfirmed && (fCurrentState == STATE_DISPLAY_PROGRESS
			|| fCurrentState == STATE_DISPLAY_STATUS)) {
		PostMessage(kMsgSetZoomLimits);
	} else if (fCurrentState == STATE_APPLY_UPDATES)
		fSaveFrameChanges = false;
	else if (fCurrentState == STATE_FINAL_MESSAGE) {
		// Disable resizing and zooming
		fSaveFrameChanges = false;
		ResizeTo(fDefaultRect.Width(), fDefaultRect.Height());
		SetFlags(Flags() | B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE);
	}

	// Quit button
	if (fCurrentState == STATE_FINAL_MESSAGE) {
		fCancelButtonLayoutItem->SetVisible(true);
 		fCancelButton->SetLabel(B_TRANSLATE_COMMENT("Quit", "Button label"));
 		fCancelButton->MakeDefault(true);
	}

	Unlock();
}


uint32
SoftwareUpdaterWindow::_GetState()
{
	return fCurrentState;
}


status_t
SoftwareUpdaterWindow::_WriteSettings()
{
	BFile file;
	status_t status = file.SetTo(fSettingsPath.Path(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status == B_OK) {
		BMessage settings;
		settings.AddBool(kKeyShowDetails, fDetailsCheckbox->Value() != 0);
		settings.AddRect(kKeyWindowFrame, Frame());
		status = settings.Flatten(&file);
	}
	file.Unset();
	return status;
}


status_t
SoftwareUpdaterWindow::_ReadSettings(BMessage& settings)
{
	BFile file;
	status_t status = file.SetTo(fSettingsPath.Path(), B_READ_ONLY);
	if (status == B_OK)
		status = settings.Unflatten(&file);
	file.Unset();
	return status;
}


SuperItem::SuperItem(const char* label)
	:
	BListItem(),
	fLabel(label),
	fRegularFont(be_plain_font),
	fBoldFont(be_plain_font),
	fShowMoreDetails(false),
	fPackageLessIcon(NULL),
	fPackageMoreIcon(NULL),
	fItemCount(0)
{
	fBoldFont.SetFace(B_BOLD_FACE);
	fBoldFont.GetHeight(&fBoldFontHeight);
	font_height fontHeight;
	fRegularFont.GetHeight(&fontHeight);
	fPackageItemLineHeight = fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading;
	fPackageLessIcon = _GetPackageIcon(GetPackageItemHeight(false));
	fPackageMoreIcon = _GetPackageIcon(GetPackageItemHeight(true));
}


SuperItem::~SuperItem()
{
	delete fPackageLessIcon;
	delete fPackageMoreIcon;
}


void
SuperItem::DrawItem(BView* owner, BRect item_rect, bool complete)
{
	owner->PushState();

	float width;
	owner->GetPreferredSize(&width, NULL);
	BString text(fItemText);
	owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	owner->SetFont(&fBoldFont);
	owner->TruncateString(&text, B_TRUNCATE_END, width);
	owner->DrawString(text.String(), BPoint(item_rect.left,
		item_rect.bottom - fBoldFontHeight.descent));

	owner->PopState();
}


float
SuperItem::GetPackageItemHeight()
{
	return GetPackageItemHeight(fShowMoreDetails);
}


float
SuperItem::GetPackageItemHeight(bool showMoreDetails)
{
	int lineCount = showMoreDetails ? 3 : 2;
	return lineCount * fPackageItemLineHeight;
}


BBitmap*
SuperItem::GetIcon(bool showMoreDetails)
{
	if (showMoreDetails)
		return fPackageMoreIcon;
	else
		return fPackageLessIcon;
}


float
SuperItem::GetIconSize(bool showMoreDetails)
{
	if (showMoreDetails)
		return fPackageMoreIcon->Bounds().Height();
	else
		return fPackageLessIcon->Bounds().Height();
}


void
SuperItem::SetDetailLevel(bool showMoreDetails)
{
	fShowMoreDetails = showMoreDetails;
}


void
SuperItem::SetItemCount(int32 count)
{
	fItemCount = count;
	fItemText = fLabel;
	fItemText.Append(" (");
	fItemText << fItemCount;
	fItemText.Append(")");
}


float
SuperItem::ZoomWidth(BView *owner)
{
	owner->PushState();
	owner->SetFont(&fBoldFont);
	float width = owner->StringWidth(fItemText.String());
	owner->PopState();
	return width;
}


BBitmap*
SuperItem::_GetPackageIcon(float listItemHeight)
{
	int32 iconSize = int(listItemHeight * .8);
	status_t result = B_ERROR;
	BRect iconRect(0, 0, iconSize - 1, iconSize - 1);
	BBitmap* packageIcon = new BBitmap(iconRect, 0, B_RGBA32);
	BMimeType nodeType;
	nodeType.SetTo("application/x-vnd.haiku-package");
	result = nodeType.GetIcon(packageIcon, icon_size(iconSize));
	// Get super type icon
	if (result != B_OK) {
		BMimeType superType;
		if (nodeType.GetSupertype(&superType) == B_OK)
			result = superType.GetIcon(packageIcon, icon_size(iconSize));
	}
	if (result != B_OK) {
		delete packageIcon;
		return NULL;
	}
	return packageIcon;
}


PackageItem::PackageItem(const char* name, const char* simple_version,
	const char* detailed_version, const char* repository, const char* summary,
	const char* file_name, SuperItem* super)
	:
	BListItem(),
	fName(name),
	fSimpleVersion(simple_version),
	fDetailedVersion(detailed_version),
	fRepository(repository),
	fSummary(summary),
	fSmallFont(be_plain_font),
	fSuperItem(super),
	fFileName(file_name),
	fDownloadProgress(0),
	fDrawBarFlag(false),
	fMoreDetailsWidth(0),
	fLessDetailsWidth(0)
{
	fLabelOffset = be_control_look->DefaultLabelSpacing();
	fSmallFont.SetSize(be_plain_font->Size() - 2);
	fSmallFont.GetHeight(&fSmallFontHeight);
	fSmallTotalHeight = fSmallFontHeight.ascent + fSmallFontHeight.descent
		+ fSmallFontHeight.leading;
}


void
PackageItem::DrawItem(BView* owner, BRect item_rect, bool complete)
{
	owner->PushState();

	float width = owner->Frame().Width();
	float nameWidth = width / 2.0;
	float offsetWidth = 0;
	bool showMoreDetails = fSuperItem->GetDetailLevel();

	BBitmap* icon = fSuperItem->GetIcon(showMoreDetails);
	if (icon != NULL && icon->IsValid()) {
		float iconSize = icon->Bounds().Height();
		float offsetMarginHeight = floor((Height() - iconSize) / 2);
		owner->SetDrawingMode(B_OP_ALPHA);
		BPoint location = BPoint(item_rect.left,
			item_rect.top + offsetMarginHeight);
		owner->DrawBitmap(icon, location);
		owner->SetDrawingMode(B_OP_COPY);
		offsetWidth = iconSize + fLabelOffset;

		if (fDrawBarFlag)
			_DrawBar(location, owner, icon_size(iconSize));
	}

	owner->SetFont(be_plain_font);
	owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	// Package name
	BString name(fName);
	owner->TruncateString(&name, B_TRUNCATE_END, nameWidth);
	BPoint cursor(item_rect.left + offsetWidth,
		item_rect.bottom - fSmallTotalHeight - fSmallFontHeight.descent - 2);
	if (showMoreDetails)
		cursor.y -= fSmallTotalHeight + 1;
	owner->DrawString(name.String(), cursor);
	cursor.x += owner->StringWidth(name.String()) + fLabelOffset;

	// Change font and color
	owner->SetFont(&fSmallFont);
	owner->SetHighColor(tint_color(ui_color(B_LIST_ITEM_TEXT_COLOR), 0.7));

	// Simple version or repository
	BString versionOrRepo;
	if (showMoreDetails)
		versionOrRepo.SetTo(fRepository);
	else
		versionOrRepo.SetTo(fSimpleVersion);
	owner->TruncateString(&versionOrRepo, B_TRUNCATE_END, width - cursor.x);
	owner->DrawString(versionOrRepo.String(), cursor);

	// Summary
	BString summary(fSummary);
	cursor.x = item_rect.left + offsetWidth;
	cursor.y += fSmallTotalHeight;
	owner->TruncateString(&summary, B_TRUNCATE_END, width - cursor.x);
	owner->DrawString(summary.String(), cursor);

	// Detailed version
	if (showMoreDetails) {
		BString version(fDetailedVersion);
		cursor.y += fSmallTotalHeight;
		owner->TruncateString(&version, B_TRUNCATE_END, width - cursor.x);
		owner->DrawString(version.String(), cursor);
	}

	owner->PopState();
}


// Modified slightly from Tracker's BPose::DrawBar
void
PackageItem::_DrawBar(BPoint where, BView* view, icon_size which)
{
	int32 yOffset;
	int32 size = which - 1;
	int32 barWidth = (int32)(7.0f / 32.0f * (float)which);
	if (barWidth < 4) {
		barWidth = 4;
		yOffset = 0;
	} else
		yOffset = 2;
	int32 barHeight = size - 3 - 2 * yOffset;


	// the black shadowed line
	view->SetHighColor(32, 32, 32, 92);
	view->MovePenTo(BPoint(where.x + size, where.y + 1 + yOffset));
	view->StrokeLine(BPoint(where.x + size, where.y + size - yOffset));
	view->StrokeLine(BPoint(where.x + size - barWidth + 1,
		where.y + size - yOffset));

	view->SetDrawingMode(B_OP_ALPHA);

	// the gray frame
	view->SetHighColor(76, 76, 76, 192);
	BRect rect(where.x + size - barWidth,where.y + yOffset,
		where.x + size - 1,where.y + size - 1 - yOffset);
	view->StrokeRect(rect);

	// calculate bar height
	int32 barPos = barHeight - int32(barHeight * fDownloadProgress / 100.0);
	if (barPos < 0)
		barPos = 0;
	else if (barPos > barHeight)
		barPos = barHeight;

	// the free space bar
	view->SetHighColor(255, 255, 255, 192);

	rect.InsetBy(1,1);
	BRect bar(rect);
	bar.bottom = bar.top + barPos - 1;
	if (barPos > 0)
		view->FillRect(bar);

	// the used space bar
	bar.top = bar.bottom + 1;
	bar.bottom = rect.bottom;
	view->SetHighColor(0, 203, 0, 192);
	view->FillRect(bar);
}


void
PackageItem::Update(BView *owner, const BFont *font)
{
	BListItem::Update(owner, font);
	SetHeight(fSuperItem->GetPackageItemHeight());
}


void
PackageItem::CalculateZoomWidths(BView *owner)
{
	owner->PushState();

	// More details
	float offsetWidth = 2 * be_control_look->DefaultItemSpacing()
		+ be_plain_font->Size()
		+ fSuperItem->GetIconSize(true) + fLabelOffset;
	// Name and repo
	owner->SetFont(be_plain_font);
	float stringWidth = owner->StringWidth(fName.String());
	owner->SetFont(&fSmallFont);
	stringWidth += fLabelOffset + owner->StringWidth(fRepository.String());
	// Summary
	float summaryWidth = owner->StringWidth(fSummary.String());
	if (summaryWidth > stringWidth)
		stringWidth = summaryWidth;
	// Version
	float versionWidth = owner->StringWidth(fDetailedVersion.String());
	if (versionWidth > stringWidth)
		stringWidth = versionWidth;
	fMoreDetailsWidth = offsetWidth + stringWidth;

	// Less details
	offsetWidth = 2 * be_control_look->DefaultItemSpacing()
		+ be_plain_font->Size()
		+ fSuperItem->GetIconSize(false) + fLabelOffset;
	// Name and version
	owner->SetFont(be_plain_font);
	stringWidth = owner->StringWidth(fName.String());
	owner->SetFont(&fSmallFont);
	stringWidth += fLabelOffset + owner->StringWidth(fSimpleVersion.String());
	// Summary
	if (summaryWidth > stringWidth)
		stringWidth = summaryWidth;
	fLessDetailsWidth = offsetWidth + stringWidth;

	owner->PopState();
}


int
PackageItem::NameCompare(PackageItem* item)
{
	// sort by package name
	return fName.ICompare(item->fName);
}


void
PackageItem::SetDownloadProgress(float percent)
{
	fDownloadProgress = percent;
}


int
SortPackageItems(const BListItem* item1, const BListItem* item2)
{
	PackageItem* first = (PackageItem*)item1;
	PackageItem* second = (PackageItem*)item2;
	return first->NameCompare(second);
}


PackageListView::PackageListView()
	:
	BOutlineListView("Package list"),
	fSuperUpdateItem(NULL),
	fSuperInstallItem(NULL),
	fSuperUninstallItem(NULL),
	fShowMoreDetails(false),
	fLastProgressItem(NULL),
	fLastProgressValue(-1)
{
	SetExplicitMinSize(BSize(B_SIZE_UNSET, 40));
	SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 400));
}


void
PackageListView::FrameResized(float newWidth, float newHeight)
{
	BOutlineListView::FrameResized(newWidth, newHeight);
	Invalidate();
}


void
PackageListView::ExpandOrCollapse(BListItem *superItem, bool expand)
{
	BOutlineListView::ExpandOrCollapse(superItem, expand);
	Window()->PostMessage(kMsgSetZoomLimits);
}


void
PackageListView::AddPackage(uint32 install_type, const char* name,
	const char* cur_ver, const char* new_ver, const char* summary,
	const char* repository, const char* file_name)
{
	SuperItem* super;
	BString simpleVersion;
	BString detailedVersion("");
	BString repositoryText(B_TRANSLATE_COMMENT("from repository",
		"List item text"));
	repositoryText.Append(" ").Append(repository);

	switch (install_type) {
		case PACKAGE_UPDATE:
		{
			if (fSuperUpdateItem == NULL) {
				fSuperUpdateItem = new SuperItem(B_TRANSLATE_COMMENT(
					"Packages to be updated", "List super item label"));
				AddItem(fSuperUpdateItem);
			}
			super = fSuperUpdateItem;

			simpleVersion.SetTo(new_ver);
			detailedVersion.Append(B_TRANSLATE_COMMENT("Updating version",
					"List item text"))
				.Append(" ").Append(cur_ver)
				.Append(" ").Append(B_TRANSLATE_COMMENT("to",
					"List item text"))
				.Append(" ").Append(new_ver);
			break;
		}

		case PACKAGE_INSTALL:
		{
			if (fSuperInstallItem == NULL) {
				fSuperInstallItem = new SuperItem(B_TRANSLATE_COMMENT(
					"New packages to be installed", "List super item label"));
				AddItem(fSuperInstallItem);
			}
			super = fSuperInstallItem;

			simpleVersion.SetTo(new_ver);
			detailedVersion.Append(B_TRANSLATE_COMMENT("Installing version",
					"List item text"))
				.Append(" ").Append(new_ver);
			break;
		}

		case PACKAGE_UNINSTALL:
		{
			if (fSuperUninstallItem == NULL) {
				fSuperUninstallItem = new SuperItem(B_TRANSLATE_COMMENT(
					"Packages to be uninstalled", "List super item label"));
				AddItem(fSuperUninstallItem);
			}
			super = fSuperUninstallItem;

			simpleVersion.SetTo("");
			detailedVersion.Append(B_TRANSLATE_COMMENT("Uninstalling version",
					"List item text"))
				.Append(" ").Append(cur_ver);
			break;
		}

		default:
			return;

	}
	PackageItem* item = new PackageItem(name, simpleVersion.String(),
		detailedVersion.String(), repositoryText.String(), summary, file_name,
		super);
	AddUnder(item, super);
	super->SetItemCount(CountItemsUnder(super, true));
	item->CalculateZoomWidths(this);
}


void
PackageListView::UpdatePackageProgress(const char* packageName, float percent)
{
	// Update only every 1 percent change
	int16 wholePercent = int16(percent);
	if (wholePercent == fLastProgressValue)
		return;
	fLastProgressValue = wholePercent;

	// A new package started downloading, find the PackageItem by name
	if (percent == 0) {
		fLastProgressItem = NULL;
		int32 count = FullListCountItems();
		for (int32 i = 0; i < count; i++) {
			PackageItem* item = dynamic_cast<PackageItem*>(FullListItemAt(i));
			if (item != NULL && strcmp(item->FileName(), packageName) == 0) {
				fLastProgressItem = item;
				fLastProgressItem->ShowProgressBar();
				break;
			}
		}
	}

	if (fLastProgressItem != NULL) {
		fLastProgressItem->SetDownloadProgress(percent);
		Invalidate();
	}
}


void
PackageListView::SortItems()
{
	if (fSuperUpdateItem != NULL)
		SortItemsUnder(fSuperUpdateItem, true, SortPackageItems);
	if (fSuperInstallItem != NULL)
		SortItemsUnder(fSuperInstallItem, true, SortPackageItems);
	if (fSuperUninstallItem != NULL)
		SortItemsUnder(fSuperUninstallItem, true, SortPackageItems);
}


float
PackageListView::ItemHeight()
{
	if (fSuperUpdateItem != NULL)
		return fSuperUpdateItem->GetPackageItemHeight();
	if (fSuperInstallItem != NULL)
		return fSuperInstallItem->GetPackageItemHeight();
	if (fSuperUninstallItem != NULL)
		return fSuperUninstallItem->GetPackageItemHeight();
	return 0;
}


void
PackageListView::SetMoreDetails(bool showMore)
{
	if (showMore == fShowMoreDetails)
		return;
	fShowMoreDetails = showMore;
	_SetItemHeights();
	InvalidateLayout();
	ResizeToPreferred();
}


BPoint
PackageListView::ZoomPoint()
{
	BPoint zoomPoint(0, 0);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++)
	{
		BListItem* item = ItemAt(i);
		float itemWidth = 0;
		if (item->OutlineLevel() == 0) {
			SuperItem* sItem = dynamic_cast<SuperItem*>(item);
			itemWidth = sItem->ZoomWidth(this);
		} else {
			PackageItem* pItem = dynamic_cast<PackageItem*>(item);
			itemWidth = fShowMoreDetails ? pItem->MoreDetailsWidth()
				: pItem->LessDetailsWidth();
		}
		if (itemWidth > zoomPoint.x)
			zoomPoint.x = itemWidth;
	}
	if (count > 0)
		zoomPoint.y = ItemFrame(count - 1).bottom;

	return zoomPoint;
}


void
PackageListView::_SetItemHeights()
{
	int32 itemCount = 0;
	float itemHeight = 0;
	BListItem* item = NULL;
	if (fSuperUpdateItem != NULL) {
		fSuperUpdateItem->SetDetailLevel(fShowMoreDetails);
		itemHeight = fSuperUpdateItem->GetPackageItemHeight();
		itemCount = CountItemsUnder(fSuperUpdateItem, true);
		for (int32 i = 0; i < itemCount; i++) {
			item = ItemUnderAt(fSuperUpdateItem, true, i);
			item->SetHeight(itemHeight);
		}
	}
	if (fSuperInstallItem != NULL) {
		fSuperInstallItem->SetDetailLevel(fShowMoreDetails);
		itemHeight = fSuperInstallItem->GetPackageItemHeight();
		itemCount = CountItemsUnder(fSuperInstallItem, true);
		for (int32 i = 0; i < itemCount; i++) {
			item = ItemUnderAt(fSuperInstallItem, true, i);
			item->SetHeight(itemHeight);
		}

	}
	if (fSuperUninstallItem != NULL) {
		fSuperUninstallItem->SetDetailLevel(fShowMoreDetails);
		itemHeight = fSuperUninstallItem->GetPackageItemHeight();
		itemCount = CountItemsUnder(fSuperUninstallItem, true);
		for (int32 i = 0; i < itemCount; i++) {
			item = ItemUnderAt(fSuperUninstallItem, true, i);
			item->SetHeight(itemHeight);
		}

	}
}
