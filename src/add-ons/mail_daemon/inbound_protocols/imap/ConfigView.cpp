/*
 * Copyright 2001-2013, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <Button.h>
#include <Catalog.h>
#include <GridLayout.h>
#include <Path.h>
#include <TextControl.h>

#include <MailFilter.h>

#include <FileConfigView.h>
#include <ProtocolConfigView.h>
#include <MailPrivate.h>

#include "FolderConfigWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "imap_config"


using namespace BPrivate;


const uint32 kMsgOpenIMAPFolder = '&OIF';


class ConfigView : public MailProtocolConfigView {
public:
								ConfigView(
									const BMailAccountSettings& accountSettings,
									const BMailProtocolSettings& settings);
	virtual						~ConfigView();

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();

private:
			MailFileConfigView*	fFileView;
			BButton*			fFolderButton;
};


ConfigView::ConfigView(const BMailAccountSettings& accountSettings,
	const BMailProtocolSettings& settings)
	:
	MailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME
		| B_MAIL_PROTOCOL_HAS_PASSWORD | B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
		| B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD
	 	| B_MAIL_PROTOCOL_HAS_FLAVORS
	 )
{
	AddFlavor(B_TRANSLATE("No encryption"));
	AddFlavor(B_TRANSLATE("SSL"));

	SetTo(settings);

	((BControl*)(FindView("leave_mail_on_server")))->SetValue(B_CONTROL_ON);
	((BControl*)(FindView("leave_mail_on_server")))->Hide();

	fFolderButton = new BButton("IMAP Folders", B_TRANSLATE(
		"Configure IMAP Folders"), new BMessage(kMsgOpenIMAPFolder));
	Layout()->AddView(fFolderButton, 0, Layout()->CountRows(), 2);

	BPath defaultFolder = BPrivate::default_mail_directory();
	defaultFolder.Append(accountSettings.Name());

	fFileView = new MailFileConfigView(B_TRANSLATE("Destination:"),
		"destination", false, defaultFolder.Path());
	fFileView->SetTo(&settings, NULL);

	Layout()->AddView(fFileView, 0, Layout()->CountRows(),
		Layout()->CountColumns());
}


ConfigView::~ConfigView()
{
}


status_t
ConfigView::SaveInto(BMailAddOnSettings& settings) const
{
	status_t status = fFileView->SaveInto(settings);
	if (status != B_OK)
		return status;

	return MailProtocolConfigView::SaveInto(settings);
}


void
ConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenIMAPFolder:
		{
			// Retrieve current settings
			BMailAddOnSettings settings;
			SaveInto(settings);

			BWindow* window = new FolderConfigWindow(Window()->Frame(),
				settings);
			window->Show();
			break;
		}

		default:
			MailProtocolConfigView::MessageReceived(message);
	}
}


void
ConfigView::AttachedToWindow()
{
	fFolderButton->SetTarget(this);
}


// #pragma mark -


BMailSettingsView*
instantiate_protocol_settings_view(const BMailAccountSettings& accountSettings,
	const BMailProtocolSettings& settings)
{
	return new ConfigView(accountSettings, settings);
}
