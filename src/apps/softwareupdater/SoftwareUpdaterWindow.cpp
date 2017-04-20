/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@warpmail.net>
 */


#include "SoftwareUpdaterWindow.h"

#include <Alert.h>
#include <AppDefs.h>
#include <Application.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Roster.h>
#include <String.h>

#include "constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdaterWindow"


SoftwareUpdaterWindow::SoftwareUpdaterWindow()
	:
	BWindow(BRect(0, 0, 300, 100),
		B_TRANSLATE_SYSTEM_NAME("SoftwareUpdater"), B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_NOT_ZOOMABLE | B_NOT_CLOSABLE),
	fStripeView(NULL),
	fHeaderView(NULL),
	fDetailView(NULL),
	fUpdateButton(NULL),
	fCancelButton(NULL),
	fStatusBar(NULL),
	fCurrentState(STATE_HEAD),
	fWaitingSem(-1),
	fWaitingForButton(false),
	fUserCancelRequested(false),
	fWarningAlertCount(0)
{
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
				.AddGlue()
				.Add(fCancelButton)
				.Add(fUpdateButton)
			.End()
		.End()
	.End();
	
	fDetailsLayoutItem = layout_item_for(fDetailView);
	fProgressLayoutItem = layout_item_for(fStatusBar);
	fPackagesLayoutItem = layout_item_for(fScrollView);
	fUpdateButtonLayoutItem = layout_item_for(fUpdateButton);
	
	_SetState(STATE_DISPLAY_STATUS);
	CenterOnScreen();
	Show();
	SetFlags(Flags() ^ B_AUTO_UPDATE_SIZE_LIMITS);
	
	// Prevent resizing for now
	fDefaultRect = Bounds();
	SetSizeLimits(fDefaultRect.Width(), fDefaultRect.Width(),
		fDefaultRect.Height(), fDefaultRect.Height());
	
	BMessage registerMessage(kMsgRegister);
	registerMessage.AddMessenger(kKeyMessenger, BMessenger(this));
	be_app->PostMessage(&registerMessage);
	
	fCancelAlertResponse.SetMessage(new BMessage(kMsgCancelResponse));
	fCancelAlertResponse.SetTarget(this);
	fWarningAlertDismissed.SetMessage(new BMessage(kMsgWarningDismissed));
	fWarningAlertDismissed.SetTarget(this);
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
			status_t result = message->FindString(kKeyPackageName, &packageName);
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
				PostMessage(B_QUIT_REQUESTED);
				be_app->PostMessage(kMsgFinalQuit);
				break;
			}
			
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
			// Verify whether the cancel request was confirmed
			int32 selection = -1;
			message->FindInt32("which", &selection);
			if (selection != 0)
				break;
				
			Lock();
			fHeaderView->SetText(B_TRANSLATE("Cancelling updates"));
			fDetailView->SetText(
				B_TRANSLATE("Attempting to cancel the updates..."));
			fCancelButton->SetEnabled(false);
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
			}
			break;
		}

		case kMsgWarningDismissed:
			fWarningAlertCount--;
			break;
		
		case kMsgNetworkAlert:
		{
			BAlert* alert = new BAlert("network_connection",
				B_TRANSLATE_COMMENT("No active network connection was found",
					"Alert message"),
				B_TRANSLATE_COMMENT("Continue anyway", "Alert button label"),
				B_TRANSLATE_COMMENT("Quit","Alert button label"),
				NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			int32 result = alert->Go();
			BMessage reply;
			reply.AddInt32(kKeyAlertResult, result);
			message->SendReply(&reply);
			break;
		}
		
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


BBitmap
SoftwareUpdaterWindow::GetNotificationIcon()
{
	return GetIcon(B_LARGE_ICON);
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
	}
	fCurrentState = state;
	
	// Update confirmation button
	// Show only when asking for confirmation to update
	if (fCurrentState == STATE_GET_CONFIRMATION) 
		fUpdateButtonLayoutItem->SetVisible(true);
	else
		fUpdateButtonLayoutItem->SetVisible(false);
	
	// View package info view
	// Show at confirmation prompt, hide at final update
	if (fCurrentState == STATE_GET_CONFIRMATION) {
		fPackagesLayoutItem->SetVisible(true);
		// Re-enable resizing
		float defaultWidth = fDefaultRect.Width();
		SetSizeLimits(defaultWidth, 9999,
			fDefaultRect.Height() + 4 * fListView->ItemHeight(), 9999);
		ResizeTo(defaultWidth, .75 * defaultWidth);
	} else if (fCurrentState == STATE_FINAL_MESSAGE) {
		fPackagesLayoutItem->SetVisible(false);
		float defaultWidth = fDefaultRect.Width();
		float defaultHeight = fDefaultRect.Height();
		SetSizeLimits(defaultWidth, defaultWidth, defaultHeight,
			defaultHeight);
		ResizeTo(defaultWidth, defaultHeight);
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
	
	// Cancel button
	if (fCurrentState == STATE_FINAL_MESSAGE)
 		fCancelButton->SetLabel(B_TRANSLATE("Quit"));
	fCancelButton->SetEnabled(fCurrentState != STATE_APPLY_UPDATES);
	
	Unlock();
}


uint32
SoftwareUpdaterWindow::_GetState()
{
	return fCurrentState;
}


SuperItem::SuperItem(const char* label)
	:
	BListItem(),
	fLabel(label),
	fShowMoreDetails(false),
	fPackageIcon(NULL),
	fItemCount(0)
{
}


SuperItem::~SuperItem()
{
	delete fPackageIcon;
}


void
SuperItem::DrawItem(BView* owner, BRect item_rect, bool complete)
{
	owner->PushState();
	
	float width;
    owner->GetPreferredSize(&width, NULL);
    BString label(fLabel);
    label.Append(" (");
    label << fItemCount;
    label.Append(")");
    owner->TruncateString(&label, B_TRUNCATE_END, width);
    owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
    owner->SetFont(&fBoldFont);
    owner->DrawString(label.String(), BPoint(item_rect.left,
		item_rect.bottom - fFontHeight.descent - 1));
	
	owner->PopState();
}


void
SuperItem::Update(BView *owner, const BFont *font)
{
	fRegularFont = *font;
	fBoldFont = *font;
	fBoldFont.SetFace(B_BOLD_FACE);
	BListItem::Update(owner, &fBoldFont);
	_SetHeights();
}


void
SuperItem::SetDetailLevel(bool showMoreDetails)
{
	fShowMoreDetails = showMoreDetails;
	_SetHeights();
}


void
SuperItem::_SetHeights()
{
	// Calculate height for PackageItem
	fRegularFont.GetHeight(&fFontHeight);
	int lineCount = fShowMoreDetails ? 3 : 2;
	fPackageItemHeight = lineCount * (fFontHeight.ascent + fFontHeight.descent
		+ fFontHeight.leading);
	
	// Calculate height for this item
	fBoldFont.GetHeight(&fFontHeight);
	SetHeight(fFontHeight.ascent + fFontHeight.descent
		+ fFontHeight.leading + 4);
	
	_GetPackageIcon();
}


void
SuperItem::_GetPackageIcon()
{
	delete fPackageIcon;
	fIconSize = int(fPackageItemHeight * .8);

	status_t result = B_ERROR;
	BRect iconRect(0, 0, fIconSize - 1, fIconSize - 1);
	fPackageIcon = new BBitmap(iconRect, 0, B_RGBA32);
	BMimeType nodeType;
	nodeType.SetTo("application/x-vnd.haiku-package");
	result = nodeType.GetIcon(fPackageIcon, icon_size(fIconSize));
	// Get super type icon
	if (result != B_OK) {
		BMimeType superType;
		if (nodeType.GetSupertype(&superType) == B_OK)
			result = superType.GetIcon(fPackageIcon, icon_size(fIconSize));
	}
	if (result != B_OK) {
		delete fPackageIcon;
		fPackageIcon = NULL;
	}
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
	fSuperItem(super),
	fFileName(file_name),
	fDownloadProgress(0),
	fDrawBarFlag(false)
{
	fLabelOffset = be_control_look->DefaultLabelSpacing();
}


void
PackageItem::DrawItem(BView* owner, BRect item_rect, bool complete)
{
	owner->PushState();
	
	float width;
    owner->GetPreferredSize(&width, NULL);
    float nameWidth = width / 2.0;
    float offset_width = 0;
    bool showMoreDetails = fSuperItem->GetDetailLevel();
	
	BBitmap* icon = fSuperItem->GetIcon();
	if (icon != NULL && icon->IsValid()) {
		int16 iconSize = fSuperItem->GetIconSize();
		float offsetMarginHeight = floor((Height() - iconSize) / 2);

		//owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->SetDrawingMode(B_OP_ALPHA);
		BPoint location = BPoint(item_rect.left,
			item_rect.top + offsetMarginHeight);
		owner->DrawBitmap(icon, location);
		owner->SetDrawingMode(B_OP_COPY);
		
		if (fDrawBarFlag)
			_DrawBar(location, owner, icon_size(iconSize));
		
		offset_width += iconSize + fLabelOffset;
	}
	
	owner->SetFont(&fRegularFont);
	owner->SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));
	
	// Package name
	font_height fontHeight = fSuperItem->GetFontHeight();
    BString name(fName);
    owner->TruncateString(&name, B_TRUNCATE_END, nameWidth);
	BPoint cursor(item_rect.left + offset_width,
		item_rect.bottom - fSmallTotalHeight - fontHeight.descent - 1);
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
	cursor.x = item_rect.left + offset_width;
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
	SetItemHeight(font);
}


void
PackageItem::SetItemHeight(const BFont* font)
{
	SetHeight(fSuperItem->GetPackageItemHeight());
	
	fRegularFont = *font;
	fSmallFont = *font;
	fSmallFont.SetSize(font->Size() - 2);
	fSmallFont.GetHeight(&fSmallFontHeight);
	fSmallTotalHeight = fSmallFontHeight.ascent + fSmallFontHeight.descent
		+ fSmallFontHeight.leading;
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
	fMenu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	fDetailMenuItem = new BMenuItem("Show more details", new BMessage());
	fMenu->AddItem(fDetailMenuItem);
	
	SetExplicitMinSize(BSize(B_SIZE_UNSET, 40));
	SetExplicitPreferredSize(BSize(B_SIZE_UNSET, 400));
}


void
PackageListView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();
	fDetailMenuItem->SetTarget(this);
}


void
PackageListView::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case kMsgMoreDetailsOff:
		case kMsgMoreDetailsOn:
		{
			fShowMoreDetails = message->what == kMsgMoreDetailsOn;
			_SetItemHeights();
			InvalidateLayout();
			ResizeToPreferred();
			break;
		}
		
		default:
			BOutlineListView::MessageReceived(message);
	}
}


void
PackageListView::FrameResized(float newWidth, float newHeight)
{
	BOutlineListView::FrameResized(newWidth, newHeight);
	
	float count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem *item = ItemAt(i);
		item->Update(this, be_plain_font);
	}
	Invalidate();
}


void
PackageListView::MouseDown(BPoint where)
{
	BOutlineListView::MouseDown(where);
	
	int32 button;
	Looper()->CurrentMessage()->FindInt32("buttons", &button);
	if (button & B_SECONDARY_MOUSE_BUTTON) {
		if (fShowMoreDetails) {
			fDetailMenuItem->SetMarked(true);
			fDetailMenuItem->Message()->what = kMsgMoreDetailsOff;
		} else {
			fDetailMenuItem->SetMarked(false);
			fDetailMenuItem->Message()->what = kMsgMoreDetailsOn;
		}
		// Offset point so cursor isn't over checkmark
		ConvertToScreen(&where);
		where.x -= 10;
		where.y -= 10;
		fMenu->Go(where, true, true, BRect(where.x - 2, where.y - 2,
			where.x + 2, where.y + 2), true);
	}
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
