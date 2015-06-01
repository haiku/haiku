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
#include <StringView.h>
#include <TextView.h>

#include <NetServer.h>
#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include "ServiceListItem.h"
#include "ServiceView.h"


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


class SSHServiceView : public ServiceView {
public:
								SSHServiceView(BNetworkSettings& settings);
	virtual						~SSHServiceView();

protected:
	virtual	void				Enable();
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

	virtual void				SettingsUpdated(uint32 which);

private:
			BNetworkSettings&	fSettings;
			BListItem*			fItem;
			ServiceView*		fView;
};


// #pragma mark -


SSHServiceView::SSHServiceView(BNetworkSettings& settings)
	:
	ServiceView("ssh", NULL, B_TRANSLATE("SSH server"), B_TRANSLATE(
		"The SSH server allows you to "
		"remotely access your machine with a terminal session, as well as "
		"file access using the SCP and SFTP protocols."), settings)
{
}


SSHServiceView::~SSHServiceView()
{
}


void
SSHServiceView::Enable()
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


// #pragma mark -


SSHServiceItem::SSHServiceItem(BNetworkSettings& settings)
	:
	fSettings(settings),
	fItem(new ServiceListItem("ssh", B_TRANSLATE("SSH server"), settings)),
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
		fView = new SSHServiceView(fSettings);

	return fView;
}


status_t
SSHServiceItem::Revert()
{
	return fView != NULL ? fView->Revert() : B_OK;
}


bool
SSHServiceItem::IsRevertable()
{
	return fView != NULL ? fView->IsRevertable() : false;
}


void
SSHServiceItem::SettingsUpdated(uint32 which)
{
	if (fView != NULL)
		fView->SettingsUpdated(which);
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
