/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolderConfig.h"

#include <Catalog.h>
#include <ControlLook.h>
#include <ListItem.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>

#include <ALMLayout.h>
#include <ALMGroup.h>
#include <StringForSize.h>

#include <crypt.h>


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
	StatusWindow(const char* text)
		:
		BWindow(BRect(0, 0, 10, 10), B_TRANSLATE("status"), B_MODAL_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NO_WORKSPACE_ACTIVATION | B_NOT_ZOOMABLE
				| B_AVOID_FRONT | B_NOT_RESIZABLE)
	{
		BView* rootView = new BView(Bounds(), "root", B_FOLLOW_ALL,
			B_WILL_DRAW);
		AddChild(rootView);
		rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BALMLayout* layout = new BALMLayout(B_USE_ITEM_SPACING,
				B_USE_ITEM_SPACING);
		rootView->SetLayout(layout);
		layout->SetInsets(B_USE_WINDOW_SPACING);

		BStringView* string = new BStringView("text", text);
		layout->AddView(string, layout->Left(), layout->Top(), layout->Right(),
			layout->Bottom());
		BSize min = layout->MinSize();
		ResizeTo(min.Width(), min.Height());
		CenterOnScreen();
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
	BStringItem::DrawItem(owner, itemRect, drawEverything);

	if (!be_control_look)
		return;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (fMouseDown)
		flags |= BControlLook::B_CLICKED;
	if (fChecked)
		flags |= BControlLook::B_ACTIVATED;
	font_height fontHeight;
	owner->GetFontHeight(&fontHeight);
	BRect boxRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));

	fBoxRect.left = itemRect.right - boxRect.Width();
	fBoxRect.top = itemRect.top + (itemRect.Height() - boxRect.Height()) / 2;
	fBoxRect.right = itemRect.right;
	fBoxRect.bottom = itemRect.top + boxRect.Height();

	be_control_look->DrawCheckBox(owner, fBoxRect, fBoxRect, base, flags);
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
	BWindow(BRect(0, 0, 300, 300), B_TRANSLATE("IMAP Folders"),
		B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NO_WORKSPACE_ACTIVATION | B_NOT_ZOOMABLE | B_AVOID_FRONT),
	fSettings(settings)
{
	BView* rootView = new BView(Bounds(), "root", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BALMLayout* layout = new BALMLayout(B_USE_ITEM_SPACING, B_USE_ITEM_SPACING);
	rootView->SetLayout(layout);
	layout->SetInsets(B_USE_WINDOW_INSETS);

	fFolderListView = new EditListView(B_TRANSLATE("IMAP Folders"));
	fFolderListView->SetExplicitPreferredSize(BSize(B_SIZE_UNLIMITED,
		B_SIZE_UNLIMITED));
	fApplyButton = new BButton("Apply", B_TRANSLATE("Apply"),
		new BMessage(kMsgApplyButton));

	fQuotaView = new BStringView("quota view",
		B_TRANSLATE("Failed to fetch available storage."));
	fQuotaView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_VERTICAL_CENTER));

	(ALMGroup(fQuotaView) / ALMGroup(fFolderListView)
		/ (ALMGroup(BSpaceLayoutItem::CreateGlue())
			| ALMGroup(fApplyButton))).BuildLayout(layout);

	PostMessage(kMsgInit);

	BSize min = layout->MinSize();
	BSize max = layout->MaxSize();
	SetSizeLimits(min.Width(), max.Width(), min.Height(), max.Height());

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
	StatusWindow* status = new StatusWindow(
		B_TRANSLATE("Fetching IMAP folders, have patience..."));
	status->Show();

	BString server;
	fSettings.FindString("server", &server);
	int32 ssl;
	fSettings.FindInt32("flavor", &ssl);
	bool useSSL = false;
	if (ssl == 1)
		useSSL = true;

	BString username;
	fSettings.FindString("username", &username);

	BString password;
	char* passwd = get_passwd(&fSettings, "cpasswd");
	if (passwd != NULL) {
		password = passwd;
		delete[] passwd;
	}

	fIMAPFolders.Connect(server, username, password, useSSL);
	fFolderList.clear();
	fIMAPFolders.GetFolders(fFolderList);
	for (unsigned int i = 0; i < fFolderList.size(); i++) {
		FolderInfo& info = fFolderList[i];
		CheckBoxItem* item = new CheckBoxItem(info.folder, info.subscribed);
		fFolderListView->AddItem(item);
		item->SetListView(fFolderListView);
	}

	uint64 used, total;
	if (fIMAPFolders.GetQuota(used, total) == B_OK) {
		char buffer[256];
		BString quotaString = "Server storage: ";
		quotaString += string_for_size(used, buffer, 256);
		quotaString += " / ";
		quotaString += string_for_size(total, buffer, 256);
		quotaString += " used.";
		fQuotaView->SetText(quotaString);
	}

	status->PostMessage(B_QUIT_REQUESTED);
}


void
FolderConfigWindow::_ApplyChanges()
{
	bool haveChanges = false;
	for (unsigned int i = 0; i < fFolderList.size(); i++) {
		FolderInfo& info = fFolderList[i];
		CheckBoxItem* item = (CheckBoxItem*)fFolderListView->ItemAt(i);
		if ((info.subscribed != item->Checked())) {
			haveChanges = true;
			break;
		}
	}
	if (!haveChanges)
		return;

	StatusWindow* status = new StatusWindow(B_TRANSLATE("Subcribe / Unsuscribe "
		"IMAP folders, have patience..."));
	status->Show();

	for (unsigned int i = 0; i < fFolderList.size(); i++) {
		FolderInfo& info = fFolderList[i];
		CheckBoxItem* item = (CheckBoxItem*)fFolderListView->ItemAt(i);
		if (info.subscribed && !item->Checked())
			fIMAPFolders.UnsubscribeFolder(info.folder);
		else if (!info.subscribed && item->Checked())
			fIMAPFolders.SubscribeFolder(info.folder);
	}

	status->PostMessage(B_QUIT_REQUESTED);
}

