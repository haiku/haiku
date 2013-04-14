/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__DAEMON_DEFS_H_
#define _PACKAGE__PRIVATE__DAEMON_DEFS_H_


namespace BPackageKit {
namespace BPrivate {


#define B_PACKAGE_DAEMON_APP_SIGNATURE "application/x-vnd.haiku-package_daemon"


enum BDaemonError {
	B_DAEMON_OK
};


// message codes for requests to and replies from the daemon
enum {
	B_MESSAGE_GET_INSTALLATION_LOCATION_INFO		= 'PKLI',
		// "location": int32
		//		the respective installation location constant
	B_MESSAGE_GET_INSTALLATION_LOCATION_INFO_REPLY	= 'PKLR',
		// "base directory device": int32
		// "base directory node": int64
		// "packages directory device": int32
		// "packages directory node": int64
		// "change count": int64
		// "active packages": message[]
		//		archived BPackageInfos of the active packages
		// "inactive packages": message[]
		//		archived BPackageInfos of the inactive packages
};


#endif	// _PACKAGE__PRIVATE__DAEMON_DEFS_H_


}	// namespace BPrivate
}	// namespace BPackageKit
