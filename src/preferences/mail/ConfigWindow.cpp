/*
 * Copyright 2007-2015 Haiku, Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//! Main E-Mail config window


#include "ConfigWindow.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <MailDaemon.h>
#include <MailSettings.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <TextView.h>

#include <MailPrivate.h>

#include "AutoConfigWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Config Window"


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
	AccountsListView(BHandler* target)
		:
		BListView(NULL, B_SINGLE_SELECTION_LIST),
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
			BView(NULL, B_WILL_DRAW)
		{
			fBitmap = bitmap;

			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			SetExplicitSize(bitmap->Bounds().Size());
		}

		~BitmapView()
		{
			delete fBitmap;
		}

		virtual void AttachedToWindow()
		{
			SetViewColor(Parent()->ViewColor());
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
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fLastSelectedAccount(NULL),
	fSaveSettings(false)
{
	BTabView* tabView = new BTabView("tab");

	// accounts listview

	BView* view = new BView("accounts", 0);
	tabView->AddTab(view);
	tabView->TabAt(0)->SetLabel(B_TRANSLATE("Accounts"));

	fAccountsListView = new AccountsListView(this);

	BButton* addButton = new BButton(NULL, B_TRANSLATE("Add"),
		new BMessage(kMsgAddAccount));
	fRemoveButton = new BButton(NULL, B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveAccount));

	fConfigView = new BView(NULL, 0);
	fConfigView->SetLayout(new BGroupLayout(B_VERTICAL));
	fConfigView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fConfigView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BScrollView* scroller = new BScrollView(NULL, fAccountsListView, 0,
		false, true);

	BLayoutBuilder::Group<>(view, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.Add(scroller)
			.AddGroup(B_HORIZONTAL)
				.Add(addButton)
				.Add(fRemoveButton)
			.End()
		.End()
		.Add(fConfigView, 2.0f);

	_ReplaceConfigView(_BuildHowToView());

	// general settings

	view = new BView("general", 0);
	view->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	tabView->AddTab(view);
	tabView->TabAt(1)->SetLabel(B_TRANSLATE("Settings"));

	fCheckMailCheckBox = new BCheckBox("check", B_TRANSLATE("Check every"),
		NULL);
	fIntervalControl = new BTextControl("time", B_TRANSLATE("minutes"), NULL,
		NULL);

	BPopUpMenu* statusPopUp = new BPopUpMenu(B_EMPTY_STRING);
	const char* statusModes[] = {
		B_TRANSLATE_COMMENT("Never", "show status window"),
		B_TRANSLATE("While sending"),
		B_TRANSLATE("While sending and receiving")};
	for (size_t i = 0; i < sizeof(statusModes) / sizeof(statusModes[0]); i++) {
		BMessage* msg = new BMessage(kMsgShowStatusWindowChanged);
		BMenuItem* item = new BMenuItem(statusModes[i], msg);
		statusPopUp->AddItem(item);
		msg->AddInt32("ShowStatusWindow", i);
	}

	fStatusModeField = new BMenuField("show status",
		B_TRANSLATE("Show notifications:"), statusPopUp);

	BMessage* msg = new BMessage(B_REFS_RECEIVED);
	BButton* editMenuButton = new BButton(B_EMPTY_STRING,
		B_TRANSLATE("Edit mailbox menu…"), msg);
	editMenuButton->SetTarget(BMessenger("application/x-vnd.Be-TRAK"));

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Mail/Menu Links");
	BEntry entry(path.Path());
	if (entry.InitCheck() == B_OK && entry.Exists()) {
		entry_ref ref;
		entry.GetRef(&ref);
		msg->AddRef("refs", &ref);
	} else
		editMenuButton->SetEnabled(false);

	BLayoutBuilder::Group<>(view, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGlue()
		.AddGroup(B_HORIZONTAL, 0.f)
			.AddGlue()
			.Add(fCheckMailCheckBox)
			.Add(fIntervalControl->CreateTextViewLayoutItem())
			.AddStrut(be_control_look->DefaultLabelSpacing())
			.Add(fIntervalControl->CreateLabelLayoutItem())
			.AddGlue()
		.End()
		.AddGroup(B_HORIZONTAL, 0.f)
			.AddGlue()
			.Add(fStatusModeField->CreateLabelLayoutItem())
			.Add(fStatusModeField->CreateMenuBarLayoutItem())
			.AddGlue()
		.End()
		.Add(editMenuButton)
		.AddGlue();

	// save/revert buttons

	BButton* applyButton = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(kMsgSaveSettings));
	BButton* revertButton = new BButton("revert", B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertSettings));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(tabView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(revertButton)
			.Add(applyButton);

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


BView*
ConfigWindow::_BuildHowToView()
{
	BView* groupView = new BView("howTo", 0);

	BitmapView* bitmapView = NULL;
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		BFile appFile(&info.ref, B_READ_ONLY);
		BAppFileInfo appFileInfo(&appFile);
		if (appFileInfo.InitCheck() == B_OK) {
			BBitmap* bitmap = new (std::nothrow) BBitmap(BRect(0, 0, 63, 63),
				B_RGBA32);
			if (appFileInfo.GetIcon(bitmap, B_LARGE_ICON) == B_OK)
				bitmapView = new BitmapView(bitmap);
			else
				delete bitmap;
		}
	}

	BTextView* text = new BTextView(NULL, B_WILL_DRAW);
	text->SetAlignment(B_ALIGN_CENTER);
	text->SetText(B_TRANSLATE(
		"Create a new account with the Add button.\n\n"
		"Remove an account with the Remove button on the selected item.\n\n"
		"Select an item in the list to change its settings."));
	text->MakeEditable(false);
	text->MakeSelectable(false);

	BLayoutBuilder::Group<>(groupView, B_VERTICAL)
		.AddGlue()
		.Add(text)
		.AddGlue();

	if (bitmapView != NULL)
		groupView->GetLayout()->AddView(1, bitmapView);

	text->SetViewColor(groupView->ViewColor());

	return groupView;
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

	if (status != B_OK)
		CenterOnScreen();
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
	// collect changed accounts
	BMessage changedAccounts(BPrivate::kMsgAccountsChanged);
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

	// apply and save general settings
	BMailSettings settings;
	if (fSaveSettings) {
		// figure out time interval
		float floatInterval;
		sscanf(fIntervalControl->Text(), "%f", &floatInterval);
		bigtime_t interval = bigtime_t(60000000L * floatInterval);

		settings.SetAutoCheckInterval(interval);
		settings.SetDaemonAutoStarts(interval != 0);

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

	BMessenger messenger(B_MAIL_DAEMON_SIGNATURE);
	if (messenger.IsValid()) {
		// server should reload general settings
		messenger.SendMessage(BPrivate::kMsgSettingsUpdated);
		// notify server about changed accounts
		messenger.SendMessage(&changedAccounts);
	}

	// Start the mail_daemon if auto start was selected
	BMailDaemon daemon;
	if (fSaveSettings && settings.DaemonAutoStarts() && !daemon.IsRunning())
		daemon.Launch();
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
				_ReplaceConfigView(_BuildHowToView());
				break;
			}
			AccountItem* item = (AccountItem*)fAccountsListView->ItemAt(index);
			if (item != NULL)
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
					_ReplaceConfigView(_BuildHowToView());
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
			_ReplaceConfigView(_BuildHowToView());
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
ConfigWindow::_SetToGeneralSettings(BMailSettings* settings)
{
	if (settings == NULL)
		return B_BAD_VALUE;

	status_t status = settings->InitCheck();
	if (status != B_OK)
		return status;

	// retrieval frequency
	uint32 interval = uint32(settings->AutoCheckInterval() / 60000000L);
	fCheckMailCheckBox->SetValue(settings->DaemonAutoStarts()
		&& interval != 0 ? B_CONTROL_ON : B_CONTROL_OFF);

	if (interval == 0)
		interval = 5;

	BString intervalText;
	intervalText.SetToFormat("%" B_PRIu32, interval);
	fIntervalControl->SetText(intervalText.String());

	int32 showStatusIndex = settings->ShowStatusWindow();
	BMenuItem* item = fStatusModeField->Menu()->ItemAt(showStatusIndex);
	if (item != NULL) {
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
		BAlert* alert = new BAlert(B_TRANSLATE("Error"), text,
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}

	// revert account data

	if (fAccountsListView->CurrentSelection() != -1)
		_ReplaceConfigView(_BuildHowToView());

	for (int32 i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		_RemoveAccountFromListView(account);
		delete account;
	}

	fAccounts.MakeEmpty();
	_LoadAccounts();
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

	BView* view = NULL;
	switch (item->GetType()) {
		case ACCOUNT_ITEM:
			view = new AccountConfigView(account);
			break;

		case INBOUND_ITEM:
			view = new ProtocolConfigView(*account, account->InboundAddOnRef(),
				account->InboundSettings());
			break;

		case OUTBOUND_ITEM:
			view = new ProtocolConfigView(*account, account->OutboundAddOnRef(),
				account->OutboundSettings());
			break;

		case FILTER_ITEM:
			view = new FiltersConfigView(*account);
			break;
	}
	if (view != NULL)
		item->SetConfigPanel(view);

	_ReplaceConfigView(view);
}


void
ConfigWindow::_ReplaceConfigView(BView* view)
{
	while (BView* child = fConfigView->ChildAt(0)) {
		fConfigView->RemoveChild(child);
		delete child;
	}

	if (view != NULL)
		fConfigView->AddChild(view);
}
