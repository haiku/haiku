/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <NetworkSettingsAddOn.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextView.h>


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

private:
			BNetworkSettings&	fSettings;
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

	BButton* button = new BButton("toggler", B_TRANSLATE("Enable"),
		new BMessage(kMsgToggleService));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(titleView)
		.Add(descriptionView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(button);

	SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
}


ServiceView::~ServiceView()
{
}


void
ServiceView::ConfigurationUpdated(const BMessage& message)
{
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
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


bool
ServiceView::_IsEnabled() const
{
	BNetworkServiceSettings settings = fSettings.Service("ssh");
	return settings.IsEnabled();
}


void
ServiceView::_Enable()
{
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
	fSettings.RemoveService("ssh");
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
