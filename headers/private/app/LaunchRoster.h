/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LAUNCH_ROSTER_H
#define _LAUNCH_ROSTER_H


#include <Messenger.h>


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

			status_t			Target(const char* name, BMessage& data,
									const char* baseName = NULL);

			status_t			StartSession(const char* login);

	class Private;

private:
	friend class Private;

			void				_InitMessenger();

private:
			BMessenger			fMessenger;
			uint32				_reserved[5];
};

// global BLaunchRoster instance
extern const BLaunchRoster* be_launch_roster;


#endif	// _LAUNCH_ROSTER_H
