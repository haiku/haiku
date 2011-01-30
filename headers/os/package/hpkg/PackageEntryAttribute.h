/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_ENTRY_ATTRIBUTE_H_
#define _PACKAGE__HPKG__PACKAGE_ENTRY_ATTRIBUTE_H_


#include <package/hpkg/PackageData.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageEntryAttribute {
public:
								BPackageEntryAttribute(const char* name);

			const char*			Name() const			{ return fName; }
			uint32				Type() const			{ return fType; }

			BPackageData&		Data()	{ return fData; }

			void				SetType(uint32 type)	{ fType = type; }

private:
			const char*			fName;
			uint32				fType;
			BPackageData			fData;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_ENTRY_ATTRIBUTE_H_
