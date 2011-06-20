/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <FileConfigView.h>
#include <MailAddon.h>
#include <MailPrivate.h>
#include <ProtocolConfigView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ConfigView"


class POP3ConfigView : public BMailProtocolConfigView {
public:
								POP3ConfigView(MailAddonSettings& settings,
									BMailAccountSettings& accountSettings);
			status_t			Archive(BMessage *into, bool deep = true) const;
			void				GetPreferredSize(float *width, float *height);
private:
			BMailFileConfigView*	fFileView;
};


POP3ConfigView::POP3ConfigView(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
	:
	BMailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME
		| B_MAIL_PROTOCOL_HAS_AUTH_METHODS | B_MAIL_PROTOCOL_HAS_PASSWORD
		| B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
		| B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD
#if USE_SSL
		| B_MAIL_PROTOCOL_HAS_FLAVORS
#endif
		)
{
	AddAuthMethod("Plain text");
	AddAuthMethod("APOP");

#if USE_SSL
	AddFlavor("No encryption");
	AddFlavor("SSL");
#endif

	SetTo(settings);

	fFileView =  new BMailFileConfigView(B_TRANSLATE("Destination:"),
		"destination", false, BPrivate::default_mail_in_directory().Path());
	fFileView->SetTo(&settings.Settings(), NULL);
	AddChild(fFileView);
	float w, h;
	BMailProtocolConfigView::GetPreferredSize(&w, &h);
	fFileView->MoveBy(0, h - 10);
	GetPreferredSize(&w, &h);
	ResizeTo(w, h);
}


status_t
POP3ConfigView::Archive(BMessage *into, bool deep) const
{
	fFileView->Archive(into, deep);
	return BMailProtocolConfigView::Archive(into, deep);
}


void
POP3ConfigView::GetPreferredSize(float* width, float* height)
{
	BMailProtocolConfigView::GetPreferredSize(width, height);
	*height += 20;
}


BView*
instantiate_config_panel(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
{
	return new POP3ConfigView(settings, accountSettings);
}

