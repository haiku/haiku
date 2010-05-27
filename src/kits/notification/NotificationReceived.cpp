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
#include <Notification.h>
#include <NotificationReceived.h>

const type_code kTypeCode = 'ipnt';


NotificationReceived::NotificationReceived()
	:
	fTitle(""),
	fType(B_INFORMATION_NOTIFICATION),
	fEnabled(false),
	fLastReceived(time(NULL))
{
}


NotificationReceived::NotificationReceived(const char* title,
	notification_type type, bool enabled)
	:
	fTitle(title),
	fType(type),
	fEnabled(enabled),
	fLastReceived(time(NULL))
{
}


NotificationReceived::~NotificationReceived()
{
}


bool
NotificationReceived::AllowsTypeCode(type_code code) const
{
	return code == kTypeCode;
}


status_t
NotificationReceived::Flatten(void* buffer, ssize_t numBytes) const
{
	BMessage msg;
	msg.AddString("notify_title", fTitle);
	msg.AddInt32("notify_type", (int32)fType);
	msg.AddInt32("notify_lastreceived", (int32)fLastReceived);
	msg.AddBool("notify_enabled", fEnabled);

	if (numBytes < msg.FlattenedSize())
		return B_ERROR;
		
	return msg.Flatten((char*)buffer, numBytes);
}


ssize_t
NotificationReceived::FlattenedSize() const
{
	BMessage msg;
	msg.AddString("notify_title", fTitle);
	msg.AddInt32("notify_type", (int32)fType);
	msg.AddInt32("notify_lastreceived", (int32)fLastReceived);
	msg.AddBool("notify_enabled", fEnabled);
	
	return msg.FlattenedSize();
}


bool
NotificationReceived::IsFixedSize() const
{
	return false;
}


type_code
NotificationReceived::TypeCode() const
{
	return kTypeCode;
}


status_t
NotificationReceived::Unflatten(type_code code, const void* buffer,
	ssize_t numBytes)
{
	if (code != kTypeCode)
		return B_ERROR;

	BMessage msg;
	status_t error = msg.Unflatten((const char*)buffer);

	if (error == B_OK) {
		msg.FindString("notify_title", &fTitle);
		msg.FindInt32("notify_type", (int32 *)&fType);
		msg.FindInt32("notify_lastreceived", (int32 *)&fLastReceived);
		msg.FindBool("notify_enabled", &fEnabled);
	}

	return error;
}


const char*
NotificationReceived::Title()
{
	return fTitle.String();
}


notification_type
NotificationReceived::Type()
{
	return fType;
}


void
NotificationReceived::SetType(notification_type type)
{
	fType = type;
}


time_t
NotificationReceived::LastReceived()
{
	return fLastReceived;
}


bool
NotificationReceived::Allowed()
{
	return fEnabled;
}


void
NotificationReceived::UpdateTimeStamp()
{
	fLastReceived = time(NULL);
}


void
NotificationReceived::SetTimeStamp(time_t time)
{
	fLastReceived = time;
}
