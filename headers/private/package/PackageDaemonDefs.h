/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__PACKAGE_DAEMON_DEFS_H_
#define _PACKAGE__PRIVATE__PACKAGE_DAEMON_DEFS_H_


namespace BPackageKit {
namespace BPrivate {


#define PACKAGE_DAEMON_APP_SIGNATURE "application/x-vnd.haiku-package_daemon"


// message codes for requests to and replies from the daemon
enum {
	MESSAGE_GET_PACKAGES		= 'PKGG',
		// "location": int32
		//		the respective installation location constant
	MESSAGE_GET_PACKAGES_REPLY	= 'PKGR'
		// "active packages": string[]
		//		file names of the active packages (no path)
		// "inactive packages": string[]
		//		file names of the inactive packages (no path)
	
};


#endif	// _PACKAGE__PRIVATE__PACKAGE_DAEMON_DEFS_H_


}	// namespace BPrivate
}	// namespace BPackageKit
