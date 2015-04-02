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
#define B_LAUNCH_DAEMON_PORT_NAME	"system:launch_daemon"


// message constants
enum {
	B_GET_LAUNCH_CONNECTIONS			= 'lncc',
};


}	// namespace BPrivate


#endif	// LAUNCH_DAEMON_DEFS_H

