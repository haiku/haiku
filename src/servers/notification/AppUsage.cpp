/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Brian Hill, supernova@tycho.email
 */

#include <Message.h>

#include <AppUsage.h>
#include <NotificationReceived.h>

const type_code kTypeCode = 'ipau';


AppUsage::AppUsage()
	:
	fAppName(""),
	fSignature(""),
	fAllow(true)
{
}


AppUsage::AppUsage(const char* name, const char* signature, bool allow)
	:
	fAppName(name),
	fSignature(signature),
	fAllow(allow)
{	
}


bool
AppUsage::AllowsTypeCode(type_code code) const
{
	return code == kTypeCode;
}


status_t
AppUsage::Flatten(void* buffer, ssize_t numBytes) const
{
	BMessage msg;
	msg.AddString("name", fAppName);
	msg.AddString("signature", fSignature);
	msg.AddBool("allow", fAllow);

	if (numBytes < msg.FlattenedSize())
		return B_ERROR;

	return msg.Flatten((char*)buffer, numBytes);
}


ssize_t
AppUsage::FlattenedSize() const
{
	BMessage msg;
	msg.AddString("name", fAppName);
	msg.AddString("signature", fSignature);
	msg.AddBool("allow", fAllow);

	return msg.FlattenedSize();
}


bool
AppUsage::IsFixedSize() const
{
	return false;
}


type_code
AppUsage::TypeCode() const
{
	return kTypeCode;
}


status_t
AppUsage::Unflatten(type_code code, const void* buffer,
	ssize_t numBytes)
{
	if (code != kTypeCode)
		return B_ERROR;

	BMessage msg;
	status_t status = B_ERROR;

	status = msg.Unflatten((const char*)buffer);

	if (status == B_OK) {
		msg.FindString("name", &fAppName);
		msg.FindString("signature", &fSignature);
		msg.FindBool("allow", &fAllow);
	}
	
	return status;
}

						
const char*
AppUsage::AppName()
{
	return fAppName.String();
}


const char*
AppUsage::Signature()
{
	return fSignature.String();
}


bool
AppUsage::Allowed()
{
	return fAllow;
}


void
AppUsage::SetAllowed(bool allow)
{
	fAllow = allow;
}
