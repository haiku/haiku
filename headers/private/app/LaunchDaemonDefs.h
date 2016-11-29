/*
 * Copyright 2015-2016, Haiku, Inc. All Rights Reserved.
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
	B_STOP_LAUNCH_TARGET		= 'lnst',
	B_LAUNCH_JOB				= 'lnjo',
	B_ENABLE_LAUNCH_JOB			= 'lnje',
	B_STOP_LAUNCH_JOB			= 'lnsj',
	B_LAUNCH_SESSION			= 'lnse',
	B_REGISTER_SESSION_DAEMON	= 'lnrs',
	B_REGISTER_LAUNCH_EVENT		= 'lnre',
	B_UNREGISTER_LAUNCH_EVENT	= 'lnue',
	B_NOTIFY_LAUNCH_EVENT		= 'lnne',
	B_RESET_STICKY_LAUNCH_EVENT	= 'lnRe',
	B_GET_LAUNCH_TARGETS		= 'lngt',
	B_GET_LAUNCH_JOBS			= 'lngj',
	B_GET_LAUNCH_TARGET_INFO	= 'lntI',
	B_GET_LAUNCH_JOB_INFO		= 'lnjI',
};


}	// namespace BPrivate


#endif	// LAUNCH_DAEMON_DEFS_H

