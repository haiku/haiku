/*
 * Copyright 2001-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <Button.h>
#include <Catalog.h>
#include <TextControl.h>

#include <MailAddon.h>
#include <Path.h>

#include <FileConfigView.h>
#include <ProtocolConfigView.h>
#include <MailPrivate.h>

#include "IMAPFolderConfig.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "imap_config"


const uint32 kMsgOpenIMAPFolder = '&OIF';


class IMAPConfig : public BMailProtocolConfigView {
public:
								IMAPConfig(MailAddonSettings& settings,
									BMailAccountSettings& accountSettings);
	virtual						~IMAPConfig();
	virtual	status_t			Archive(BMessage *into, bool deep = true) const;
	virtual	void				GetPreferredSize(float *width, float *height);

	virtual	void				MessageReceived(BMessage* message);
	virtual void				AttachedToWindow();

private:
			BMailFileConfigView*	fFileView;
			BButton*			fIMAPFolderButton;
			MailAddonSettings&	fAddonSettings;
};


IMAPConfig::IMAPConfig(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
	:
	BMailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME
		| B_MAIL_PROTOCOL_HAS_PASSWORD | B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
		| B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD
#ifdef USE_SSL
	 	| B_MAIL_PROTOCOL_HAS_FLAVORS
#endif
	 ),
	 fAddonSettings(settings)
{
#ifdef USE_SSL
	AddFlavor(B_TRANSLATE("No encryption"));
	AddFlavor(B_TRANSLATE("SSL"));
#endif

	SetTo(settings);

	((BControl*)(FindView("leave_mail_on_server")))->SetValue(B_CONTROL_ON);
	((BControl*)(FindView("leave_mail_on_server")))->Hide();

	BRect frame = FindView("delete_remote_when_local")->Frame();

	((BControl*)(FindView("delete_remote_when_local")))->SetEnabled(true);
	((BControl*)(FindView("delete_remote_when_local")))->MoveBy(0, -25);

	fIMAPFolderButton = new BButton(frame, "IMAP Folders", B_TRANSLATE(
		"IMAP Folders"), new BMessage(kMsgOpenIMAPFolder));
	AddChild(fIMAPFolderButton);

	frame.right -= 10;

	BPath defaultFolder = BPrivate::default_mail_directory();
	defaultFolder.Append(accountSettings.Name());

	fFileView = new BMailFileConfigView(B_TRANSLATE("Destination:"),
		"destination", false, defaultFolder.Path());
	fFileView->SetTo(&settings.Settings(), NULL);
	AddChild(fFileView);
	fFileView->MoveBy(0, frame.bottom + 5);

	ResizeToPreferred();
}


IMAPConfig::~IMAPConfig()
{
}


status_t
IMAPConfig::Archive(BMessage *into, bool deep) const
{
	fFileView->Archive(into, deep);
	return BMailProtocolConfigView::Archive(into, deep);
}


void
IMAPConfig::GetPreferredSize(float *width, float *height)
{
	BMailProtocolConfigView::GetPreferredSize(width,height);
	*height -= 20;
}


void
IMAPConfig::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgOpenIMAPFolder:
		{
			BMessage settings;
			Archive(&settings);
			BWindow* window = new FolderConfigWindow(Window()->Frame(),
				settings);
			window->Show();
			break;
		}

		default:
			BMailProtocolConfigView::MessageReceived(message);
	}
}


void
IMAPConfig::AttachedToWindow()
{
	fIMAPFolderButton->SetTarget(this);
}


// #pragma mark -


BView*
instantiate_config_panel(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
{
	return new IMAPConfig(settings, accountSettings);
}
