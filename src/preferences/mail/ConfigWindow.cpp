/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//! main E-Mail config window


#include "ConfigWindow.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Catalog.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <TextControl.h>
#include <TextView.h>
#include <MenuItem.h>
#include <Screen.h>
#include <PopUpMenu.h>
#include <MenuBar.h>
#include <TabView.h>
#include <Box.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Roster.h>
#include <Resources.h>
#include <Region.h>

#include <AppFileInfo.h>
#include <Entry.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>

#include <Catalog.h>
#include <Locale.h>

#include <MailDaemon.h>
#include <MailSettings.h>

#include "AutoConfigWindow.h"
#include "CenterContainer.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Config Window"


using std::nothrow;

// define if you want to have an apply button
//#define HAVE_APPLY_BUTTON


const uint32 kMsgAccountsRightClicked = 'arcl';
const uint32 kMsgAccountSelected = 'acsl';
const uint32 kMsgAddAccount = 'adac';
const uint32 kMsgRemoveAccount = 'rmac';

const uint32 kMsgIntervalUnitChanged = 'iuch';

const uint32 kMsgShowStatusWindowChanged = 'shst';
const uint32 kMsgStatusLookChanged = 'lkch';
const uint32 kMsgStatusWorkspaceChanged = 'wsch';

const uint32 kMsgSaveSettings = 'svst';
const uint32 kMsgRevertSettings = 'rvst';
const uint32 kMsgCancelSettings = 'cnst';



AccountItem::AccountItem(const char* label, BMailAccountSettings* account,
	item_types type)
	:
	BStringItem(label),
	fAccount(account),
	fType(type),
	fConfigPanel(NULL)
{
}


void
AccountItem::Update(BView* owner, const BFont* font)
{
	if (fType == ACCOUNT_ITEM)
		font = be_bold_font;

	BStringItem::Update(owner,font);
}


void
AccountItem::DrawItem(BView* owner, BRect rect, bool complete)
{
	owner->PushState();
	if (fType == ACCOUNT_ITEM)
		owner->SetFont(be_bold_font);

	BStringItem::DrawItem(owner, rect, complete);
	owner->PopState();
}


void
AccountItem::SetConfigPanel(BView* panel)
{
	fConfigPanel = panel;
}


BView*
AccountItem::ConfigPanel()
{
	return fConfigPanel;
}


//	#pragma mark -


class AccountsListView : public BListView {
public:
	AccountsListView(BRect rect, BHandler* target)
		:
		BListView(rect, NULL, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL),
		fTarget(target)
	{
	}

	void
	KeyDown(const char *bytes, int32 numBytes)
	{
		if (numBytes != 1)
			return;

		if ((*bytes == B_DELETE) || (*bytes == B_BACKSPACE))
			Window()->PostMessage(kMsgRemoveAccount);

		BListView::KeyDown(bytes,numBytes);
	}

	void
	MouseDown(BPoint point)
	{
		BListView::MouseDown(point);

		BPoint dummy;
		uint32 buttons;
		GetMouse(&dummy, &buttons);
		if (buttons != B_SECONDARY_MOUSE_BUTTON)
			return;

		int32 index = IndexOf(point);
		if (index < 0)
			return;

		BMessage message(kMsgAccountsRightClicked);
		ConvertToScreen(&point);
		message.AddPoint("point", point);
		message.AddInt32("index", index);
		BMessenger messenger(fTarget);
		messenger.SendMessage(&message);
	}

private:
			BHandler*			fTarget;
};


class BitmapView : public BView {
	public:
		BitmapView(BBitmap *bitmap)
			:
			BView(bitmap->Bounds(), NULL, B_FOLLOW_NONE, B_WILL_DRAW)
		{
			fBitmap = bitmap;

			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		}

		~BitmapView()
		{
			delete fBitmap;
		}

		virtual void AttachedToWindow()
		{
			SetViewColor(Parent()->ViewColor());

			MoveTo((Parent()->Bounds().Width() - Bounds().Width()) / 2,
				Frame().top);
		}

		virtual void Draw(BRect updateRect)
		{
			DrawBitmap(fBitmap, updateRect, updateRect);
		}

	private:
		BBitmap *fBitmap;
};


//	#pragma mark -


ConfigWindow::ConfigWindow()
	:
	BWindow(BRect(100, 100, 600, 540), B_TRANSLATE_SYSTEM_NAME("E-mail"),
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
	fLastSelectedAccount(NULL),
	fSaveSettings(false)
{
	// create controls
	BRect rect(Bounds());
	BView *top = new BView(rect, NULL, B_FOLLOW_ALL, 0);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);

	// determine font height
	font_height fontHeight;
	top->GetFontHeight(&fontHeight);
	int32 height = (int32)(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 5;

	rect.InsetBy(10, 10);
	rect.bottom -= 18 + height;
	BTabView *tabView = new BTabView(rect, NULL);

	BView *view;
	rect = tabView->Bounds();
	rect.bottom -= tabView->TabHeight() + 4;
	tabView->AddTab(view = new BView(rect, NULL, B_FOLLOW_ALL, 0));
	tabView->TabAt(0)->SetLabel(B_TRANSLATE("Accounts"));
	view->SetViewColor(top->ViewColor());

	// accounts listview

	rect = view->Bounds().InsetByCopy(10, 10);
	rect.right = 190 - B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= height + 18;
	fAccountsListView = new AccountsListView(rect, this);
	view->AddChild(new BScrollView(NULL, fAccountsListView, B_FOLLOW_ALL, 0,
		false, true));
	rect.right += B_V_SCROLL_BAR_WIDTH;

	rect.left -= 2;
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + height;
	BRect sizeRect = rect;
	sizeRect.right = sizeRect.left + 30 + view->StringWidth(
		B_TRANSLATE("Add"));
	view->AddChild(new BButton(sizeRect, NULL, B_TRANSLATE("Add"),
		new BMessage(kMsgAddAccount), B_FOLLOW_BOTTOM));

	sizeRect.left = sizeRect.right + 5;
	sizeRect.right = sizeRect.left + 30 + view->StringWidth(
		B_TRANSLATE("Remove"));
	view->AddChild(fRemoveButton = new BButton(
		sizeRect, NULL, B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveAccount), B_FOLLOW_BOTTOM));

	// accounts config view
	rect = view->Bounds();
	rect.left = fAccountsListView->Frame().right + B_V_SCROLL_BAR_WIDTH + 16;
	rect.right -= 10;
	fConfigView = new CenterContainer(rect);
	view->AddChild(fConfigView);

	_MakeHowToView();

	// general settings

	rect = tabView->ContainerView()->Bounds();
	tabView->AddTab(view = new CenterContainer(rect));
	tabView->TabAt(1)->SetLabel(B_TRANSLATE("Settings"));

	rect = view->Bounds().InsetByCopy(10, 10);
	rect.bottom = rect.top + height * 5 + 15;
	BBox *box = new BBox(rect);
	box->SetLabel(B_TRANSLATE("Mail checking"));
	view->AddChild(box);

	rect = box->Bounds().InsetByCopy(10, 10);
	rect.top += 7;
	rect.bottom = rect.top + height;
	BRect tile = rect.OffsetByCopy(0, 1);
	int32 labelWidth = (int32)view->StringWidth(B_TRANSLATE("Check every")) + 6;
	tile.right = 80 + labelWidth;
	fIntervalControl = new BTextControl(tile, "time",
		B_TRANSLATE("Check every"), NULL, NULL);
	fIntervalControl->SetDivider(labelWidth);
	box->AddChild(fIntervalControl);

	BPopUpMenu* frequencyPopUp = new BPopUpMenu(B_EMPTY_STRING);
	const char* frequencyStrings[] = {
		B_TRANSLATE_COMMENT("never", "mail checking frequency"),
		B_TRANSLATE("minutes"),
		B_TRANSLATE("hours"),
		B_TRANSLATE("days")};

	for (int32 i = 0; i < 4; i++) {
		BMenuItem* item = new BMenuItem(frequencyStrings[i],
			new BMessage(kMsgIntervalUnitChanged));
		frequencyPopUp->AddItem(item);
	}
	tile.left = tile.right + 5;
	tile.right = rect.right;
	tile.OffsetBy(0,-1);
	fIntervalUnitField = new BMenuField(tile, "frequency", B_EMPTY_STRING,
		frequencyPopUp);
	fIntervalUnitField->SetDivider(0.0);
	box->AddChild(fIntervalUnitField);

	rect.OffsetBy(0,height + 9);
	rect.bottom -= 2;
	fPPPActiveCheckBox = new BCheckBox(rect, "ppp active",
		B_TRANSLATE("Only when dial-up is connected"), NULL);
	box->AddChild(fPPPActiveCheckBox);

	rect.OffsetBy(0,height + 9);
	rect.bottom -= 2;
	fPPPActiveSendCheckBox = new BCheckBox(rect, "ppp activesend",
		B_TRANSLATE("Schedule outgoing mail when dial-up is disconnected"),
		NULL);
	box->AddChild(fPPPActiveSendCheckBox);

	// Miscellaneous settings box

	rect = box->Frame();  rect.bottom = rect.top + 3 * height + 33;
	box = new BBox(rect);
	box->SetLabel(B_TRANSLATE("Miscellaneous"));
	view->AddChild(box);

	BPopUpMenu *statusPopUp = new BPopUpMenu(B_EMPTY_STRING);
	const char *statusModes[] = {
		B_TRANSLATE_COMMENT("Never", "show status window"),
		B_TRANSLATE("While sending"),
		B_TRANSLATE("While sending and receiving"),
		B_TRANSLATE("Always")};
	for (int32 i = 0; i < 4; i++) {
		BMessage* msg = new BMessage(kMsgShowStatusWindowChanged);
		BMenuItem* item = new BMenuItem(statusModes[i], msg);
		statusPopUp->AddItem(item);
		msg->AddInt32("ShowStatusWindow", i);
	}
	rect = box->Bounds().InsetByCopy(10, 10);
	rect.top += 7;
	rect.bottom = rect.top + height + 5;
	labelWidth = (int32)view->StringWidth(
		B_TRANSLATE("Show connection status window:"))	+ 8;
	fStatusModeField = new BMenuField(rect, "show status",
		B_TRANSLATE("Show connection status window:"), statusPopUp);
	fStatusModeField->SetDivider(labelWidth);
	box->AddChild(fStatusModeField);

	rect = fStatusModeField->Frame();;
	rect.OffsetBy(0, rect.Height() + 10);
	BMessage* msg = new BMessage(B_REFS_RECEIVED);
	BButton *button = new BButton(rect, B_EMPTY_STRING,
		B_TRANSLATE("Edit mailbox menu…"), msg);
	button->ResizeToPreferred();
	box->AddChild(button);
	button->SetTarget(BMessenger("application/x-vnd.Be-TRAK"));

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Mail/Menu Links");
	BEntry entry(path.Path());
	if (entry.InitCheck() == B_OK && entry.Exists()) {
		entry_ref ref;
		entry.GetRef(&ref);
		msg->AddRef("refs", &ref);
	}
	else
		button->SetEnabled(false);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 30,0);
	fAutoStartCheckBox = new BCheckBox(rect, "start daemon",
		B_TRANSLATE("Start mail services on startup"), NULL);
	fAutoStartCheckBox->ResizeToPreferred();
	box->AddChild(fAutoStartCheckBox);

	// save/revert buttons

	top->AddChild(tabView);

	rect = tabView->Frame();
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + height + 5;
	BButton *saveButton = new BButton(rect, "apply", B_TRANSLATE("Apply"),
		new BMessage(kMsgSaveSettings));
	float w,h;
	saveButton->GetPreferredSize(&w, &h);
	saveButton->ResizeTo(w, h);
	saveButton->MoveTo(rect.right - w, rect.top);
	top->AddChild(saveButton);

	BButton *revertButton = new BButton(rect, "revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertSettings));
	revertButton->GetPreferredSize(&w, &h);
	revertButton->ResizeTo(w,h);
	revertButton->MoveTo(saveButton->Frame().left - 10 - w, rect.top);
	top->AddChild(revertButton);

	_LoadSettings();
		// this will also move our window to the stored position

	fAccountsListView->SetSelectionMessage(new BMessage(kMsgAccountSelected));
	fAccountsListView->MakeFocus(true);
}


ConfigWindow::~ConfigWindow()
{
	while (fAccounts.CountItems() > 0)
		_RemoveAccount(fAccounts.ItemAt(0));
	for (int32 i = 0; i < fToDeleteAccounts.CountItems(); i++)
		delete fToDeleteAccounts.ItemAt(i);
}


void
ConfigWindow::_MakeHowToView()
{
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BFile appFile(&info.ref, B_READ_ONLY);
		BAppFileInfo appFileInfo(&appFile);
		if (appFileInfo.InitCheck() == B_OK) {
			BBitmap *bitmap = new (nothrow) BBitmap(BRect(0, 0, 63, 63),
				B_RGBA32);
			if (appFileInfo.GetIcon(bitmap, B_LARGE_ICON) == B_OK) {
				fConfigView->AddChild(new BitmapView(bitmap));
			} else
				delete bitmap;
		}
	}

	BRect rect = fConfigView->Bounds();
	BTextView *text = new BTextView(rect, NULL, rect, B_FOLLOW_NONE,
		B_WILL_DRAW);
	text->SetViewColor(fConfigView->Parent()->ViewColor());
	text->SetAlignment(B_ALIGN_CENTER);
	text->SetText(B_TRANSLATE(
		"\n\nCreate a new account with the Add button.\n\n"
		"Remove an account with the Remove button on the selected item.\n\n"
		"Select an item in the list to change its settings."));
	rect = text->Bounds();
	text->ResizeTo(rect.Width(), text->TextHeight(0, 42));
	text->SetTextRect(rect);

	text->MakeEditable(false);
	text->MakeSelectable(false);

	fConfigView->AddChild(text);

	fConfigView->Layout();
}


void
ConfigWindow::_LoadSettings()
{
	// load accounts
	for (int i = 0; i < fAccounts.CountItems(); i++)
		delete fAccounts.ItemAt(i);
	fAccounts.MakeEmpty();

	_LoadAccounts();

	// load in general settings
	BMailSettings settings;
	status_t status = _SetToGeneralSettings(&settings);
	if (status == B_OK) {
		// move own window
		MoveTo(settings.ConfigWindowFrame().LeftTop());
	} else {
		fprintf(stderr, B_TRANSLATE("Error retrieving general settings: %s\n"),
			strerror(status));
	}

	BScreen screen(this);
	BRect screenFrame(screen.Frame().InsetByCopy(0, 5));
	if (!screenFrame.Contains(Frame().LeftTop())
		|| !screenFrame.Contains(Frame().RightBottom()))
		status = B_ERROR;

	if (status != B_OK) {
		// center window on screen
		MoveTo((screenFrame.Width() - Frame().Width()) / 2,
			(screenFrame.Height() - Frame().Height()) / 2);
	}
}


void
ConfigWindow::_LoadAccounts()
{
	BMailAccounts accounts;
	for (int32 i = 0; i < accounts.CountAccounts(); i++)
		fAccounts.AddItem(new BMailAccountSettings(*accounts.AccountAt(i)));

	for (int i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		_AddAccountToView(account);
	}
}


void
ConfigWindow::_SaveSettings()
{
	// remove config views (trigger view archive)
	fConfigView->DeleteChildren();

	// collect changed accounts
	BMessage changedAccounts(kMsgAccountsChanged);
	for (int32 i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		if (account && account->HasBeenModified())
			changedAccounts.AddInt32("account", account->AccountID());
	}
	for (int32 i = 0; i < fToDeleteAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fToDeleteAccounts.ItemAt(i);
		changedAccounts.AddInt32("account", account->AccountID());
	}

	// cleanup account directory
	for (int32 i = 0; i < fToDeleteAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fToDeleteAccounts.ItemAt(i);
		BEntry entry(account->AccountFile());
		entry.Remove();
		delete account;
	}
	fToDeleteAccounts.MakeEmpty();

	/*** save general settings ***/

	// figure out time interval
	float interval;
	sscanf(fIntervalControl->Text(),"%f",&interval);
	float multiplier = 0;
	switch (fIntervalUnitField->Menu()->IndexOf(
			fIntervalUnitField->Menu()->FindMarked())) {
		case 1:		// minutes
			multiplier = 60;
			break;
		case 2:		// hours
			multiplier = 60 * 60;
			break;
		case 3:		// days
			multiplier = 24 * 60 * 60;
			break;
	}
	time_t time = (time_t)(multiplier * interval);

	// apply and save general settings
	BMailSettings settings;
	if (fSaveSettings) {
		settings.SetAutoCheckInterval(time * 1e6);
		settings.SetCheckOnlyIfPPPUp(fPPPActiveCheckBox->Value()
			== B_CONTROL_ON);
		settings.SetSendOnlyIfPPPUp(fPPPActiveSendCheckBox->Value()
			== B_CONTROL_ON);
		settings.SetDaemonAutoStarts(fAutoStartCheckBox->Value()
			== B_CONTROL_ON);

		// status mode (alway, fetching/retrieving, ...)
		int32 index = fStatusModeField->Menu()->IndexOf(
			fStatusModeField->Menu()->FindMarked());
		settings.SetShowStatusWindow(index);

	} else {
		// restore status window look
		settings.SetStatusWindowLook(settings.StatusWindowLook());
	}

	settings.SetConfigWindowFrame(Frame());
	settings.Save();

	/*** save accounts ***/

	if (fSaveSettings) {
		for (int i = 0; i < fAccounts.CountItems(); i++)
			fAccounts.ItemAt(i)->Save();
	}

	BMessenger messenger("application/x-vnd.Be-POST");
	if (messenger.IsValid()) {
		// server should reload general settings
		messenger.SendMessage(kMsgSettingsUpdated);
		// notify server about changed accounts
		messenger.SendMessage(&changedAccounts);
	}

	// start the mail_daemon if auto start was selected
	if (fSaveSettings && fAutoStartCheckBox->Value() == B_CONTROL_ON
		&& !be_roster->IsRunning("application/x-vnd.Be-POST"))
	{
		be_roster->Launch("application/x-vnd.Be-POST");
	}
}


bool
ConfigWindow::QuitRequested()
{
	_SaveSettings();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
ConfigWindow::MessageReceived(BMessage *msg)
{
	BRect autoConfigRect(0, 0, 400, 300);
	BRect frame;

	AutoConfigWindow *autoConfigWindow = NULL;
	switch (msg->what) {
		case kMsgAccountsRightClicked:
		{
			BPoint point;
			msg->FindPoint("point", &point);
			int32 index = msg->FindInt32("index");
			AccountItem* clickedItem = dynamic_cast<AccountItem*>(
				fAccountsListView->ItemAt(index));
			if (clickedItem == NULL || clickedItem->GetType() != ACCOUNT_ITEM)
				break;

			BPopUpMenu rightClickMenu("accounts", false, false);

			BMenuItem* inMenuItem = new BMenuItem(B_TRANSLATE("Incoming"),
				NULL);
			BMenuItem* outMenuItem = new BMenuItem(B_TRANSLATE("Outgoing"),
				NULL);
			rightClickMenu.AddItem(inMenuItem);
			rightClickMenu.AddItem(outMenuItem);

			BMailAccountSettings* settings = clickedItem->GetAccount();
			if (settings->IsInboundEnabled())
				inMenuItem->SetMarked(true);
			if (settings->IsOutboundEnabled())
				outMenuItem->SetMarked(true);

			BMenuItem* selectedItem = rightClickMenu.Go(point);
			if (selectedItem == NULL)
				break;
			if (selectedItem == inMenuItem) {
				AccountItem* item = dynamic_cast<AccountItem*>(
					fAccountsListView->ItemAt(index + 1));
				if (item == NULL)
					break;
				if (settings->IsInboundEnabled()) {
					settings->SetInboundEnabled(false);
					item->SetEnabled(false);
				} else {
					settings->SetInboundEnabled(true);
					item->SetEnabled(true);
				}
			} else {
				AccountItem* item = dynamic_cast<AccountItem*>(
					fAccountsListView->ItemAt(index + 2));
				if (item == NULL)
					break;
				if (settings->IsOutboundEnabled()) {
					settings->SetOutboundEnabled(false);
					item->SetEnabled(false);
				} else {
					settings->SetOutboundEnabled(true);
					item->SetEnabled(true);
				}
			}
		}

		case kMsgAccountSelected:
		{
			int32 index;
			if (msg->FindInt32("index", &index) != B_OK || index < 0) {
				// deselect current item
				fConfigView->DeleteChildren();
				_MakeHowToView();
				break;
			}
			AccountItem *item = (AccountItem *)fAccountsListView->ItemAt(index);
			if (item)
				_AccountSelected(item);
			break;
		}

		case kMsgAddAccount:
		{
			frame = Frame();
			autoConfigRect.OffsetTo(
				frame.left + (frame.Width() - autoConfigRect.Width()) / 2,
				frame.top + (frame.Width() - autoConfigRect.Height()) / 2);
			autoConfigWindow = new AutoConfigWindow(autoConfigRect, this);
			autoConfigWindow->Show();
			break;
		}

		case kMsgRemoveAccount:
		{
			int32 index = fAccountsListView->CurrentSelection();
			if (index >= 0) {
				AccountItem *item = (AccountItem *)fAccountsListView->ItemAt(
					index);
				if (item != NULL) {
					_RemoveAccount(item->GetAccount());
					_MakeHowToView();
				}
			}
			break;
		}

		case kMsgIntervalUnitChanged:
		{
			int32 index;
			if (msg->FindInt32("index",&index) == B_OK)
				fIntervalControl->SetEnabled(index != 0);
			break;
		}

		case kMsgShowStatusWindowChanged:
		{
			// the status window stuff is the only "live" setting
			BMessenger messenger("application/x-vnd.Be-POST");
			if (messenger.IsValid())
				messenger.SendMessage(msg);
			break;
		}

		case kMsgRevertSettings:
			_RevertToLastSettings();
			break;

		case kMsgSaveSettings:
			fSaveSettings = true;
			_SaveSettings();
			AccountUpdated(fLastSelectedAccount);
			_MakeHowToView();
			fAccountsListView->DeselectAll();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


BMailAccountSettings*
ConfigWindow::AddAccount()
{
	BMailAccountSettings* account = new BMailAccountSettings;
	if (!account)
		return NULL;
	fAccounts.AddItem(account);
	_AddAccountToView(account);
	return account;
}


void
ConfigWindow::AccountUpdated(BMailAccountSettings* account)
{
	if (account == NULL)
		return;

	for (int i = 0; i < fAccountsListView->CountItems(); i++) {
		AccountItem* item = (AccountItem*)fAccountsListView->ItemAt(i);
		if (item->GetAccount() == account) {
			if (item->GetType() == ACCOUNT_ITEM) {
				item->SetText(account->Name());
				fAccountsListView->Invalidate();
			}
		}
	}
}


status_t
ConfigWindow::_SetToGeneralSettings(BMailSettings *settings)
{
	if (!settings)
		return B_BAD_VALUE;

	status_t status = settings->InitCheck();
	if (status != B_OK)
		return status;

	// retrieval frequency

	time_t interval = time_t(settings->AutoCheckInterval() / 1e6L);
	char text[25];
	text[0] = 0;
	int timeIndex = 0;
	if (interval >= 60) {
		timeIndex = 1;
		sprintf(text, "%ld", interval / (60));
	}
	if (interval >= (60*60)) {
		timeIndex = 2;
		sprintf(text, "%ld", interval / (60*60));
	}
	if (interval >= (60*60*24)) {
		timeIndex = 3;
		sprintf(text, "%ld", interval / (60*60*24));
	}
	fIntervalControl->SetText(text);

	if (BMenuItem *item = fIntervalUnitField->Menu()->ItemAt(timeIndex))
		item->SetMarked(true);
	fIntervalControl->SetEnabled(timeIndex != 0);

	fPPPActiveCheckBox->SetValue(settings->CheckOnlyIfPPPUp());
	fPPPActiveSendCheckBox->SetValue(settings->SendOnlyIfPPPUp());

	fAutoStartCheckBox->SetValue(settings->DaemonAutoStarts());

	int32 showStatusIndex = settings->ShowStatusWindow();
	BMenuItem *item = fStatusModeField->Menu()->ItemAt(showStatusIndex);
	if (item) {
		item->SetMarked(true);
		// send live update to the server by simulating a menu click
		BMessage msg(kMsgShowStatusWindowChanged);
		msg.AddInt32("ShowStatusWindow", showStatusIndex);
		PostMessage(&msg);
	}

	return B_OK;
}


void
ConfigWindow::_RevertToLastSettings()
{
	// revert general settings
	BMailSettings settings;

	// restore status window look
	settings.SetStatusWindowLook(settings.StatusWindowLook());

	status_t status = _SetToGeneralSettings(&settings);
	if (status != B_OK) {
		char text[256];
		sprintf(text, B_TRANSLATE(
				"\nThe general settings couldn't be reverted.\n\n"
				"Error retrieving general settings:\n%s\n"),
			strerror(status));
		(new BAlert(B_TRANSLATE("Error"), text, B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
	}

	// revert account data

	if (fAccountsListView->CurrentSelection() != -1)
		fConfigView->DeleteChildren();

	for (int32 i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		_RemoveAccountFromListView(account);
		delete account;
	}
	fAccounts.MakeEmpty();
	_LoadAccounts();

	if (fConfigView->CountChildren() == 0)
		_MakeHowToView();
}


void
ConfigWindow::_AddAccountToView(BMailAccountSettings* account)
{
	BString label;
	label << account->Name();

	AccountItem* item;
	item = new AccountItem(label, account, ACCOUNT_ITEM);
	fAccountsListView->AddItem(item);

	item = new AccountItem(B_TRANSLATE("\t\t· Incoming"), account, INBOUND_ITEM);
	fAccountsListView->AddItem(item);
	if (!account->IsInboundEnabled())
		item->SetEnabled(false);

	item = new AccountItem(B_TRANSLATE("\t\t· Outgoing"), account,
		OUTBOUND_ITEM);
	fAccountsListView->AddItem(item);
	if (!account->IsOutboundEnabled())
		item->SetEnabled(false);

	item = new AccountItem(B_TRANSLATE("\t\t· E-mail filters"), account,
		FILTER_ITEM);
	fAccountsListView->AddItem(item);
}


void
ConfigWindow::_RemoveAccount(BMailAccountSettings* account)
{
	_RemoveAccountFromListView(account);
	fAccounts.RemoveItem(account);
	fToDeleteAccounts.AddItem(account);
}


void
ConfigWindow::_RemoveAccountFromListView(BMailAccountSettings* account)
{
	for (int i = 0; i < fAccountsListView->CountItems(); i++) {
		AccountItem* item = (AccountItem*)fAccountsListView->ItemAt(i);
		if (item->GetAccount() == account) {
			fAccountsListView->RemoveItem(i);
			fConfigView->RemoveChild(item->ConfigPanel());
			delete item;
			i--;
		}
	}
}


void
ConfigWindow::_AccountSelected(AccountItem* item)
{
	AccountUpdated(fLastSelectedAccount);

	BMailAccountSettings* account = item->GetAccount();
	fLastSelectedAccount = account;

	fConfigView->Hide();
	fConfigView->DeleteChildren();

	BView* view = NULL;
	switch (item->GetType()) {
		case ACCOUNT_ITEM:
			view = new AccountConfigView(fConfigView->Bounds(), account);
			break;

		case INBOUND_ITEM:
		{
			view = new InProtocolsConfigView(account);
			break;
		}

		case OUTBOUND_ITEM:
		{
			view = new OutProtocolsConfigView(account);
			break;
		}

		case FILTER_ITEM:
		{
			view = new FiltersConfigView(fConfigView->Bounds(), *account);
			break;
		}
	}
	if (view) {
		item->SetConfigPanel(view);
		fConfigView->AddChild(view);
	}

	fConfigView->Layout();
	fConfigView->Show();
}
