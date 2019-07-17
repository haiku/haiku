/*
 * Copyright 2007-2015, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "AutoConfigView.h"

#include <pwd.h>

#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Window.h>

#include <MailSettings.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "E-Mail"


AutoConfigView::AutoConfigView(AutoConfig &config)
	:
	BBox("auto config"),
	fAutoConfig(config)
{
	// Search for SMTP entry_ref
	_GetSMTPAddOnRef(&fSMTPAddOnRef);

	fInProtocolsField = new BMenuField(NULL, NULL, _SetupProtocolMenu());

	fEmailView = new BTextControl("email", B_TRANSLATE("E-mail address:"),
		"", new BMessage(kEMailChangedMsg));

	fLoginNameView = new BTextControl("login", B_TRANSLATE("Login name:"),
		"", NULL);

	fPasswordView = new BTextControl("password", B_TRANSLATE("Password:"),
		"", NULL);
	fPasswordView->TextView()->HideTyping(true);

	fAccountNameView = new BTextControl("account", B_TRANSLATE("Account name:"),
		"", NULL);

	fNameView = new BTextControl("name", B_TRANSLATE("Real name:"), "", NULL);

	struct passwd* passwd = getpwent();
	if (passwd != NULL)
		fNameView->SetText(passwd->pw_gecos);

	AddChild(BLayoutBuilder::Grid<>()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.SetSpacing(B_USE_HALF_ITEM_SPACING, B_USE_HALF_ITEM_SPACING)

		.Add(fInProtocolsField->CreateLabelLayoutItem(), 0, 0)
		.Add(fInProtocolsField->CreateMenuBarLayoutItem(), 1, 0)

		.Add(fEmailView->CreateLabelLayoutItem(), 0, 1)
		.Add(fEmailView->CreateTextViewLayoutItem(), 1, 1)

		.Add(fLoginNameView->CreateLabelLayoutItem(), 0, 2)
		.Add(fLoginNameView->CreateTextViewLayoutItem(), 1, 2)

		.Add(fPasswordView->CreateLabelLayoutItem(), 0, 3)
		.Add(fPasswordView->CreateTextViewLayoutItem(), 1, 3)

		.Add(fAccountNameView->CreateLabelLayoutItem(), 0, 4)
		.Add(fAccountNameView->CreateTextViewLayoutItem(), 1, 4)

		.Add(fNameView->CreateLabelLayoutItem(), 0, 5)
		.Add(fNameView->CreateTextViewLayoutItem(), 1, 5)
		.View());
}


void
AutoConfigView::AttachedToWindow()
{
	// Resize the view to fit the contents properly
	BRect rect = Bounds();
	float newHeight = fNameView->Frame().bottom + 20 + 2;
	newHeight += InnerFrame().top;
	ResizeTo(rect.Width(), newHeight);

	AdoptParentColors();
	fEmailView->SetTarget(this);
	fEmailView->MakeFocus(true);
}


void
AutoConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kEMailChangedMsg:
		{
			BString text = fLoginNameView->Text();
			if (text == "")
				_ProposeUsername();
			fLoginNameView->MakeFocus();
			fLoginNameView->TextView()->SelectAll();

			text = fAccountNameView->Text();
			if (text == "")
				fAccountNameView->SetText(fEmailView->Text());
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


bool
AutoConfigView::GetBasicAccountInfo(account_info &info)
{
	status_t status = B_OK;

	BString inboundProtocolName = "";
	BMenuItem* item = fInProtocolsField->Menu()->FindMarked();
	if (item) {
		inboundProtocolName = item->Label();
		item->Message()->FindRef("protocol", &(info.inboundProtocol));
	}
	else
		status = B_ERROR;

	if (inboundProtocolName.FindFirst("IMAP") >= 0)
		info.inboundType = IMAP;
	else
		info.inboundType = POP;

	info.outboundProtocol = fSMTPAddOnRef;
	info.name = fNameView->Text();
	info.accountName = fAccountNameView->Text();
	info.email = fEmailView->Text();
	info.loginName = fLoginNameView->Text();
	info.password = fPasswordView->Text();

	return status;
}


BPopUpMenu*
AutoConfigView::_SetupProtocolMenu()
{
	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("Choose Protocol"));

	// TODO: use path finder!
	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY :
			B_BEOS_ADDONS_DIRECTORY, &path);
		if (status != B_OK)
			return menu;

		path.Append("mail_daemon");
		path.Append("inbound_protocols");

		BDirectory dir(path.Path());
		entry_ref protocolRef;
		while (dir.GetNextRef(&protocolRef) == B_OK)
		{
			BEntry entry(&protocolRef);

			BMessage* msg = new BMessage(kProtokollChangedMsg);
			BMenuItem* item = new BMenuItem(entry.Name(), msg);
			menu->AddItem(item);
			msg->AddRef("protocol", &protocolRef);

			item->SetMarked(true);
		}
	}

	// make imap default protocol if existing
	BMenuItem* imapItem =  menu->FindItem("IMAP");
	if (imapItem)
		imapItem->SetMarked(true);

	return menu;
}


status_t
AutoConfigView::_GetSMTPAddOnRef(entry_ref *ref)
{
	directory_which which[] = {
		B_USER_ADDONS_DIRECTORY,
		B_BEOS_ADDONS_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath path;
		status_t status = find_directory(which[i], &path);
		if (status != B_OK)
			return B_ERROR;

		path.Append("mail_daemon");
		path.Append("outbound_protocols");
		path.Append("SMTP");

		BEntry entry(path.Path());
		if (entry.Exists() && entry.GetRef(ref) == B_OK)
			return B_OK;
	}

	return B_FILE_NOT_FOUND;
}


BString
AutoConfigView::_ExtractLocalPart(const char* email)
{
	const char* at = strrchr(email, '@');
	return BString(email, at - email);
}


void
AutoConfigView::_ProposeUsername()
{
	const char* email = fEmailView->Text();
	provider_info info;
	status_t status = fAutoConfig.GetInfoFromMailAddress(email, &info);
	if (status == B_OK) {
		BString localPart = _ExtractLocalPart(email);
		switch (info.username_pattern) {
			case 0:
				// username is the mail address
				fLoginNameView->SetText(email);
				break;
			case 1:
				// username is the local-part
				fLoginNameView->SetText(localPart.String());
				break;
			case 2:
				// do nothing
				break;
		}
	}
	else {
		fLoginNameView->SetText(email);
	}
}


bool
AutoConfigView::IsValidMailAddress(BString email)
{
	int32 atPos = email.FindFirst("@");
	if (atPos < 0)
		return false;
	BString provider;
	email.CopyInto(provider, atPos + 1, email.Length() - atPos);
	if (provider.FindLast(".") < 0)
		return false;
	return true;
}


// #pragma mark -


ServerSettingsView::ServerSettingsView(const account_info &info)
	:
	BGroupView("server", B_VERTICAL),
	fInboundAccount(true),
	fOutboundAccount(true),
	fInboundAuthMenu(NULL),
	fOutboundAuthMenu(NULL),
	fInboundEncrItemStart(NULL),
	fOutboundEncrItemStart(NULL),
	fImageID(-1)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fInboundAccount = true;
	fOutboundAccount = true;

	// inbound
	BBox* box = new BBox("inbound");
	box->SetLabel(B_TRANSLATE("Incoming"));
	AddChild(box);

	BString serverName;
	if (info.inboundType == IMAP)
		serverName = info.providerInfo.imap_server;
	else
		serverName = info.providerInfo.pop_server;

	BGridView* grid = new BGridView("inner");
	grid->GridLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	box->AddChild(grid);

	fInboundNameView = new BTextControl("inbound", B_TRANSLATE("Server Name:"),
		serverName, new BMessage(kServerChangedMsg));
	grid->GridLayout()->AddItem(fInboundNameView->CreateLabelLayoutItem(),
		0, 0);
	grid->GridLayout()->AddItem(fInboundNameView->CreateTextViewLayoutItem(),
		1, 0);

	int32 row = 1;

	_GetAuthEncrMenu(info.inboundProtocol, fInboundAuthMenu,
		fInboundEncryptionMenu);
	if (fInboundAuthMenu != NULL) {
		int authID = info.providerInfo.authentification_pop;
		if (info.inboundType == POP)
			fInboundAuthMenu->Menu()->ItemAt(authID)->SetMarked(true);
		fInboundAuthItemStart = fInboundAuthMenu->Menu()->FindMarked();

		grid->GridLayout()->AddItem(fInboundAuthMenu->CreateLabelLayoutItem(),
			0, row);
		grid->GridLayout()->AddItem(fInboundAuthMenu->CreateMenuBarLayoutItem(),
			1, row++);
	}
	if (fInboundEncryptionMenu != NULL) {
		BMenuItem *item = NULL;
		if (info.inboundType == POP) {
			item = fInboundEncryptionMenu->Menu()->ItemAt(
				info.providerInfo.ssl_pop);
			if (item != NULL)
				item->SetMarked(true);
		}
		if (info.inboundType == IMAP) {
			item = fInboundEncryptionMenu->Menu()->ItemAt(
				info.providerInfo.ssl_imap);
			if (item != NULL)
				item->SetMarked(true);
		}
		fInboundEncrItemStart = fInboundEncryptionMenu->Menu()->FindMarked();

		grid->GridLayout()->AddItem(
			fInboundEncryptionMenu->CreateLabelLayoutItem(), 0, row);
		grid->GridLayout()->AddItem(
			fInboundEncryptionMenu->CreateMenuBarLayoutItem(), 1, row++);
	}
	grid->GridLayout()->AddItem(BSpaceLayoutItem::CreateGlue(), 0, row);

	if (!fInboundAccount)
		box->Hide();

	// outbound
	box = new BBox("outbound");
	box->SetLabel(B_TRANSLATE("Outgoing"));
	AddChild(box);

	grid = new BGridView("inner");
	grid->GridLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	box->AddChild(grid);

	serverName = info.providerInfo.smtp_server;
	fOutboundNameView = new BTextControl("outbound",
		B_TRANSLATE("Server name:"), serverName.String(),
		new BMessage(kServerChangedMsg));
	grid->GridLayout()->AddItem(fOutboundNameView->CreateLabelLayoutItem(),
		0, 0);
	grid->GridLayout()->AddItem(fOutboundNameView->CreateTextViewLayoutItem(),
		1, 0);

	row = 1;

	_GetAuthEncrMenu(info.outboundProtocol, fOutboundAuthMenu,
		fOutboundEncryptionMenu);
	if (fOutboundAuthMenu != NULL) {
		BMenuItem* item = fOutboundAuthMenu->Menu()->ItemAt(
			info.providerInfo.authentification_smtp);
		if (item != NULL)
			item->SetMarked(true);
		fOutboundAuthItemStart = item;

		grid->GridLayout()->AddItem(fOutboundAuthMenu->CreateLabelLayoutItem(),
			0, row);
		grid->GridLayout()->AddItem(
			fOutboundAuthMenu->CreateMenuBarLayoutItem(), 1, row++);
	}
	if (fOutboundEncryptionMenu != NULL) {
		BMenuItem* item = fOutboundEncryptionMenu->Menu()->ItemAt(
			info.providerInfo.ssl_smtp);
		if (item != NULL)
			item->SetMarked(true);
		fOutboundEncrItemStart = item;

		grid->GridLayout()->AddItem(
			fOutboundEncryptionMenu->CreateLabelLayoutItem(), 0, row);
		grid->GridLayout()->AddItem(
			fOutboundEncryptionMenu->CreateMenuBarLayoutItem(), 1, row++);
	}
	grid->GridLayout()->AddItem(BSpaceLayoutItem::CreateGlue(), 0, row);

	if (!fOutboundAccount)
		box->Hide();
}


ServerSettingsView::~ServerSettingsView()
{
	// Remove manually, as their code may be located in an add-on
	RemoveChild(fInboundAuthMenu);
	RemoveChild(fInboundEncryptionMenu);
	delete fInboundAuthMenu;
	delete fInboundEncryptionMenu;
	unload_add_on(fImageID);
}


void
ServerSettingsView::GetServerInfo(account_info& info)
{
	if (info.inboundType == IMAP) {
		info.providerInfo.imap_server = fInboundNameView->Text();
		if (fInboundEncryptionMenu != NULL) {
			BMenuItem* item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.ssl_imap
					= fInboundEncryptionMenu->Menu()->IndexOf(item);
			}
		}
	} else {
		info.providerInfo.pop_server = fInboundNameView->Text();
		BMenuItem* item = NULL;
		if (fInboundAuthMenu != NULL) {
			item = fInboundAuthMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.authentification_pop
					= fInboundAuthMenu->Menu()->IndexOf(item);
			}
		}
		if (fInboundEncryptionMenu != NULL) {
			item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item != NULL) {
				info.providerInfo.ssl_pop
					= fInboundEncryptionMenu->Menu()->IndexOf(item);
			}
		}
	}
	info.providerInfo.smtp_server = fOutboundNameView->Text();
	BMenuItem* item = NULL;
	if (fOutboundAuthMenu != NULL) {
		item = fOutboundAuthMenu->Menu()->FindMarked();
		if (item != NULL) {
			info.providerInfo.authentification_smtp
				= fOutboundAuthMenu->Menu()->IndexOf(item);
		}
	}

	if (fOutboundEncryptionMenu != NULL) {
		item = fOutboundEncryptionMenu->Menu()->FindMarked();
		if (item != NULL) {
			info.providerInfo.ssl_smtp
				= fOutboundEncryptionMenu->Menu()->IndexOf(item);
		}
	}
	_DetectMenuChanges();
}


void
ServerSettingsView::_DetectMenuChanges()
{
	bool changed = _HasMarkedChanged(fInboundAuthMenu, fInboundAuthItemStart)
		|| _HasMarkedChanged(fInboundEncryptionMenu, fInboundEncrItemStart)
		|| _HasMarkedChanged(fOutboundAuthMenu, fOutboundAuthItemStart)
		|| _HasMarkedChanged(fOutboundEncryptionMenu, fOutboundEncrItemStart);

	if (changed) {
		BMessage msg(kServerChangedMsg);
		BMessenger messenger(NULL, Window()->Looper());
		messenger.SendMessage(&msg);
	}
}


bool
ServerSettingsView::_HasMarkedChanged(BMenuField* field,
	BMenuItem* originalItem)
{
	if (field != NULL) {
		BMenuItem *item = field->Menu()->FindMarked();
		if (item != originalItem)
			return true;
	}
	return false;
}


void
ServerSettingsView::_GetAuthEncrMenu(entry_ref protocol,
	BMenuField*& authField, BMenuField*& sslField)
{
	BMailAccountSettings dummySettings;
	BView *view = new BStringView("", "Not here!");//CreateConfigView(protocol, dummySettings.InboundSettings(),
//		dummySettings, fImageId);

	authField = (BMenuField*)view->FindView("auth_method");
	sslField = (BMenuField*)view->FindView("flavor");

	view->RemoveChild(authField);
	view->RemoveChild(sslField);
	delete view;
}
