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

	/* These functions are for managing data in the meta mime file*/
	status_t Install(const char *type);
	status_t Delete(const char *type);
	bool IsInstalled(const char *type) const;
	status_t GetIcon(const char *type, BBitmap *icon, icon_size size) const;
	status_t GetPreferredApp(const char *type, char *signature, app_verb verb = B_OPEN) const;
//	status_t GetAttrInfo(BMessage *info) const;
//	status_t GetFileExtensions(BMessage *extensions) const;
	status_t GetShortDescription(const char *type, char *description) const;
	status_t GetLongDescription(const char *type, char *description) const;
//	status_t GetSupportingApps(BMessage *signatures) const;

	status_t SetIcon(const char *type, const BBitmap *icon, icon_size size);
	status_t SetIcon(const char *type, icon_size size, const void *data, size_t dataSize);
	status_t SetPreferredApp(const char *type, const char *signature, app_verb verb = B_OPEN);
//	status_t SetAttrInfo(const BMessage *info);
//	status_t SetFileExtensions(const BMessage *extensions);
	status_t SetShortDescription(const char *type, const char *description);	
	status_t SetLongDescription(const char *type, const char *description);

//	static status_t GetInstalledSupertypes(BMessage *super_types);
//	static status_t GetInstalledTypes(BMessage *types);
//	static status_t GetInstalledTypes(const char *super_type,
//									  BMessage *subtypes);
//	static status_t GetWildcardApps(BMessage *wild_ones);
//	static bool IsValid(const char *mimeType);
//
	status_t GetAppHint(const char *type, entry_ref *ref) const;
	status_t SetAppHint(const char *type, const entry_ref *ref);

	/* for application signatures only.*/
//	status_t GetIconForType(const char *type, BBitmap *icon,
//							icon_size which) const;
//	status_t SetIconForType(const char *type, const BBitmap *icon,
//							icon_size which);

	/* sniffer rule manipulation */
//	status_t GetSnifferRule(BString *result) const;
//	status_t SetSnifferRule(const char *);
//	static status_t CheckSnifferRule(const char *rule, BString *parseError);

	/* calls to ask the sniffer to identify the MIME type of a file or data in
	   memory */
//	static status_t GuessMimeType(const entry_ref *file, BMimeType *result);
//	static status_t GuessMimeType(const void *buffer, int32 length,
//								  BMimeType *result);
//	static status_t GuessMimeType(const char *filename, BMimeType *result);

	status_t StartWatching(BMessenger target);
	status_t StopWatching(BMessenger target);

	static status_t GetIconData(const BBitmap *icon, icon_size size, void **data, int32 *dataSize);

	static const char *kDefaultDatabaseDir;
private:
	static const char *kMiniIconAttribute; 
	static const char *kLargeIconAttribute; 

	ssize_t ReadMimeAttr(const char *type, const char *attr, void *data,
	                       size_t len, type_code datatype) const;
	status_t WriteMimeAttr(const char *type, const char *attr, const void *data,
						     size_t len, type_code datatype);	
	status_t OpenType(const char *type, BNode *result) const;
	status_t OpenOrCreateType(const char *type, BNode *result);
	inline std::string TypeToFilename(const char *type) const;
	
	status_t SendMonitorUpdate(int32 which, const char *type, const char *extraType, bool largeIcon);
	status_t SendMonitorUpdate(int32 which, const char *type, const char *extraType);
	status_t SendMonitorUpdate(int32 which, const char *type, bool largeIcon);
	status_t SendMonitorUpdate(int32 which, const char *type);
	status_t SendMonitorUpdate(BMessage &msg);
	
	
	std::string fDatabaseDir;
	status_t fCStatus;
	std::set<BMessenger> fMonitorSet;
};

} // namespace BPrivate

#endif	// _MIME_DATABASE_H
