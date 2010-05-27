/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_RECEIVED_H
#define _NOTIFICATION_RECEIVED_H

#include <Flattenable.h>
#include <Roster.h>
#include <String.h>

class NotificationReceived : public BFlattenable {
public:
								NotificationReceived();
								NotificationReceived(const char* title, notification_type type,
									bool enabled = true);
								~NotificationReceived();

	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Flatten(void *buffer, ssize_t numBytes) const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	status_t			Unflatten(type_code code, const void *buffer,
								ssize_t numBytes);

			const char*			Title();
			notification_type	Type();
			void				SetType(notification_type type);
			time_t				LastReceived();
			bool				Allowed();

			void				SetTimeStamp(time_t time);
			void				UpdateTimeStamp();

private:
			BString				fTitle;
			notification_type	fType;
			bool				fEnabled;
			time_t				fLastReceived;
};

#endif	// _NOTIFICATION_RECEIVED_H
