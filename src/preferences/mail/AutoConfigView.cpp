#include "AutoConfigView.h"

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <String.h>
#include <Window.h>

#include <MailSettings.h>


AutoConfigView::AutoConfigView(BRect rect, AutoConfig &config)
	:	BBox(rect),
		fAutoConfig(config)
{
	int32 stepSize = 30;
	int32 divider = 100;
	BPoint topLeft(20, 20);
	BPoint rightDown(rect.Width() - 20, 20 + stepSize);
	
	// type view
	BPopUpMenu *chainsPopUp = new BPopUpMenu(B_EMPTY_STRING);
	const char *chainModes[] = {
		"Receive Mail Only",
		"Send Mail Only",
		"Send and Receive Mail"};
	BMenuItem *item;
	for (int32 i = 0; i < 3; i++)
		chainsPopUp->AddItem(item = new BMenuItem(chainModes[i], NULL));

	fTypeField = new BMenuField(BRect(topLeft, rightDown), NULL,
									"Account Type:", chainsPopUp);
	fTypeField->SetDivider(divider);
	fTypeField->Menu()->ItemAt(2)->SetMarked(true);
	//fAccount->SetType(2);
	AddChild(fTypeField);
	
	// protocol view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fInProtocolsField = SetupProtokolView(BRect(topLeft, rightDown));
	if (fInProtocolsField) {
		fTypeField->SetDivider(divider);
		AddChild(fInProtocolsField);
	}
	
	// search for smtp ref
	GetSMTPAddonRef(&fSMTPAddonRef);

	// email view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fEmailView = new BTextControl(BRect(topLeft, rightDown),
									"email", "E-mail Address", "",
									new BMessage(kEMailChangedMsg));
	fEmailView->SetDivider(divider);
	AddChild(fEmailView);

	// login name view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fLoginNameView = new BTextControl(BRect(topLeft, rightDown),
									"login", "Login Name", "",
									NULL);
	fLoginNameView->SetDivider(divider);
	AddChild(fLoginNameView);

	// password view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fPasswordView = new BTextControl(BRect(topLeft, rightDown),
									"password", "Password", "",
									NULL);
	fPasswordView->SetDivider(divider);
	fPasswordView->TextView()->HideTyping(true);
	AddChild(fPasswordView);

	// account view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fAccountNameView = new BTextControl(BRect(topLeft, rightDown),
									"account", "Account Name", "",
									NULL);
	fAccountNameView->SetDivider(divider);
	AddChild(fAccountNameView);

	// name view
	topLeft.y += stepSize;
	rightDown.y += stepSize;
	fNameView = new BTextControl(BRect(topLeft, rightDown),
									"name", "Real Name", "",
									NULL);
	AddChild(fNameView);
	fNameView->SetDivider(divider);
	
}


void
AutoConfigView::AttachedToWindow()
{
	fEmailView->SetTarget(this);
	fEmailView->MakeFocus(true);
}


void
AutoConfigView::MessageReceived(BMessage *msg)
{
	BString text, login;
	switch (msg->what)
	{
		case kEMailChangedMsg:
		{
			text = fLoginNameView->Text();
			if (text == "")
				ProposeUsername();
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

	BMenuItem* item = fTypeField->Menu()->FindMarked();
	info.type = fTypeField->Menu()->IndexOf(item);
	
	BString inboundProtocolName = "";
	item = fInProtocolsField->Menu()->FindMarked();
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
	
	info.outboundProtocol = fSMTPAddonRef;
	info.name = fNameView->Text();
	info.accountName = fAccountNameView->Text();
	info.email = fEmailView->Text();
	info.loginName = fLoginNameView->Text();
	info.password = fPasswordView->Text();

	return status;
}


BMenuField*
AutoConfigView::SetupProtokolView(BRect rect)
{
	BPopUpMenu *menu = new BPopUpMenu("Choose Protocol");

	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY :
											B_BEOS_ADDONS_DIRECTORY, &path);
		if (status != B_OK)
			return NULL;
	
		path.Append("mail_daemon");
		path.Append("inbound_protocols");
				
		BDirectory dir(path.Path());
		entry_ref protocolRef;
		while (dir.GetNextRef(&protocolRef) == B_OK)
		{
			char name[B_FILE_NAME_LENGTH];
			BEntry entry(&protocolRef);
			entry.GetName(name);
	
			BMenuItem *item;
			BMessage *msg = new BMessage(kProtokollChangedMsg);
			menu->AddItem(item = new BMenuItem(name, msg));
			msg->AddRef("protocol",&protocolRef);
	
			//if (*ref == protocolRef)
			item->SetMarked(true);
		}
	}

	BMenuField *protocolsMenuField = new BMenuField(rect, NULL, NULL, menu);
	protocolsMenuField->ResizeToPreferred();
	return protocolsMenuField;
}


status_t
AutoConfigView::GetSMTPAddonRef(entry_ref *ref)
{
	for (int i = 0; i < 2; i++) {
		BPath path;
		status_t status = find_directory((i == 0) ? B_USER_ADDONS_DIRECTORY :
											B_BEOS_ADDONS_DIRECTORY, &path);
		if (status != B_OK)
		{
			return B_ERROR;
		}
	
		path.Append("mail_daemon");
		path.Append("outbound_protocols");
				
		BDirectory dir(path.Path());
		
		while (dir.GetNextRef(ref) == B_OK)
		{
			return B_OK;
		}
	}
	
	return B_ERROR;
}


BString
AutoConfigView::ExtractLocalPart(const char* email)
{
	BString emailS(email);
	BString localPart;
	int32 at = emailS.FindLast("@");
	emailS.CopyInto(localPart, 0, at);
	return localPart;
}


void
AutoConfigView::ProposeUsername()
{
	const char* email = fEmailView->Text();
	provider_info info;
	status_t status = fAutoConfig.GetInfoFromMailAddress(email, &info);
	if (status == B_OK) {
		BString localPart = ExtractLocalPart(email);
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


ServerSettingsView::ServerSettingsView(BRect rect, const account_info &info)
	:	BView(rect, NULL,B_FOLLOW_ALL,0),
		fInboundAccount(false),
		fOutboundAccount(false),
		fInboundAuthMenu(NULL),
		fOutboundAuthMenu(NULL),
		fInboundEncrItemStart(NULL),
		fOutboundEncrItemStart(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	int32 divider = 120;

	switch (info.type) {
		case 0:
			fInboundAccount = true;
			break;
		case 1:
			fOutboundAccount = true;
			break;
		case 2:
			fInboundAccount = true;
			fOutboundAccount = true;
			break;
	}
	// inbound
	BRect boxRect = Bounds();
	boxRect.bottom /= 2;
	boxRect.bottom -= 5;

	BBox *box = new BBox(boxRect);
	box->SetLabel("Inbound");
	AddChild(box);

	BString serverName;
	if (info.inboundType == IMAP)
		serverName = info.providerInfo.imap_server;
	else
		serverName = info.providerInfo.pop_server;

	fInboundNameView = new BTextControl(BRect(10, 20, rect.Width() - 20, 35),
										"inbound", "Server Name",
										serverName.String(),
										new BMessage(kServerChangedMsg));
	fInboundNameView->SetDivider(divider);

	box->AddChild(fInboundNameView);
	
	GetAuthEncrMenu(info.inboundProtocol, &fInboundAuthMenu,
						&fInboundEncryptionMenu);
	if (fInboundAuthMenu) {
		int authID = info.providerInfo.authentification_pop;
		if (info.inboundType == POP)
			fInboundAuthMenu->Menu()->ItemAt(authID)->SetMarked(true);
		fInboundAuthItemStart = fInboundAuthMenu->Menu()->FindMarked();
		box->AddChild(fInboundAuthMenu);
		fInboundAuthMenu->SetDivider(divider);
		fInboundAuthMenu->MoveTo(10, 50);
	}
	if (fInboundEncryptionMenu) {
		BMenuItem *item = NULL;
		if (info.inboundType == POP) {
			item = fInboundEncryptionMenu->Menu()
					->ItemAt(info.providerInfo.ssl_pop);
			if (item)
				item->SetMarked(true);
			fInboundEncryptionMenu->MoveTo(10, 80);
		}
		if (info.inboundType == IMAP) {
			item = fInboundEncryptionMenu->Menu()
					->ItemAt(info.providerInfo.ssl_imap);
			if (item)
				item->SetMarked(true);
			fInboundEncryptionMenu->MoveTo(10, 50);
		}
		fInboundEncrItemStart = fInboundEncryptionMenu->Menu()->FindMarked();
		box->AddChild(fInboundEncryptionMenu);
		fInboundEncryptionMenu->SetDivider(divider);
	}
	
	if (!fInboundAccount) {
		fInboundNameView->SetEnabled(false);
		if (fInboundAuthMenu)
			fInboundAuthMenu->SetEnabled(false);
	}

	// outbound
	boxRect = Bounds();
	boxRect.top = boxRect.bottom / 2;
	boxRect.top += 5;

	box = new BBox(boxRect);
	box->SetLabel("Outbound");
	AddChild(box);

	serverName = info.providerInfo.smtp_server;
	fOutboundNameView = new BTextControl(BRect(10, 20, rect.Width() - 20, 30),
									"outbound", "Server Name",
									serverName.String(),
									new BMessage(kServerChangedMsg));
	fOutboundNameView->SetDivider(divider);

	box->AddChild(fOutboundNameView);
	
	GetAuthEncrMenu(info.outboundProtocol, &fOutboundAuthMenu,
						&fOutboundEncryptionMenu);
	if (fOutboundAuthMenu) {
		BMenuItem *item = fOutboundAuthMenu->Menu()
							->ItemAt(info.providerInfo.authentification_smtp);
		if (item)
			item->SetMarked(true);
		fOutboundAuthItemStart = item;
		box->AddChild(fOutboundAuthMenu);
		fOutboundAuthMenu->SetDivider(divider);
		fOutboundAuthMenu->MoveTo(10, 50);
	}
	if (fOutboundEncryptionMenu) {
		BMenuItem *item = fOutboundEncryptionMenu->Menu()
							->ItemAt(info.providerInfo.ssl_smtp);
		if (item)
			item->SetMarked(true);
		fOutboundEncrItemStart = item;
		box->AddChild(fOutboundEncryptionMenu);
		fOutboundEncryptionMenu->SetDivider(divider);
		fOutboundEncryptionMenu->MoveTo(10, 80);
	}

	if (!fOutboundAccount) {
		fOutboundNameView->SetEnabled(false);
		if (fOutboundAuthMenu)
			fOutboundAuthMenu->SetEnabled(false);
	}
	
}


void
ServerSettingsView::GetServerInfo(account_info &info)
{
	if (info.inboundType == IMAP) {
		info.providerInfo.imap_server = fInboundNameView->Text();
		if (fInboundEncryptionMenu) {
			BMenuItem* item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item)
				info.providerInfo.ssl_imap = fInboundEncryptionMenu->Menu()
												->IndexOf(item);
		}
	} else {
		info.providerInfo.pop_server = fInboundNameView->Text();
		BMenuItem* item = NULL;
		if (fInboundAuthMenu) {
			item = fInboundAuthMenu->Menu()->FindMarked();
			if (item)
				info.providerInfo.authentification_pop = fInboundAuthMenu
															->Menu()
															->IndexOf(item);
		}
		if (fInboundEncryptionMenu) {
			item = fInboundEncryptionMenu->Menu()->FindMarked();
			if (item)
				info.providerInfo.ssl_pop = fInboundEncryptionMenu->Menu()
												->IndexOf(item);
		}
	}
	info.providerInfo.smtp_server = fOutboundNameView->Text();
	BMenuItem* item = NULL;
	if (fOutboundAuthMenu) {
		item = fOutboundAuthMenu->Menu()->FindMarked();
		if (item)
			info.providerInfo.authentification_smtp = fOutboundAuthMenu->Menu()
														->IndexOf(item);
	}

	if (fOutboundEncryptionMenu) {
		item = fOutboundEncryptionMenu->Menu()->FindMarked();
		if (item)
			info.providerInfo.ssl_smtp = fOutboundEncryptionMenu->Menu()
											->IndexOf(item);
	}
	DetectMenuChanges();
}


void
ServerSettingsView::DetectMenuChanges()
{
	bool changed = false;
	if (fInboundAuthMenu) {
		BMenuItem *item = fInboundAuthMenu->Menu()->FindMarked();
		if (fInboundAuthItemStart != item)
			changed = true;
	}
	if (fInboundEncryptionMenu) {
		BMenuItem *item = fInboundEncryptionMenu->Menu()->FindMarked();
		if (fInboundEncrItemStart != item)
			changed = true;
	}
	if (fOutboundAuthMenu) {
		BMenuItem *item = fOutboundAuthMenu->Menu()->FindMarked();
		if (fOutboundAuthItemStart != item)
			changed = true;
	}
	if (fOutboundEncryptionMenu) {
		BMenuItem *item = fOutboundEncryptionMenu->Menu()->FindMarked();
		if (fOutboundEncrItemStart != item)
			changed = true;
	}
	if (changed) {
		BMessage msg(kServerChangedMsg);
		BMessenger messenger(NULL, Window()->Looper());
		messenger.SendMessage(&msg);
	}
}


void
ServerSettingsView::GetAuthEncrMenu(const entry_ref &ref,
									BMenuField **authField,
									BMenuField **sslField)
{
// TODO: These menus being NULL is not correctly handled everywhere
// in this class.
	BMessage dummyMsg;
	BView *(* instantiate_config)(BMessage *,BMessage *);
	BPath addon(&ref);
	image_id image = load_add_on(addon.Path());
	if (image < B_OK) {
		*authField = NULL;
		*sslField = NULL;
		return;
	}

	if (get_image_symbol(image,"instantiate_config_panel",
			B_SYMBOL_TYPE_TEXT,(void **)&instantiate_config) < B_OK)
	{
		unload_add_on(image);
		image = B_MISSING_SYMBOL;
		*authField = NULL;
		*sslField = NULL;
		return;
	}

	BView *view = (*instantiate_config)(&dummyMsg, &dummyMsg);
	*authField = (BMenuField *)(view->FindView("auth_method"));
	*sslField = (BMenuField *)(view->FindView("flavor"));
		
	view->RemoveChild(*authField);
	view->RemoveChild(*sslField);
	delete view;
	unload_add_on(image);
}
