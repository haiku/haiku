/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LAUNCH_ROSTER_PRIVATE_H
#define _LAUNCH_ROSTER_PRIVATE_H


#include <LaunchRoster.h>


class BLaunchRoster::Private {
public:
								Private(BLaunchRoster* roster);
								Private(BLaunchRoster& roster);

			status_t			RegisterSessionDaemon(const BMessenger& daemon);

private:
			BLaunchRoster*		fRoster;
};


#endif	// _LAUNCH_ROSTER_PRIVATE_H
