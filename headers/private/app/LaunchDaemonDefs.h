/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef LAUNCH_DAEMON_DEFS_H
#define LAUNCH_DAEMON_DEFS_H


//!	launch_daemon interface


#include <Errors.h>
#include <Roster.h>


namespace BPrivate {

#define kLaunchDaemonSignature "application/x-vnd.Haiku-launch_daemon"
#define B_LAUNCH_DAEMON_PORT_NAME "system:launch_daemon"


// Message constants
enum {
	B_GET_LAUNCH_DATA			= 'lnda',
	B_LAUNCH_TARGET				= 'lntg',
	B_LAUNCH_SESSION			= 'lnse',
	B_REGISTER_SESSION_DAEMON	= 'lnrs',
	B_REGISTER_LAUNCH_EVENT		= 'lnre',
	B_UNREGISTER_LAUNCH_EVENT	= 'lnue',
	B_NOTIFY_LAUNCH_EVENT		= 'lnne',
};


}	// namespace BPrivate


#endif	// LAUNCH_DAEMON_DEFS_H

