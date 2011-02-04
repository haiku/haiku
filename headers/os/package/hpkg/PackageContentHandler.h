/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_CONTENT_HANDLER_H_
#define _PACKAGE__HPKG__PACKAGE_CONTENT_HANDLER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageAttributeValue;
class BPackageEntry;
class BPackageEntryAttribute;
class BPackageInfoAttributeValue;


class BLowLevelPackageContentHandler {
public:
	virtual						~BLowLevelPackageContentHandler();

	virtual	status_t			HandleAttribute(const char* attributeName,
									const BPackageAttributeValue& value,
									void* parentToken, void*& _token) = 0;
	virtual	status_t			HandleAttributeDone(const char* attributeName,
									const BPackageAttributeValue& value,
									void* token) = 0;

	virtual	void				HandleErrorOccurred() = 0;
};


class BPackageContentHandler {
public:
	virtual						~BPackageContentHandler();

	virtual	status_t			HandleEntry(BPackageEntry* entry) = 0;
	virtual	status_t			HandleEntryAttribute(BPackageEntry* entry,
									BPackageEntryAttribute* attribute) = 0;
	virtual	status_t			HandleEntryDone(BPackageEntry* entry) = 0;

	virtual	status_t			HandlePackageAttribute(
									const BPackageInfoAttributeValue& value
									) = 0;

	virtual	void				HandleErrorOccurred() = 0;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_CONTENT_HANDLER_H_
