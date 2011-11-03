/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Message.h>

#include <AppUsage.h>
#include <NotificationReceived.h>

const type_code kTypeCode = 'ipau';


AppUsage::AppUsage()
	:
	fName(""),
	fAllow(true)
{
}


AppUsage::AppUsage(const char* name, bool allow)
	:
	fName(name),
	fAllow(allow)
{	
}


AppUsage::~AppUsage()
{
	notification_t::iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++)
		delete nIt->second;
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
	msg.AddString("signature", fName);
	msg.AddBool("allow", fAllow);

	notification_t::const_iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++)
		msg.AddFlat("notification", nIt->second);

	if (numBytes < msg.FlattenedSize())
		return B_ERROR;

	return msg.Flatten((char*)buffer, numBytes);
}


ssize_t
AppUsage::FlattenedSize() const
{
	BMessage msg;
	msg.AddString("signature", fName);
	msg.AddBool("allow", fAllow);

	notification_t::const_iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++)
		msg.AddFlat("notification", nIt->second);

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
		msg.FindString("signature", &fName);
		msg.FindBool("allow", &fAllow);

		type_code type;
		int32 count = 0;

		status = msg.GetInfo("notification", &type, &count);
		if (status != B_OK)
			return status;

		for (int32 i = 0; i < count; i++) {
			NotificationReceived *notification = new NotificationReceived();
			msg.FindFlat("notification", i, notification);
			fNotifications[notification->Title()] = notification;
		}

		status = B_OK;
	}
	
	return status;
}

						
const char*
AppUsage::Name()
{
	return fName.String();
}


bool
AppUsage::Allowed(const char* title, notification_type type)
{
	bool allowed = fAllow;

	if (allowed) {
		notification_t::iterator nIt = fNotifications.find(title);
		if (nIt == fNotifications.end()) {
			allowed = true;		
			fNotifications[title] = new NotificationReceived(title, type);
		} else {
			allowed = nIt->second->Allowed();
			nIt->second->UpdateTimeStamp();
			nIt->second->SetType(type);
		}
	}

	return allowed;
}


bool
AppUsage::Allowed()
{
	return fAllow;
}


NotificationReceived*
AppUsage::NotificationAt(int32 index)
{
	notification_t::iterator nIt = fNotifications.begin();
	for (int32 i = 0; i < index; i++)
		nIt++;

	return nIt->second;
}


int32
AppUsage::Notifications()
{
	return fNotifications.size();
}


void
AppUsage::AddNotification(NotificationReceived* notification)
{
	fNotifications[notification->Title()] = notification;
}
