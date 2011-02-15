/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_
#define _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_


#include <SupportDefs.h>

#include <package/hpkg/PackageContentHandler.h>


namespace BPackageKit {


class BRepositoryInfo;


namespace BHPKG {


class BRepositoryContentHandler : public BPackageContentHandler {
public:
	virtual	status_t			HandleRepositoryInfo(
									const BRepositoryInfo& info) = 0;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__REPOSITORY_CONTENT_HANDLER_H_
