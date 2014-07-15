/*
 * Copyright 2002-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_DATABASE_SUPPORT_H
#define _MIME_DATABASE_SUPPORT_H


#include <Mime.h>
#include <SupportDefs.h>


class BBitmap;


namespace BPrivate {
namespace Storage {
namespace Mime {


class DatabaseLocation;


// Attribute Prefixes
extern const char* kMiniIconAttrPrefix;
extern const char* kLargeIconAttrPrefix;
extern const char* kIconAttrPrefix;

// Attribute names
extern const char* kFileTypeAttr;
extern const char* kTypeAttr;
extern const char* kAttrInfoAttr;
extern const char* kAppHintAttr;
extern const char* kShortDescriptionAttr;
extern const char* kLongDescriptionAttr;
extern const char* kFileExtensionsAttr;
extern const char* kMiniIconAttr;
extern const char* kLargeIconAttr;
extern const char* kIconAttr;
extern const char* kPreferredAppAttr;
extern const char* kSnifferRuleAttr;
extern const char* kSupportedTypesAttr;

// Attribute Datatypes
extern const int32 kFileTypeType;
extern const int32 kTypeType;
extern const int32 kAppHintType;
extern const int32 kAttrInfoType;
extern const int32 kShortDescriptionType;
extern const int32 kLongDescriptionType;
extern const int32 kFileExtensionsType;
extern const int32 kMiniIconType;
extern const int32 kLargeIconType;
extern const int32 kIconType;
extern const int32 kPreferredAppType;
extern const int32 kSnifferRuleType;
extern const int32 kSupportedTypesType;

// Message fields
extern const char* kApplicationsField;
extern const char* kExtensionsField;
extern const char* kSupertypesField;
extern const char* kSupportingAppsSubCountField;
extern const char* kSupportingAppsSuperCountField;
extern const char* kTypesField;

// Mime types
extern const char* kGenericFileType;
extern const char* kDirectoryType;
extern const char* kSymlinkType;
extern const char* kMetaMimeType;

// Error codes (to be used only by BPrivate::Storage::Mime members)
extern const status_t kMimeGuessFailureError;


DatabaseLocation* default_database_location();

// Called by BMimeType to get properly formatted icon data ready
// to be shipped off to SetIcon*() and written to the database
status_t get_icon_data(const BBitmap* icon, icon_size size, void** data,
	int32* dataSize);


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_DATABASE_SUPPORT_H
