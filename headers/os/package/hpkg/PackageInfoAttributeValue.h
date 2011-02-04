/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_INFO_ATTRIBUTE_VALUE_H_
#define _PACKAGE__HPKG__PACKAGE_INFO_ATTRIBUTE_VALUE_H_


#include <SupportDefs.h>

#include <package/PackageArchitecture.h>
#include <package/PackageInfoAttributes.h>
#include <package/PackageResolvableOperator.h>
#include <package/PackageResolvableType.h>


namespace BPackageKit {

namespace BHPKG {


struct BPackageVersionData {
			const char*			major;
			const char*			minor;
			const char*			micro;
			uint8				release;
};


struct BPackageResolvableData {
			BPackageResolvableType	type;
			const char*			name;
			bool				haveVersion;
			BPackageVersionData	version;
};


struct BPackageResolvableExpressionData {
			const char*			name;
			bool				haveOpAndVersion;
			BPackageResolvableOperator	op;
			BPackageVersionData	version;
};


struct BPackageInfoAttributeValue {
			union {
				uint64			unsignedInt;
				const char*		string;
				BPackageVersionData version;
				BPackageResolvableData resolvable;
				BPackageResolvableExpressionData resolvableExpression;
			};
			uint8				attributeIndex;

public:
	inline						BPackageInfoAttributeValue();

	inline	void				SetTo(BPackageInfoAttributeIndex index,
									uint8 value);
	inline	void				SetTo(BPackageInfoAttributeIndex index,
									const char* value);
};


BPackageInfoAttributeValue::BPackageInfoAttributeValue()
	:
	attributeIndex(B_PACKAGE_INFO_ENUM_COUNT)
{
}


void
BPackageInfoAttributeValue::SetTo(BPackageInfoAttributeIndex index, uint8 value)
{
	attributeIndex = index;
	unsignedInt = value;
}


void
BPackageInfoAttributeValue::SetTo(BPackageInfoAttributeIndex index,
	const char* value)
{
	attributeIndex = index;
	string = value;
}


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_ATTRIBUTE_VALUE_H_
