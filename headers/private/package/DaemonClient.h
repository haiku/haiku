/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__DAEMON_CLIENT_H_
#define _PACKAGE__PRIVATE__DAEMON_CLIENT_H_


#include <Messenger.h>
#include <package/PackageDefs.h>


namespace BPackageKit {


class BInstallationLocationInfo;
class BPackageInfoSet;


namespace BPrivate {


class BDaemonClient {
public:
								BDaemonClient();
								~BDaemonClient();

			status_t			GetInstallationLocationInfo(
									BPackageInstallationLocation location,
									BInstallationLocationInfo& _info);

private:
			status_t			_InitMessenger();
			status_t			_ExtractPackageInfoSet(const BMessage& message,
									const char* field, BPackageInfoSet& _infos);

private:
			BMessenger			fDaemonMessenger;
};


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__DAEMON_CLIENT_H_
