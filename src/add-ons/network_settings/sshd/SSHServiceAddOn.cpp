/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <pwd.h>

#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>

#include <NetServer.h>
#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>


using namespace BNetworkKit;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SSHServiceAddOn"


static const uint32 kMsgToggleService = 'tgls';


class SSHServiceAddOn : public BNetworkSettingsAddOn {
public:
								SSHServiceAddOn(image_id image,
									BNetworkSettings& settings);
	virtual						~SSHServiceAddOn();

	virtual	BNetworkSettingsItem*
								CreateNextItem(uint32& cookie);
};


class ServiceView : public BView {
public:
								ServiceView(BNetworkSettings& settings);
	virtual						~ServiceView();

			void				ConfigurationUpdated(const BMessage& message);

	virtual void				MessageReceived(BMessage* message);

private:
			bool				_IsEnabled() const;
			void				_Enable();
			void				_Disable();
			void				_UpdateEnableButton();

private:
			BNetworkSettings&	fSettings;
			BButton*			fEnableButton;
};


class SSHServiceItem : public BNetworkSettingsItem {
public:
								SSHServiceItem(BNetworkSettings& settings);
	virtual						~SSHServiceItem();

	virtual	BNetworkSettingsType
								Type() const;

	virtual BListItem*			ListItem();
	virtual BView*				View();

	virtual	status_t			Revert();
	virtual bool				IsRevertable();

	virtual void				ConfigurationUpdated(const BMessage& message);

private:
			BNetworkSettings&	fSettings;
			BStringItem*		fItem;
			ServiceView*		fView;
};


// #pragma mark -


ServiceView::ServiceView(BNetworkSettings& settings)
	:
	BView("service", 0),
	fSettings(settings)
{
	BStringView* titleView = new BStringView("service",
		B_TRANSLATE("SSH server"));
	titleView->SetFont(be_bold_font);
	titleView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BTextView* descriptionView = new BTextView("description");
	descriptionView->SetText(B_TRANSLATE("The SSH server allows you to "
		"remotely access your machine with a terminal session, as well as "
		"file access using the SCP and SFTP protocols."));
	descriptionView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	descriptionView->MakeEditable(false);

	fEnableButton = new BButton("toggler", B_TRANSLATE("Enable"),
		new BMessage(kMsgToggleService));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(titleView)
		.Add(descriptionView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fEnableButton);

	SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	_UpdateEnableButton();
}


ServiceView::~ServiceView()
{
}


void
ServiceView::ConfigurationUpdated(const BMessage& message)
{
	_UpdateEnableButton();
}


void
ServiceView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgToggleService:
			if (_IsEnabled())
				_Disable();
			else
				_Enable();

			_UpdateEnableButton();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
ServiceView::_IsEnabled() const
{
	BMessage request(kMsgIsServiceRunning);
	request.AddString("name", "ssh");

	BMessenger networkServer(kNetServerSignature);
	BMessage reply;
	status_t status = networkServer.SendMessage(&request, &reply);
	if (status == B_OK)
		return reply.GetBool("running");

	return false;
}


void
ServiceView::_Enable()
{
	if (getpwnam("sshd") == NULL) {
		// We need to create a dedicated user for the service
		// The following code is copied from useradd, and should be moved
		// to a public class.
		gid_t groupID = 100;

		// Find an unused user ID
		uid_t userID = 1000;
		while (getpwuid(userID) != NULL)
			userID++;

		const char* home = "/boot/home";
		int expiration = 99999;
		int inactive = -1;
		const char* shell = "/bin/sh";
		const char* realName = "";

		int min = -1;
		int max = -1;
		int warn = 7;

		// prepare request for the registrar
		KMessage message(BPrivate::B_REG_UPDATE_USER);
		if (message.AddInt32("uid", userID) == B_OK
			&& message.AddInt32("gid", groupID) == B_OK
			&& message.AddString("name", "sshd") == B_OK
			&& message.AddString("password", "x") == B_OK
			&& message.AddString("home", home) == B_OK
			&& message.AddString("shell", shell) == B_OK
			&& message.AddString("real name", realName) == B_OK
			&& message.AddString("shadow password", "!") == B_OK
			&& message.AddInt32("last changed", time(NULL)) == B_OK
			&& message.AddInt32("min", min) == B_OK
			&& message.AddInt32("max", max) == B_OK
			&& message.AddInt32("warn", warn) == B_OK
			&& message.AddInt32("inactive", inactive) == B_OK
			&& message.AddInt32("expiration", expiration) == B_OK
			&& message.AddInt32("flags", 0) == B_OK
			&& message.AddBool("add user", true) == B_OK) {
			// send the request
			KMessage reply;
			status_t error = send_authentication_request_to_registrar(message,
				reply);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to create user: %s\n",
					strerror(error));
			}
		}
	}

	BNetworkServiceSettings settings;
	settings.SetName("ssh");
	settings.SetStandAlone(true);

	settings.AddArgument("/bin/sshd");
	settings.AddArgument("-D");

	BMessage service;
	if (settings.GetMessage(service) == B_OK)
		fSettings.AddService(service);
}


void
ServiceView::_Disable()
{
	// Since the sshd user may have been customized, we don't remove it
	fSettings.RemoveService("ssh");
}


void
ServiceView::_UpdateEnableButton()
{
	fEnableButton->SetLabel(_IsEnabled()
		? B_TRANSLATE("Disable") : B_TRANSLATE("Enable"));
}


// #pragma mark -


SSHServiceItem::SSHServiceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new BStringItem(B_TRANSLATE("SSH server"))),
	fView(NULL)
{
}


SSHServiceItem::~SSHServiceItem()
{
	if (fView->Parent() == NULL)
		delete fView;

	delete fItem;
}


BNetworkSettingsType
SSHServiceItem::Type() const
{
	return B_NETWORK_SETTINGS_TYPE_SERVICE;
}


BListItem*
SSHServiceItem::ListItem()
{
	return fItem;
}


BView*
SSHServiceItem::View()
{
	if (fView == NULL)
		fView = new ServiceView(fSettings);

	return fView;
}


status_t
SSHServiceItem::Revert()
{
	return B_OK;
}


bool
SSHServiceItem::IsRevertable()
{
	return false;
}


void
SSHServiceItem::ConfigurationUpdated(const BMessage& message)
{
	if (fView != NULL)
		fView->ConfigurationUpdated(message);
}


// #pragma mark -


SSHServiceAddOn::SSHServiceAddOn(image_id image,
	BNetworkSettings& settings)
	:
	BNetworkSettingsAddOn(image, settings)
{
}


SSHServiceAddOn::~SSHServiceAddOn()
{
}


BNetworkSettingsItem*
SSHServiceAddOn::CreateNextItem(uint32& cookie)
{
	if (cookie++ == 0)
		return new SSHServiceItem(Settings());

	return NULL;
}


// #pragma mark -


extern "C"
BNetworkSettingsAddOn*
instantiate_network_settings_add_on(image_id image, BNetworkSettings& settings)
{
	return new SSHServiceAddOn(image, settings);
}
