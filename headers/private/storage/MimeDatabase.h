// MimeDatabase.h

#ifndef _MIME_DATABASE_H
#define _MIME_DATABASE_H

#include <Messenger.h>
#include <StorageDefs.h>

#include <string>
#include <set>

class BNode;

namespace BPrivate {

class MimeDatabase  {
public:
	MimeDatabase(const char *databaseDir = kDefaultDatabaseDir);
	~MimeDatabase();
	
	status_t InitCheck();

	// Type management
	status_t Install(const char *type);
	status_t Delete(const char *type);
	bool IsInstalled(const char *type) const;
	
	// Get()
	status_t GetAppHint(const char *type, entry_ref *ref) const;
	status_t GetAttrInfo(const char *type, BMessage *info) const;
	status_t GetFileExtensions(const char *type, BMessage *extensions) const;
	status_t GetIcon(const char *type, BBitmap *icon, icon_size size) const;
	status_t GetIconForType(const char *type, const char *fileType, BBitmap *icon,
							icon_size which) const;
	status_t GetShortDescription(const char *type, char *description) const;
	status_t GetLongDescription(const char *type, char *description) const;
	status_t GetPreferredApp(const char *type, char *signature, app_verb verb) const;
//	status_t GetSnifferRule(BString *result) const;

	// Set()
	status_t SetAppHint(const char *type, const entry_ref *ref);
	status_t SetAttrInfo(const char *type, const BMessage *info);
	status_t SetShortDescription(const char *type, const char *description);	
	status_t SetLongDescription(const char *type, const char *description);
	status_t SetFileExtensions(const char *type, const BMessage *extensions);
	status_t SetIcon(const char *type, const void *data, size_t dataSize, icon_size which);
	status_t SetIconForType(const char *type, const char *fileType, const void *data,
							size_t dataSize, icon_size which);
	status_t SetPreferredApp(const char *type, const char *signature, app_verb verb = B_OPEN);
//	status_t SetSnifferRule(const char *);

	// Non-atomic Get()
//	static status_t GetInstalledSupertypes(BMessage *super_types);
//	static status_t GetInstalledTypes(BMessage *types);
//	static status_t GetInstalledTypes(const char *super_type,
//									  BMessage *subtypes);
//	status_t GetSupportingApps(BMessage *signatures) const;

	// Sniffer
//	static status_t CheckSnifferRule(const char *rule, BString *parseError);
//	static status_t GuessMimeType(const entry_ref *file, BMimeType *result);
//	static status_t GuessMimeType(const void *buffer, int32 length,
//								  BMimeType *result);
//	static status_t GuessMimeType(const char *filename, BMimeType *result);

	// Monitor
	status_t StartWatching(BMessenger target);
	status_t StopWatching(BMessenger target);

	// Delete()
	status_t DeleteIcon(const char *type, icon_size size);
	status_t DeletePreferredApp(const char *type, app_verb verb = B_OPEN);
	status_t DeleteAttrInfo(const char *type);
	status_t DeleteFileExtensions(const char *type);
	status_t DeleteShortDescription(const char *type);
	status_t DeleteLongDescription(const char *type);
	status_t DeleteSnifferRule(const char *type);
	status_t DeleteIconForType(const char *type, const char *fileType, icon_size which);
	status_t DeleteAppHint(const char *type);

	// Misc
	static status_t GetIconData(const BBitmap *icon, icon_size size, void **data, int32 *dataSize);
	static std::string ToLower(const char *str);

	// Constants
	static const char *kDefaultDatabaseDir;	
	
	static const char *kMiniIconAttrPrefix;
	static const char *kLargeIconAttrPrefix; 
	
	static const char *kFileTypeAttr;
	static const char *kTypeAttr;
	static const char *kAttrInfoAttr;
	static const char *kAppHintAttr;
	static const char *kShortDescriptionAttr;
	static const char *kLongDescriptionAttr;
	static const char *kFileExtensionsAttr;
	static const char *kMiniIconAttr;
	static const char *kLargeIconAttr;
	static const char *kPreferredAppAttr;
	static const char *kSnifferRuleAttr;
	static const char *kSupportedTypesAttr;
	
	static const int32 kFileTypeType;
	static const int32 kTypeType;
	static const int32 kAppHintType;
	static const int32 kAttrInfoType;
	static const int32 kShortDescriptionType;
	static const int32 kLongDescriptionType;
	static const int32 kFileExtensionsType;
	static const int32 kMiniIconType;
	static const int32 kLargeIconType;
	static const int32 kPreferredAppType;
	static const int32 kSnifferRuleType;
	static const int32 kSupportedTypesType;
	
private:
	status_t DeleteAttribute(const char *type, const char *attr);

	ssize_t ReadMimeAttr(const char *type, const char *attr, void *data,
	                       size_t len, type_code datatype) const;
	status_t ReadMimeAttrMessage(const char *type, const char *attr, BMessage *msg) const;
	status_t WriteMimeAttr(const char *type, const char *attr, const void *data,
						     size_t len, type_code datatype);	
	status_t WriteMimeAttrMessage(const char *type, const char *attr, const BMessage *msg);	

	status_t OpenType(const char *type, BNode *result) const;
	status_t OpenOrCreateType(const char *type, BNode *result);
	inline std::string TypeToFilename(const char *type) const;
	
	status_t SendMonitorUpdate(int32 which, const char *type, const char *extraType,
	                             bool largeIcon, int32 action);
	status_t SendMonitorUpdate(int32 which, const char *type, const char *extraType,
	                             int32 action);
	status_t SendMonitorUpdate(int32 which, const char *type, bool largeIcon,
	                             int32 action);
	status_t SendMonitorUpdate(int32 which, const char *type,
	                             int32 action);
	status_t SendMonitorUpdate(BMessage &msg);
	
	// General icon functions used by {Get,Set}Icon() and {Get,Set}IconForType()
	status_t GetIcon(const char *type, const char *attr, BBitmap *icon, icon_size which) const;
	status_t SetIcon(const char *type, const char *attr, const void *data, size_t dataSize, icon_size which);
	
	std::string fDatabaseDir;
	status_t fCStatus;
	std::set<BMessenger> fMonitorSet;
};

} // namespace BPrivate

#endif	// _MIME_DATABASE_H
