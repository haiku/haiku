/*
 * Copyright 2019 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Rob Gill, <rrobgill@protonmail.com>
 */


#include "HostnameView.h"

#include <stdio.h>
#include <string.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <SeparatorView.h>
#include <StringView.h>


static const int32 kMsgApply = 'aply';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HostnameView"


HostnameView::HostnameView(BNetworkSettingsItem* item)
	:
	BView("hostname", 0),
	fItem(item)
{
	BStringView* titleView = new BStringView("title",
		B_TRANSLATE("Hostname settings"));
	titleView->SetFont(be_bold_font);
	titleView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fHostname = new BTextControl(B_TRANSLATE("Hostname:"), "", NULL);
	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage(kMsgApply));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(titleView)
		.Add(fHostname)
		.AddGlue()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fApplyButton);

	_LoadHostname();
}


HostnameView::~HostnameView()
{
}

status_t
HostnameView::Revert()
{
	return B_OK;
}

bool
HostnameView::IsRevertable() const
{
	return false;
}


void
HostnameView::AttachedToWindow()
{
	fApplyButton->SetTarget(this);
}


void
HostnameView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgApply:
			if (_SaveHostname() == B_OK)
				fItem->NotifySettingsUpdated();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


status_t
HostnameView::_LoadHostname()
{
	BString fHostnameString;
	char hostname[MAXHOSTNAMELEN];

	if (gethostname(hostname, MAXHOSTNAMELEN) == 0) {

		fHostnameString.SetTo(hostname, MAXHOSTNAMELEN);
		fHostname->SetText(fHostnameString);

		return B_OK;
	}

	return B_ERROR;
}


status_t
HostnameView::_SaveHostname()
{
	BString hostnamestring("");

	size_t hostnamelen(strlen(fHostname->Text()));

	if (hostnamelen == 0)
		return B_ERROR;

	if (hostnamelen > MAXHOSTNAMELEN) {
		hostnamestring.Truncate(MAXHOSTNAMELEN);
		hostnamelen = MAXHOSTNAMELEN;
	}

	hostnamestring << fHostname->Text();

	if (sethostname(hostnamestring, hostnamelen) == 0)
		return B_OK;

	return B_ERROR;
}
