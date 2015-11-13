/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */


#include "IPAddressControl.h"

#include <NetworkAddress.h>


static const uint32 kMsgModified = 'txmd';


IPAddressControl::IPAddressControl(int family, const char* label,
	const char* name)
	:
	BTextControl(name, label, "", NULL),
	fFamily(family),
	fAllowEmpty(true)
{
	SetModificationMessage(new BMessage(kMsgModified));
}


IPAddressControl::~IPAddressControl()
{
}


bool
IPAddressControl::AllowEmpty() const
{
	return fAllowEmpty;
}


void
IPAddressControl::SetAllowEmpty(bool empty)
{
	fAllowEmpty = empty;
}


void
IPAddressControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
	SetTarget(this);
	_UpdateMark();
}


void
IPAddressControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgModified:
			_UpdateMark();
			break;

		default:
			BTextControl::MessageReceived(message);
			break;
	}
}


void
IPAddressControl::_UpdateMark()
{
	if (TextLength() == 0) {
		MarkAsInvalid(!fAllowEmpty);
		return;
	}

	BNetworkAddress address;
	bool success = address.SetTo(fFamily, Text(), (char*)NULL,
		B_NO_ADDRESS_RESOLUTION) == B_OK;

	MarkAsInvalid(!success);
}
