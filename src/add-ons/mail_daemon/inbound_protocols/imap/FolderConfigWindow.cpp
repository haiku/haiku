/*
 * Copyright 2011-2013, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "FolderConfigWindow.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>

#include <StringForSize.h>

#include "Settings.h"
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IMAPFolderConfig"


class EditableListItem {
public:
								EditableListItem();
	virtual						~EditableListItem() {}

	virtual void				MouseDown(BPoint where) = 0;
	virtual	void				MouseUp(BPoint where) = 0;

			void				SetListView(BListView* list)
									{ fListView = list; }

protected:
			BListView*			fListView;
};


class CheckBoxItem : public BStringItem, public EditableListItem {
public:
								CheckBoxItem(const char* text, bool checked);

			void				DrawItem(BView* owner, BRect itemRect,
									bool drawEverything = false);

			void				MouseDown(BPoint where);
			void				MouseUp(BPoint where);

			bool				Checked() { return fChecked; }
private:
			BRect				fBoxRect;
			bool				fChecked;
			bool				fMouseDown;
};


class EditListView : public BListView {
public:
								EditListView(const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
										| B_NAVIGABLE);

	virtual void				MouseDown(BPoint where);
	virtual void				MouseUp(BPoint where);
	virtual void				FrameResized(float newWidth, float newHeight);

private:
			EditableListItem*	fLastMouseDown;
};


class StatusWindow : public BWindow {
public:
	StatusWindow(BWindow* parent, const char* text)
		:
		BWindow(BRect(0, 0, 10, 10), B_TRANSLATE("status"), B_MODAL_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NO_WORKSPACE_ACTIVATION | B_NOT_ZOOMABLE
				| B_AVOID_FRONT | B_NOT_RESIZABLE | B_NOT_ZOOMABLE
				| B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
	{
		BLayoutBuilder::Group<>(this)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(new BStringView("text", text));
		CenterIn(parent->Frame());
	}
};


const uint32 kMsgApplyButton = '&Abu';
const uint32 kMsgInit = '&Ini';


// #pragma mark -


EditableListItem::EditableListItem()
	:
	fListView(NULL)
{

}


// #pragma mark -


CheckBoxItem::CheckBoxItem(const char* text, bool checked)
	:
	BStringItem(text),
	fChecked(checked),
	fMouseDown(false)
{
}


void
CheckBoxItem::DrawItem(BView* owner, BRect itemRect, bool drawEverything)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lowColor = owner->LowColor();
	uint32 flags = 0;
	if (fMouseDown)
		flags |= BControlLook::B_CLICKED;
	if (fChecked)
		flags |= BControlLook::B_ACTIVATED;

	font_height fontHeight;
	owner->GetFontHeight(&fontHeight);
	BRect boxRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));

	owner->PushState();

	float left = itemRect.left;
	fBoxRect.left = left + 3;
	fBoxRect.top = itemRect.top + (itemRect.Height() - boxRect.Height()) / 2;
	fBoxRect.right = fBoxRect.left + boxRect.Width();
	fBoxRect.bottom = itemRect.top + boxRect.Height();

	itemRect.left = fBoxRect.right + be_control_look->DefaultLabelSpacing();

	if (IsSelected() || drawEverything) {
		if (IsSelected()) {
			owner->SetHighColor(tint_color(lowColor, B_DARKEN_2_TINT));
			owner->SetLowColor(owner->HighColor());
		} else
			owner->SetHighColor(lowColor);

		owner->FillRect(
			BRect(left, itemRect.top, itemRect.left, itemRect.bottom));
	}

	be_control_look->DrawCheckBox(owner, fBoxRect, fBoxRect, base, flags);
	owner->PopState();

	BStringItem::DrawItem(owner, itemRect, drawEverything);
}


void
CheckBoxItem::MouseDown(BPoint where)
{
	if (!fBoxRect.Contains(where))
		return;

	fMouseDown = true;

	fListView->InvalidateItem(fListView->IndexOf(this));
}


void
CheckBoxItem::MouseUp(BPoint where)
{
	if (!fMouseDown)
		return;
	fMouseDown = false;

	if (fBoxRect.Contains(where)) {
		if (fChecked)
			fChecked = false;
		else
			fChecked = true;
	}

	fListView->InvalidateItem(fListView->IndexOf(this));
}


// #pragma mark -


EditListView::EditListView(const char* name, list_view_type type, uint32 flags)
	:
	BListView(name, type, flags),
	fLastMouseDown(NULL)
{
}


void
EditListView::MouseDown(BPoint where)
{
	BListView::MouseDown(where);

	int32 index = IndexOf(where);
	EditableListItem* handler = dynamic_cast<EditableListItem*>(ItemAt(index));
	if (handler == NULL)
		return;

	fLastMouseDown = handler;
	handler->MouseDown(where);
	SetMouseEventMask(B_POINTER_EVENTS);
}


void
EditListView::MouseUp(BPoint where)
{
	BListView::MouseUp(where);
	if (fLastMouseDown) {
		fLastMouseDown->MouseUp(where);
		fLastMouseDown = NULL;
	}

	int32 index = IndexOf(where);
	EditableListItem* handler = dynamic_cast<EditableListItem*>(ItemAt(index));
	if (handler == NULL)
		return;

	handler->MouseUp(where);
}


void
EditListView::FrameResized(float newWidth, float newHeight)
{
	Invalidate();
}


// #pragma mark -


FolderConfigWindow::FolderConfigWindow(BRect parent, const BMessage& settings)
	:
	BWindow(BRect(0, 0, 350, 350), B_TRANSLATE("IMAP Folders"),
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fSettings("in", settings)
{
	fQuotaView = new BStringView("quota view",
		B_TRANSLATE("Failed to fetch available storage."));
	fQuotaView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));
	fQuotaView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fFolderListView = new EditListView(B_TRANSLATE("IMAP Folders"));
	fFolderListView->SetExplicitPreferredSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNLIMITED));

	BButton* cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(B_QUIT_REQUESTED));
	BButton* applyButton = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(kMsgApplyButton));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fQuotaView)
		.Add(new BScrollView("scroller", fFolderListView, 0, false, true))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)
			.Add(applyButton);

	PostMessage(kMsgInit);
	CenterIn(parent);
}


void
FolderConfigWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgInit:
			_LoadFolders();
			break;

		case kMsgApplyButton:
			_ApplyChanges();
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
FolderConfigWindow::_LoadFolders()
{
	StatusWindow* statusWindow = new StatusWindow(this,
		B_TRANSLATE("Fetching IMAP folders, have patience" B_UTF8_ELLIPSIS));
	statusWindow->Show();

	status_t status = fProtocol.Connect(fSettings.ServerAddress(),
		fSettings.Username(), fSettings.Password(), fSettings.UseSSL());
	if (status != B_OK) {
		statusWindow->PostMessage(B_QUIT_REQUESTED);

		// Show error message on screen
		BString message = B_TRANSLATE("Could not connect to server "
			"\"%server%\":\n%error%");
		message.ReplaceFirst("%server%", fSettings.Server());
		message.ReplaceFirst("%error%", strerror(status));
		BAlert* alert = new BAlert("IMAP error", message.String(),
			B_TRANSLATE("Ok"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();

		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	// TODO: don't get all of them at once, but retrieve them level by level
	fFolderList.clear();
	BString separator;
	fProtocol.GetFolders(fFolderList, separator);
	for (size_t i = 0; i < fFolderList.size(); i++) {
		IMAP::FolderEntry& entry = fFolderList[i];
		CheckBoxItem* item = new CheckBoxItem(
			MailboxToFolderName(entry.folder, separator), entry.subscribed);
		fFolderListView->AddItem(item);
		item->SetListView(fFolderListView);
	}

	uint64 used, total;
	if (fProtocol.GetQuota(used, total) == B_OK) {
		char buffer[256];
		BString quotaString = "Server storage: ";
		quotaString += string_for_size(used, buffer, 256);
		quotaString += " / ";
		quotaString += string_for_size(total, buffer, 256);
		quotaString += " used.";
		fQuotaView->SetText(quotaString);
	}

	statusWindow->PostMessage(B_QUIT_REQUESTED);
}


void
FolderConfigWindow::_ApplyChanges()
{
	bool haveChanges = false;
	for (size_t i = 0; i < fFolderList.size(); i++) {
		IMAP::FolderEntry& entry = fFolderList[i];
		CheckBoxItem* item = (CheckBoxItem*)fFolderListView->ItemAt(i);
		if (entry.subscribed != item->Checked()) {
			haveChanges = true;
			break;
		}
	}
	if (!haveChanges)
		return;

	StatusWindow* status = new StatusWindow(this,
		B_TRANSLATE("Update subcription of IMAP folders, have patience"
		B_UTF8_ELLIPSIS));
	status->Show();

	for (size_t i = 0; i < fFolderList.size(); i++) {
		IMAP::FolderEntry& entry = fFolderList[i];
		CheckBoxItem* item = (CheckBoxItem*)fFolderListView->ItemAt(i);
		if (entry.subscribed && !item->Checked())
			fProtocol.UnsubscribeFolder(entry.folder);
		else if (!entry.subscribed && item->Checked())
			fProtocol.SubscribeFolder(entry.folder);
	}

	status->PostMessage(B_QUIT_REQUESTED);
}

