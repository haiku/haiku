/*
 * Copyright 2007-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <GridLayout.h>
#include <MailFilter.h>

#include <FileConfigView.h>
#include <MailPrivate.h>
#include <ProtocolConfigView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


using namespace BPrivate;


class POP3ConfigView : public MailProtocolConfigView {
public:
								POP3ConfigView(BMailAccountSettings& settings);
			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			MailFileConfigView*	fFileView;
};


POP3ConfigView::POP3ConfigView(BMailAccountSettings& settings)
	:
	MailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME
		| B_MAIL_PROTOCOL_HAS_AUTH_METHODS | B_MAIL_PROTOCOL_HAS_PASSWORD
		| B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
		| B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD
#if USE_SSL
		| B_MAIL_PROTOCOL_HAS_FLAVORS
#endif
		)
{
	AddAuthMethod(B_TRANSLATE("Plain text"));
	AddAuthMethod(B_TRANSLATE("APOP"));

#if USE_SSL
	AddFlavor(B_TRANSLATE("No encryption"));
	AddFlavor(B_TRANSLATE("SSL"));
#endif

	SetTo(settings.InboundSettings());

	fFileView = new MailFileConfigView(B_TRANSLATE("Destination:"),
		"destination", false, BPrivate::default_mail_in_directory().Path());
	fFileView->SetTo(&settings.InboundSettings(), NULL);

	Layout()->AddView(fFileView, 0, Layout()->CountRows(),
		Layout()->CountColumns());
}


status_t
POP3ConfigView::Archive(BMessage* into, bool deep) const
{
	fFileView->Archive(into, deep);
	return MailProtocolConfigView::Archive(into, deep);
}


// #pragma mark -


BView*
instantiate_protocol_config_panel(BMailAccountSettings& settings)
{
	return new POP3ConfigView(settings);
}
