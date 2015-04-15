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

			status_t			GetData(const char* signature, BMessage& data);
			port_id				GetPort(const char* signature,
									const char* name = NULL);

private:
			void				_InitMessenger();

private:
			BMessenger			fMessenger;
			uint32				_reserved[5];
};

// global BLaunchRoster instance
extern const BLaunchRoster* be_launch_roster;


#endif	// _LAUNCH_ROSTER_H
