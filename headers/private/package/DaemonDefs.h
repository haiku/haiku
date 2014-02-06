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
	B_DAEMON_OK = 0,
	B_DAEMON_INSTALLATION_LOCATION_BUSY,
	B_DAEMON_CHANGE_COUNT_MISMATCH,
	B_DAEMON_BAD_REQUEST,
	B_DAEMON_NO_SUCH_PACKAGE,
	B_DAEMON_PACKAGE_ALREADY_EXISTS
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

	B_MESSAGE_COMMIT_TRANSACTION		= 'PKTC',
		// "location": int32
		//		the respective installation location constant
		// "change count": int64
		//		the expected change count of the installation location; fail,
		//		if something has changed in the meantime
		// "transaction": string
		//		name of the transaction directory (subdirectory of the
		//		administrative directory) where to-be-activated packages live
		// "activate": string[]
		//		file names of the packages to activate; must be in the
		//		transaction directory
		// "deactivate": string[]
		//		file names of the packages to activate; must be in the
		//		transaction directory
	B_MESSAGE_COMMIT_TRANSACTION_REPLY	= 'PKTR'
		// "error": int32
		//		regular error code or BDaemonError describing how committing
		//		the transaction went
		// "error message": string
		//		[error case only] gives some additional information what went
		//		wrong; optional
		// "error package": string
		//		[error case only] file name of the package causing the error,
		//		if any in particarly; optional
		// "old state": string
		//		name of the directory (subdirectory of the administrative
		//		directory) containing the deactivated packages
};


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__DAEMON_DEFS_H_
