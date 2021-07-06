/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "DownloadWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <UrlContext.h>

#include "BrowserApp.h"
#include "BrowserWindow.h"
#include "DownloadProgressView.h"
#include "SettingsKeys.h"
#include "SettingsMessage.h"
#include "WebDownload.h"
#include "WebPage.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Download Window"

enum {
	INIT = 'init',
	OPEN_DOWNLOADS_FOLDER = 'odnf',
	REMOVE_FINISHED_DOWNLOADS = 'rmfd',
	REMOVE_MISSING_DOWNLOADS = 'rmmd'
};


class DownloadsContainerView : public BGroupView {
public:
	DownloadsContainerView()
		:
		BGroupView(B_VERTICAL, 0.0)
	{
		SetFlags(Flags() | B_PULSE_NEEDED);
		SetViewColor(245, 245, 245);
		AddChild(BSpaceLayoutItem::CreateGlue());
	}

	virtual BSize MinSize()
	{
		BSize minSize = BGroupView::MinSize();
		return BSize(minSize.width, 80);
	}

	virtual void Pulse()
	{
		DownloadProgressView::SpeedVersusEstimatedFinishTogglePulse();
	}

protected:
	virtual void DoLayout()
	{
		BGroupView::DoLayout();
		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			BSize minSize = BGroupView::MinSize();
			float height = Bounds().Height();
			float max = minSize.height - height;
			scrollBar->SetRange(0, max);
			if (minSize.height > 0)
				scrollBar->SetProportion(height / minSize.height);
			else
				scrollBar->SetProportion(1);
		}
	}
};


class DownloadContainerScrollView : public BScrollView {
public:
	DownloadContainerScrollView(BView* target)
		:
		BScrollView("Downloads scroll view", target, 0, false, true,
			B_NO_BORDER)
	{
	}

protected:
	virtual void DoLayout()
	{
		BScrollView::DoLayout();
		// Tweak scroll bar layout to hide part of the frame for better looks.
		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		scrollBar->MoveBy(1, -1);
		scrollBar->ResizeBy(0, 2);
		Target()->ResizeBy(1, 0);
		// Set the scroll steps
		if (BView* item = Target()->ChildAt(0)) {
			scrollBar->SetSteps(item->MinSize().height + 1,
				item->MinSize().height + 1);
		}
	}
};


// #pragma mark -


DownloadWindow::DownloadWindow(BRect frame, bool visible,
		SettingsMessage* settings)
	: BWindow(frame, B_TRANSLATE("Downloads"),
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),
	fMinimizeOnClose(false)
{
	SetPulseRate(1000000);

	settings->AddListener(BMessenger(this));
	BPath downloadPath;
	if (find_directory(B_DESKTOP_DIRECTORY, &downloadPath) != B_OK)
		downloadPath.SetTo("/boot/home/Desktop");
	fDownloadPath = settings->GetValue(kSettingsKeyDownloadPath,
		downloadPath.Path());
	settings->SetValue(kSettingsKeyDownloadPath, fDownloadPath);

	SetLayout(new BGroupLayout(B_VERTICAL, 0.0));

	DownloadsContainerView* downloadsGroupView = new DownloadsContainerView();
	fDownloadViewsLayout = downloadsGroupView->GroupLayout();

	BMenuBar* menuBar = new BMenuBar("Menu bar");
	BMenu* menu = new BMenu(B_TRANSLATE("Downloads"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open downloads folder"),
		new BMessage(OPEN_DOWNLOADS_FOLDER)));
	BMessage* newWindowMessage = new BMessage(NEW_WINDOW);
	newWindowMessage->AddString("url", "");
	BMenuItem* newWindowItem = new BMenuItem(B_TRANSLATE("New browser window"),
		newWindowMessage, 'N');
	menu->AddItem(newWindowItem);
	newWindowItem->SetTarget(be_app);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'D'));
	menuBar->AddItem(menu);

	fDownloadsScrollView = new DownloadContainerScrollView(downloadsGroupView);

	fRemoveFinishedButton = new BButton(B_TRANSLATE("Remove finished"),
		new BMessage(REMOVE_FINISHED_DOWNLOADS));
	fRemoveFinishedButton->SetEnabled(false);

	fRemoveMissingButton = new BButton(B_TRANSLATE("Remove missing"),
		new BMessage(REMOVE_MISSING_DOWNLOADS));
	fRemoveMissingButton->SetEnabled(false);

	const float spacing = be_control_look->DefaultItemSpacing();

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0.0)
		.Add(menuBar)
		.Add(fDownloadsScrollView)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fRemoveMissingButton)
			.Add(fRemoveFinishedButton)
			.SetInsets(12, 5, 12, 5)
		)
	);

	PostMessage(INIT);

	if (!visible)
		Hide();
	Show();
	MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
}


DownloadWindow::~DownloadWindow()
{
	// Only necessary to save the current progress of unfinished downloads:
	_SaveSettings();
}


void
DownloadWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	// We need to intercept mouse down events inside the area of download
	// progress views (regardless of whether they have children at the click),
	// so that they may display a context menu.
	BPoint where;
	int32 buttons;
	if (message->what == B_MOUSE_DOWN
		&& message->FindPoint("screen_where", &where) == B_OK
		&& message->FindInt32("buttons", &buttons) == B_OK
		&& (buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		for (int32 i = fDownloadViewsLayout->CountItems() - 1;
				BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
			DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
				item->View());
			if (!view)
				continue;
			BPoint viewWhere(where);
			view->ConvertFromScreen(&viewWhere);
			if (view->Bounds().Contains(viewWhere)) {
				view->ShowContextMenu(where);
				return;
			}
		}
	}
	BWindow::DispatchMessage(message, target);
}


void
DownloadWindow::FrameResized(float newWidth, float newHeight)
{
	MoveOnScreen(B_DO_NOT_RESIZE_TO_FIT | B_MOVE_IF_PARTIALLY_OFFSCREEN);
}


void
DownloadWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case INIT:
		{
			_LoadSettings();
			// Small trick to get the correct enabled status of the Remove
			// finished button
			_DownloadFinished(NULL);
			break;
		}
		case B_DOWNLOAD_ADDED:
		{
			BWebDownload* download;
			if (message->FindPointer("download", reinterpret_cast<void**>(
					&download)) == B_OK) {
				_DownloadStarted(download);
			}
			break;
		}
		case B_DOWNLOAD_REMOVED:
		{
			BWebDownload* download;
			if (message->FindPointer("download", reinterpret_cast<void**>(
					&download)) == B_OK) {
				_DownloadFinished(download);
			}
			break;
		}
		case OPEN_DOWNLOADS_FOLDER:
		{
			entry_ref ref;
			status_t status = get_ref_for_path(fDownloadPath.String(), &ref);
			if (status == B_OK)
				status = be_roster->Launch(&ref);
			if (status != B_OK && status != B_ALREADY_RUNNING) {
				BString errorString(B_TRANSLATE_COMMENT("The downloads folder could "
					"not be opened.\n\nError: %error", "Don't translate "
					"variable %error"));
				errorString.ReplaceFirst("%error", strerror(status));
				BAlert* alert = new BAlert(B_TRANSLATE("Error opening downloads "
					"folder"), errorString.String(), B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go(NULL);
			}
			break;
		}
		case REMOVE_FINISHED_DOWNLOADS:
			_RemoveFinishedDownloads();
			break;
		case REMOVE_MISSING_DOWNLOADS:
			_RemoveMissingDownloads();
			break;
		case SAVE_SETTINGS:
			_ValidateButtonStatus();
			_SaveSettings();
			break;

		case SETTINGS_VALUE_CHANGED:
		{
			BString string;
			if (message->FindString("name", &string) == B_OK
				&& string == kSettingsKeyDownloadPath
				&& message->FindString("value", &string) == B_OK) {
				fDownloadPath = string;
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
DownloadWindow::QuitRequested()
{
	if (fMinimizeOnClose) {
		if (!IsMinimized())
			Minimize(true);
	} else {
		if (!IsHidden())
			Hide();
	}
	return false;
}


bool
DownloadWindow::DownloadsInProgress()
{
	bool downloadsInProgress = false;
	if (!Lock())
		return downloadsInProgress;

	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->Download() != NULL) {
			downloadsInProgress = true;
			break;
		}
	}

	Unlock();

	return downloadsInProgress;
}


void
DownloadWindow::SetMinimizeOnClose(bool minimize)
{
	if (Lock()) {
		fMinimizeOnClose = minimize;
		Unlock();
	}
}


// #pragma mark - private


void
DownloadWindow::_DownloadStarted(BWebDownload* download)
{
	download->Start(BPath(fDownloadPath.String()));

	int32 finishedCount = 0;
	int32 missingCount = 0;
	int32 index = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->URL() == download->URL()) {
			index = i;
			view->RemoveSelf();
			delete view;
			continue;
		}
		if (view->IsFinished())
			finishedCount++;
		if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
	DownloadProgressView* view = new DownloadProgressView(download);
	if (!view->Init()) {
		delete view;
		return;
	}
	fDownloadViewsLayout->AddView(index, view);

	// Scroll new download into view
	if (BScrollBar* scrollBar = fDownloadsScrollView->ScrollBar(B_VERTICAL)) {
		float min;
		float max;
		scrollBar->GetRange(&min, &max);
		float viewHeight = view->MinSize().height + 1;
		float scrollOffset = min + index * viewHeight;
		float scrollBarHeight = scrollBar->Bounds().Height() - 1;
		float value = scrollBar->Value();
		if (scrollOffset < value)
			scrollBar->SetValue(scrollOffset);
		else if (scrollOffset + viewHeight > value + scrollBarHeight) {
			float diff = scrollOffset + viewHeight - (value + scrollBarHeight);
			scrollBar->SetValue(value + diff);
		}
	}

	_SaveSettings();

	SetWorkspaces(B_CURRENT_WORKSPACE);
	if (IsHidden())
		Show();

	Activate(true);
}


void
DownloadWindow::_DownloadFinished(BWebDownload* download)
{
	int32 finishedCount = 0;
	int32 missingCount = 0;
	for (int32 i = 0;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i++) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (download && view->Download() == download) {
			view->DownloadFinished();
			finishedCount++;
			continue;
		}
		if (view->IsFinished())
			finishedCount++;
		if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
	if (download)
		_SaveSettings();
}


void
DownloadWindow::_RemoveFinishedDownloads()
{
	int32 missingCount = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->IsFinished()) {
			view->RemoveSelf();
			delete view;
		} else if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(false);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
	_SaveSettings();
}


void
DownloadWindow::_RemoveMissingDownloads()
{
	int32 finishedCount = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->IsMissing()) {
			view->RemoveSelf();
			delete view;
		} else if (view->IsFinished())
			finishedCount++;
	}
	fRemoveMissingButton->SetEnabled(false);
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	_SaveSettings();
}


void
DownloadWindow::_ValidateButtonStatus()
{
	int32 finishedCount = 0;
	int32 missingCount = 0;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;
		if (view->IsFinished())
			finishedCount++;
		if (view->IsMissing())
			missingCount++;
	}
	fRemoveFinishedButton->SetEnabled(finishedCount > 0);
	fRemoveMissingButton->SetEnabled(missingCount > 0);
}


void
DownloadWindow::_SaveSettings()
{
	BFile file;
	if (!_OpenSettingsFile(file, B_ERASE_FILE | B_CREATE_FILE | B_WRITE_ONLY))
		return;
	BMessage message;
	for (int32 i = fDownloadViewsLayout->CountItems() - 1;
			BLayoutItem* item = fDownloadViewsLayout->ItemAt(i); i--) {
		DownloadProgressView* view = dynamic_cast<DownloadProgressView*>(
			item->View());
		if (!view)
			continue;

	BMessage downloadArchive;
		if (view->SaveSettings(&downloadArchive) == B_OK)
			message.AddMessage("download", &downloadArchive);
	}
	message.Flatten(&file);
}


void
DownloadWindow::_LoadSettings()
{
	BFile file;
	if (!_OpenSettingsFile(file, B_READ_ONLY))
		return;
	BMessage message;
	if (message.Unflatten(&file) != B_OK)
		return;
	BMessage downloadArchive;
	for (int32 i = 0;
			message.FindMessage("download", i, &downloadArchive) == B_OK;
			i++) {
		DownloadProgressView* view = new DownloadProgressView(
			&downloadArchive);
		if (!view->Init(&downloadArchive))
			continue;
		fDownloadViewsLayout->AddView(0, view);
	}
}


bool
DownloadWindow::_OpenSettingsFile(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kApplicationName) != B_OK
		|| path.Append("Downloads") != B_OK) {
		return false;
	}
	return file.SetTo(path.Path(), mode) == B_OK;
}


