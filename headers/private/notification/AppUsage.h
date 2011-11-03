/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _APP_USAGE_H
#define _APP_USAGE_H

#include <map>

#include <Entry.h>
#include <Flattenable.h>
#include <Notification.h>
#include <Roster.h>
#include <String.h>

class BMessage;
class NotificationReceived;

typedef std::map<BString, NotificationReceived*> notification_t;

class AppUsage : public BFlattenable {
public:
										AppUsage();
										AppUsage(const char* name,
											bool allow = true);
										~AppUsage();

	virtual	bool						AllowsTypeCode(type_code code) const;
	virtual	status_t					Flatten(void* buffer, ssize_t numBytes) const;
	virtual	ssize_t						FlattenedSize() const;
	virtual	bool						IsFixedSize() const;
	virtual	type_code					TypeCode() const;
	virtual	status_t					Unflatten(type_code code, const void* buffer,
											ssize_t numBytes);

			const char*					Name();
			bool						Allowed(const char* title, notification_type type);
			bool						Allowed();
			NotificationReceived*		NotificationAt(int32 index);
			int32						Notifications();
			void						AddNotification(NotificationReceived* notification);

private:
			BString						fName;
			bool						fAllow;
			notification_t				fNotifications;
};

#endif	// _APP_USAGE_H
