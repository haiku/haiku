/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


#include <TextControl.h>

#include <Catalog.h>
#include <FileConfigView.h>
#include <MailAddon.h>
#include <MenuField.h>
#include <MailPrivate.h>
#include <ProtocolConfigView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ConfigView"


class SMTPConfigView : public BMailProtocolConfigView {
public:
								SMTPConfigView(MailAddonSettings& settings,
									BMailAccountSettings& accountSettings);
			status_t			Archive(BMessage *into, bool deep = true) const;
			void				GetPreferredSize(float *width, float *height);
private:
			BMailFileConfigView* fFileView;
};


SMTPConfigView::SMTPConfigView(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
	:
	BMailProtocolConfigView(B_MAIL_PROTOCOL_HAS_AUTH_METHODS
		| B_MAIL_PROTOCOL_HAS_USERNAME | B_MAIL_PROTOCOL_HAS_PASSWORD
		| B_MAIL_PROTOCOL_HAS_HOSTNAME
#ifdef USE_SSL
		| B_MAIL_PROTOCOL_HAS_FLAVORS
#endif
		)
{
#ifdef USE_SSL
	AddFlavor(B_TRANSLATE("Unencrypted"));
	AddFlavor(B_TRANSLATE("SSL"));
	AddFlavor(B_TRANSLATE("STARTTLS"));
#endif

	AddAuthMethod(B_TRANSLATE("None"), false);
	AddAuthMethod(B_TRANSLATE("ESMTP"));
	AddAuthMethod(B_TRANSLATE("POP3 before SMTP"), false);

	BTextControl *control = (BTextControl *)(FindView("host"));
	control->SetLabel(B_TRANSLATE("SMTP server:"));

	// Reset the dividers after changing one
	float widestLabel = 0;
	for (int32 i = CountChildren(); i-- > 0;) {
		if (BTextControl *text = dynamic_cast<BTextControl *>(ChildAt(i)))
			widestLabel = MAX(widestLabel,text->StringWidth(text->Label()) + 5);
	}
	for (int32 i = CountChildren(); i-- > 0;) {
		if (BTextControl *text = dynamic_cast<BTextControl *>(ChildAt(i)))
			text->SetDivider(widestLabel);
	}

	BMenuField *field = (BMenuField *)(FindView("auth_method"));
	field->SetDivider(widestLabel);

	SetTo(settings);

	fFileView = new BMailFileConfigView(B_TRANSLATE("Destination:"), "path",
		false, BPrivate::default_mail_out_directory().Path());
	fFileView->SetTo(&settings.Settings(), NULL);
	AddChild(fFileView);
	float w, h;
	BMailProtocolConfigView::GetPreferredSize(&w, &h);
	fFileView->MoveBy(0, h - 10);
	GetPreferredSize(&w, &h);
	ResizeTo(w, h);
}


status_t
SMTPConfigView::Archive(BMessage *into, bool deep) const
{
	fFileView->Archive(into, deep);
	return BMailProtocolConfigView::Archive(into, deep);
}


void
SMTPConfigView::GetPreferredSize(float* width, float* height)
{
	BMailProtocolConfigView::GetPreferredSize(width, height);
	*width += 20;
	*height += 20;
}


BView*
instantiate_config_panel(MailAddonSettings& settings,
	BMailAccountSettings& accountSettings)
{
	return new SMTPConfigView(settings, accountSettings);
}

