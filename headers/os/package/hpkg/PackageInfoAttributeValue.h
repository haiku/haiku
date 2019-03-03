/*
 * Copyright 2009-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_INFO_ATTRIBUTE_VALUE_H_
#define _PACKAGE__HPKG__PACKAGE_INFO_ATTRIBUTE_VALUE_H_


#include <SupportDefs.h>

#include <string.h>

#include <package/PackageArchitecture.h>
#include <package/PackageInfoAttributes.h>
#include <package/PackageResolvableOperator.h>
#include <package/WritableFileUpdateType.h>


namespace BPackageKit {

namespace BHPKG {


struct BPackageVersionData {
	const char*			major;
	const char*			minor;
	const char*			micro;
	const char*			preRelease;
	uint32				revision;
};


struct BPackageResolvableData {
	const char*			name;
	bool				haveVersion;
	bool				haveCompatibleVersion;
	BPackageVersionData	version;
	BPackageVersionData	compatibleVersion;
};


struct BPackageResolvableExpressionData {
	const char*			name;
	bool				haveOpAndVersion;
	BPackageResolvableOperator	op;
	BPackageVersionData	version;
};


struct BGlobalWritableFileInfoData {
	const char*				path;
	BWritableFileUpdateType	updateType;
	bool					isDirectory;
};


struct BUserSettingsFileInfoData {
	const char*	path;
	const char*	templatePath;
	bool		isDirectory;
};


struct BUserData {
	const char*			name;
	const char*			realName;
	const char*			home;
	const char*			shell;
	const char* const*	groups;
	size_t				groupCount;
};


struct BPackageInfoAttributeValue {
			union {
				uint64			unsignedInt;
				const char*		string;
				BPackageVersionData version;
				BPackageResolvableData resolvable;
				BPackageResolvableExpressionData resolvableExpression;
				BGlobalWritableFileInfoData globalWritableFileInfo;
				BUserSettingsFileInfoData userSettingsFileInfo;
				BUserData		user;
			};
			BPackageInfoAttributeID	attributeID;

public:
								BPackageInfoAttributeValue();

			void				SetTo(BPackageInfoAttributeID id,
									uint8 value);
			void				SetTo(BPackageInfoAttributeID id,
									const char* value);

			void				Clear();
};


inline
BPackageInfoAttributeValue::BPackageInfoAttributeValue()
{
	Clear();
}


inline void
BPackageInfoAttributeValue::SetTo(BPackageInfoAttributeID id, uint8 value)
{
	attributeID = id;
	unsignedInt = value;
}


inline void
BPackageInfoAttributeValue::SetTo(BPackageInfoAttributeID id,
	const char* value)
{
	attributeID = id;
	string = value;
}


inline void
BPackageInfoAttributeValue::Clear()
{
	unsignedInt = 0;
	string = NULL;
	memset(&version, 0, sizeof(version));
	memset(&resolvable, 0, sizeof(resolvable));
	memset(&resolvableExpression, 0, sizeof(resolvableExpression));
	memset(&globalWritableFileInfo, 0, sizeof(globalWritableFileInfo));
	memset(&userSettingsFileInfo, 0, sizeof(userSettingsFileInfo));
	memset(&user, 0, sizeof(user));
	attributeID = B_PACKAGE_INFO_ENUM_COUNT;
}


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_ATTRIBUTE_VALUE_H_
