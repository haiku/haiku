/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LAUNCH_ROSTER_H
#define _LAUNCH_ROSTER_H


#include <Messenger.h>


// Flags for RegisterEvent()
enum {
	B_STICKY_EVENT	= 0x01
};


class BLaunchRoster {
public:
								BLaunchRoster();
								~BLaunchRoster();

			status_t			InitCheck() const;

			status_t			GetData(BMessage& data);
			status_t			GetData(const char* signature, BMessage& data);
			port_id				GetPort(const char* name = NULL);
			port_id				GetPort(const char* signature,
									const char* name);

			status_t			Target(const char* name, const BMessage& data,
									const char* baseName = NULL);
			status_t			Target(const char* name,
									const BMessage* data = NULL,
									const char* baseName = NULL);

			status_t			StartSession(const char* login);

			status_t			RegisterEvent(const BMessenger& source,
									const char* name, uint32 flags);
			status_t			UnregisterEvent(const BMessenger& source,
									const char* name);
			status_t			NotifyEvent(const BMessenger& source,
									const char* name);
			status_t			ResetStickyEvent(const BMessenger& source,
									const char* name);

	class Private;

private:
	friend class Private;

			void				_InitMessenger();
			status_t			_UpdateEvent(uint32 what,
									const BMessenger& source, const char* name,
									uint32 flags = 0);

private:
			BMessenger			fMessenger;
			uint32				_reserved[5];
};


#endif	// _LAUNCH_ROSTER_H
