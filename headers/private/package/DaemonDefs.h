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
		//		a BTransactionError describing how committing the transaction
		//		went
		// "system error": int32
		//		a status_t for the operation that failed; B_ERROR, if n/a
		// "error package": string
		//		[error case only] file name of the package causing the error,
		//		if any in particarly; optional
		// "path1": string
		//		[error case only] first path specific to the error
		// "path2": string
		//		[error case only] second path specific to the error
		// "string1": string
		//		[error case only] first string specific to the error
		// "string2": string
		//		[error case only] second string specific to the error
		// "old state": string
		//		[success case only] name of the directory (subdirectory of the
		//		administrative directory) containing the deactivated packages
		// "issues": message[]
		//		A list of non-critical issues that occurred while performing the
		//		package activation. On success the user should be notified about
		//		these. Each contains:
		//		"type": int32
		//			a BTransactionIssue::BType specifying the kind of issue
		//		"package": string
		//			file name of the package which the issue is related to
		//		"path1": string
		//			first path specific to the issue
		//		"path2": string
		//			second path specific to the issue
		//		"system error": int32
		//			a status_t for the operation that failed; B_OK, if n/a
		//		"exit code": int32
		//			a exit code of the program that failed; 0, if n/a
};


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__DAEMON_DEFS_H_
