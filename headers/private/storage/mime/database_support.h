//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file database_support.h
	Private mime database function and constant declarations
*/

#ifndef _MIME_DATABASE_SUPPORT_H
#define _MIME_DATABASE_SUPPORT_H

#include <StorageDefs.h>

#include <string>

class BNode;
class BMessage;

namespace BPrivate {
namespace Storage {
namespace Mime {

// Database directory
extern const std::string kDatabaseDir;
extern const std::string kApplicationDatabaseDir;
	
// Attribute Prefixes
extern const char *kMiniIconAttrPrefix;
extern const char *kLargeIconAttrPrefix; 
	
// Attribute names
extern const char *kFileTypeAttr;
extern const char *kTypeAttr;
extern const char *kAttrInfoAttr;
extern const char *kAppHintAttr;
extern const char *kShortDescriptionAttr;
extern const char *kLongDescriptionAttr;
extern const char *kFileExtensionsAttr;
extern const char *kMiniIconAttr;
extern const char *kLargeIconAttr;
extern const char *kPreferredAppAttr;
extern const char *kSnifferRuleAttr;
extern const char *kSupportedTypesAttr;

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
extern const int32 kPreferredAppType;
extern const int32 kSnifferRuleType;
extern const int32 kSupportedTypesType;

// Message fields
extern const char *kApplicationsField;
extern const char *kExtensionsField;
extern const char *kSupertypesField;
extern const char *kSupportingAppsSubCountField;
extern const char *kSupportingAppsSuperCountField;
extern const char *kTypesField;

// Mime types
extern const char *kGenericFileType;
extern const char *kDirectoryType;
extern const char *kSymlinkType;
extern const char *kMetaMimeType;

// Error codes (to be used only by BPrivate::Storage::Mime members)
extern const status_t kMimeGuessFailureError;

std::string type_to_filename(const char *type);

status_t open_type(const char *type, BNode *result);
status_t open_or_create_type(const char *type, BNode *result, bool *didCreate);

ssize_t read_mime_attr(const char *type, const char *attr, void *data,
                       size_t len, type_code datatype);
status_t read_mime_attr_message(const char *type, const char *attr, BMessage *msg);
status_t read_mime_attr_string(const char *type, const char *attr, BString *str);
status_t write_mime_attr(const char *type, const char *attr, const void *data,
						     size_t len, type_code datatype, bool *didCreate);	
status_t write_mime_attr_message(const char *type, const char *attr,
									const BMessage *msg, bool *didCreate);	

status_t delete_attribute(const char *type, const char *attr);

} // namespace Mime
} // namespace Storage
} // namespace BPrivate

#endif	// _MIME_DATABASE_SUPPORT_H
