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
								POP3ConfigView(
									const BMailProtocolSettings& settings);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

private:
			MailFileConfigView*	fFileView;
};


POP3ConfigView::POP3ConfigView(const BMailProtocolSettings& settings)
	:
	MailProtocolConfigView(B_MAIL_PROTOCOL_HAS_USERNAME
		| B_MAIL_PROTOCOL_HAS_AUTH_METHODS | B_MAIL_PROTOCOL_HAS_PASSWORD
		| B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
		| B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD
		| B_MAIL_PROTOCOL_HAS_FLAVORS
		)
{
	AddAuthMethod(B_TRANSLATE("Plain text"));
	AddAuthMethod(B_TRANSLATE("APOP"));

	AddFlavor(B_TRANSLATE("No encryption"));
	AddFlavor(B_TRANSLATE("SSL"));

	SetTo(settings);

	fFileView = new MailFileConfigView(B_TRANSLATE("Destination:"),
		"destination", false, BPrivate::default_mail_in_directory().Path());
	fFileView->SetTo(&settings, NULL);

	Layout()->AddView(fFileView, 0, Layout()->CountRows(),
		Layout()->CountColumns());
}


status_t
POP3ConfigView::SaveInto(BMailAddOnSettings& settings) const
{
	status_t status = fFileView->SaveInto(settings);
	if (status != B_OK)
		return status;

	return MailProtocolConfigView::SaveInto(settings);
}


// #pragma mark -


BMailSettingsView*
instantiate_protocol_settings_view(const BMailAccountSettings& accountSettings,
	const BMailProtocolSettings& settings)
{
	return new POP3ConfigView(settings);
}
