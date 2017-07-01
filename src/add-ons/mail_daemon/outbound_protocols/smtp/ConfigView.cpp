/*
 * Copyright 2007-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002, Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <GridLayout.h>
#include <MailFilter.h>
#include <MenuField.h>
#include <TextControl.h>

#include <FileConfigView.h>
#include <MailPrivate.h>
#include <ProtocolConfigView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


using namespace BPrivate;


class SMTPConfigView : public MailProtocolConfigView {
public:
								SMTPConfigView(
									const BMailAccountSettings& settings);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

private:
			MailFileConfigView*	fFileView;
};


SMTPConfigView::SMTPConfigView(const BMailAccountSettings& settings)
	:
	MailProtocolConfigView(B_MAIL_PROTOCOL_HAS_AUTH_METHODS
		| B_MAIL_PROTOCOL_HAS_USERNAME | B_MAIL_PROTOCOL_HAS_PASSWORD
		| B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_HAS_FLAVORS
		)
{
	AddFlavor(B_TRANSLATE("Unencrypted"));
	AddFlavor(B_TRANSLATE("SSL"));

	AddAuthMethod(B_TRANSLATE("None"), false);
	AddAuthMethod(B_TRANSLATE("ESMTP"));
	AddAuthMethod(B_TRANSLATE("POP3 before SMTP"), false);

	BTextControl* control = (BTextControl*)FindView("host");
	control->SetLabel(B_TRANSLATE("SMTP server:"));

	SetTo(settings.OutboundSettings());

	fFileView = new MailFileConfigView(B_TRANSLATE("Destination:"), "path",
		false, BPrivate::default_mail_out_directory().Path());
	fFileView->SetTo(&settings.OutboundSettings(), NULL);

	Layout()->AddView(fFileView, 0, Layout()->CountRows(),
		Layout()->CountColumns());
}


status_t
SMTPConfigView::SaveInto(BMailAddOnSettings& settings) const
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
	return new SMTPConfigView(accountSettings);
}
