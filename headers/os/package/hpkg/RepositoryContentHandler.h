/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_
#define _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_


#include <SupportDefs.h>


namespace BPackageKit {


class BPackageInfo;
class BRepositoryInfo;


namespace BHPKG {


class BRepositoryContentHandler {
public:
	virtual						~BRepositoryContentHandler();

	virtual	status_t			HandleRepositoryInfo(
									const BRepositoryInfo& info) = 0;
	virtual	status_t			HandlePackage(const BPackageInfo& info) = 0;
	virtual	status_t			HandlePackagesDone() = 0;

	virtual	void				HandleErrorOccurred() = 0;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_
