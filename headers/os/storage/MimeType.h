/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MIME_TYPE_H
#define _MIME_TYPE_H


#include <SupportDefs.h>
#include <StorageDefs.h>

#include <Messenger.h>
#include <Mime.h>
#include <File.h>
#include <Entry.h>

class BBitmap;
class BResources;
class BAppFileInfo;
class BMessenger;

namespace BPrivate {
	class MimeDatabase;
	namespace Storage {
		namespace Mime {
			class CreateAppMetaMimeThread;
		}
	}
}

enum app_verb {
	B_OPEN
};

extern const char *B_APP_MIME_TYPE;		// platform dependent
extern const char *B_PEF_APP_MIME_TYPE;	// "application/x-be-executable"
extern const char *B_PE_APP_MIME_TYPE;	// "application/x-vnd.be-peexecutable"
extern const char *B_ELF_APP_MIME_TYPE;	// "application/x-vnd.be-elfexecutable"
extern const char *B_RESOURCE_MIME_TYPE;// "application/x-be-resource"
extern const char *B_FILE_MIME_TYPE;	// "application/octet-stream"

/* ------------------------------------------------------------- */

// MIME Monitor BMessage::what value
enum {
	B_META_MIME_CHANGED = 'MMCH'
};

// MIME Monitor "be:which" values
enum {
	B_ICON_CHANGED					= 0x00000001,
	B_PREFERRED_APP_CHANGED			= 0x00000002,
	B_ATTR_INFO_CHANGED				= 0x00000004,
	B_FILE_EXTENSIONS_CHANGED		= 0x00000008,
	B_SHORT_DESCRIPTION_CHANGED		= 0x00000010,
	B_LONG_DESCRIPTION_CHANGED		= 0x00000020,
	B_ICON_FOR_TYPE_CHANGED			= 0x00000040,
	B_APP_HINT_CHANGED				= 0x00000080,
	B_MIME_TYPE_CREATED				= 0x00000100,
	B_MIME_TYPE_DELETED				= 0x00000200,
	B_SNIFFER_RULE_CHANGED			= 0x00000400,
	B_SUPPORTED_TYPES_CHANGED		= 0x00000800,

	B_EVERYTHING_CHANGED			= (int)0xFFFFFFFF
};

// MIME Monitor "be:action" values
enum {
	B_META_MIME_MODIFIED	= 'MMMD',
	B_META_MIME_DELETED 	= 'MMDL',
};

class BMimeType	{
	public:
		BMimeType();
		BMimeType(const char *mimeType);
		virtual ~BMimeType();

		status_t SetTo(const char *mimeType);
		void Unset();
		status_t InitCheck() const;

		/* these functions simply perform string manipulations*/
		const char *Type() const;
		bool IsValid() const;
		bool IsSupertypeOnly() const;
		status_t GetSupertype(BMimeType *superType) const;

		bool operator==(const BMimeType &type) const;
		bool operator==(const char *type) const;

		bool Contains(const BMimeType *type) const;

		/* These functions are for managing data in the meta mime file */
		status_t Install();
		status_t Delete();
		bool IsInstalled() const;
		status_t GetIcon(BBitmap* icon, icon_size size) const;
		status_t GetIcon(uint8** _data, size_t* _size) const;
		status_t GetPreferredApp(char *signature, app_verb verb = B_OPEN) const;
		status_t GetAttrInfo(BMessage *info) const;
		status_t GetFileExtensions(BMessage *extensions) const;
		status_t GetShortDescription(char *description) const;
		status_t GetLongDescription(char *description) const;
		status_t GetSupportingApps(BMessage *signatures) const;

		status_t SetIcon(const BBitmap *icon, icon_size size);
		status_t SetIcon(const uint8* data, size_t size);
		status_t SetPreferredApp(const char *signature, app_verb verb = B_OPEN);
		status_t SetAttrInfo(const BMessage *info);
		status_t SetFileExtensions(const BMessage *extensions);
		status_t SetShortDescription(const char *description);
		status_t SetLongDescription(const char *description);

		static status_t GetInstalledSupertypes(BMessage *supertypes);
		static status_t GetInstalledTypes(BMessage *types);
		static status_t GetInstalledTypes(const char* supertype,
							BMessage* subtypes);
		static status_t GetWildcardApps(BMessage* wildcardApps);
		static bool IsValid(const char *mimeType);

		status_t GetAppHint(entry_ref *ref) const;
		status_t SetAppHint(const entry_ref *ref);

		/* for application signatures only. */
		status_t GetIconForType(const char* type, BBitmap* icon,
							icon_size which) const;
		status_t GetIconForType(const char* type, uint8** _data,
							size_t* _size) const;
		status_t SetIconForType(const char* type, const BBitmap* icon,
							icon_size which);
		status_t SetIconForType(const char* type, const uint8* data,
							size_t size);

		/* sniffer rule manipulation */
		status_t GetSnifferRule(BString *result) const;
		status_t SetSnifferRule(const char *);
		static status_t CheckSnifferRule(const char *rule, BString *parseError);

		/* calls to ask the sniffer to identify the MIME type of a file or data in
		   memory */
		static status_t GuessMimeType(const entry_ref *file, BMimeType *type);
		static status_t GuessMimeType(const void *buffer, int32 length,
							BMimeType *type);
		static status_t GuessMimeType(const char *filename, BMimeType *type);

		static status_t StartWatching(BMessenger target);
		static status_t StopWatching(BMessenger target);

		/* Deprecated. Use SetTo() instead. */
		status_t SetType(const char *mimeType);

	private:
		BMimeType(const char* mimeType, const char* mimePath);
			// if mimePath is NULL, defaults to "/boot/home/config/settings/beos_mime/"

		friend class MimeTypeTest;
			// for testing only

		friend class BAppFileInfo;
		friend class BPrivate::Storage::Mime::CreateAppMetaMimeThread;

		virtual void _ReservedMimeType1();
		virtual void _ReservedMimeType2();
		virtual void _ReservedMimeType3();

		BMimeType& operator=(const BMimeType& source);
		BMimeType(const BMimeType& source);

		status_t GetSupportedTypes(BMessage* types);
		status_t SetSupportedTypes(const BMessage* types, bool fullSync = true);

		static status_t GetAssociatedTypes(const char* extension, BMessage* types);

	private:
		char*		fType;
		BFile*		fMeta;
		void*		_unused;
		entry_ref	fRef;
		status_t	fCStatus;
		uint32		_reserved[4];
};

#endif	// _MIME_TYPE_H
