/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__V1__PACKAGE_INFO_CONTENT_HANDLER_H_
#define _PACKAGE__HPKG__V1__PACKAGE_INFO_CONTENT_HANDLER_H_


#include <package/hpkg/v1/PackageContentHandler.h>


namespace BPackageKit {


class BPackageInfo;


namespace BHPKG {


class BErrorOutput;


namespace V1 {


class BPackageInfoContentHandler : public BPackageContentHandler {
public:
								BPackageInfoContentHandler(
									BPackageInfo& packageInfo,
									BHPKG::BErrorOutput* errorOutput = NULL);
	virtual						~BPackageInfoContentHandler();

	virtual	status_t			HandleEntry(BPackageEntry* entry);
	virtual	status_t			HandleEntryAttribute(BPackageEntry* entry,
									BPackageEntryAttribute* attribute);
	virtual	status_t			HandleEntryDone(BPackageEntry* entry);

	virtual	status_t			HandlePackageAttribute(
									const BHPKG::BPackageInfoAttributeValue&
										value);

	virtual	void				HandleErrorOccurred();

protected:
			BPackageInfo&		fPackageInfo;
			BHPKG::BErrorOutput* fErrorOutput;
};


}	// namespace V1

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__V1__PACKAGE_INFO_CONTENT_HANDLER_H_
