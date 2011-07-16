/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_INFO_CONTENT_HANDLER_H_
#define _PACKAGE__PACKAGE_INFO_CONTENT_HANDLER_H_


#include <package/hpkg/PackageContentHandler.h>


namespace BPackageKit {


class BPackageInfo;


namespace BHPKG {
	class BErrorOutput;
}


class BPackageInfoContentHandler : public BHPKG::BPackageContentHandler {
public:
								BPackageInfoContentHandler(
									BPackageInfo& packageInfo,
									BHPKG::BErrorOutput* errorOutput = NULL);
	virtual						~BPackageInfoContentHandler();

	virtual	status_t			HandleEntry(BHPKG::BPackageEntry* entry);
	virtual	status_t			HandleEntryAttribute(
									BHPKG::BPackageEntry* entry,
									BHPKG::BPackageEntryAttribute* attribute);
	virtual	status_t			HandleEntryDone(BHPKG::BPackageEntry* entry);

	virtual	status_t			HandlePackageAttribute(
									const BHPKG::BPackageInfoAttributeValue&
										value);

	virtual	void				HandleErrorOccurred();

protected:
			BPackageInfo&		fPackageInfo;
			BHPKG::BErrorOutput* fErrorOutput;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_INFO_CONTENT_HANDLER_H_
