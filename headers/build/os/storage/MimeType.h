//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file MimeType.h
	BMimeType interface declarations.
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

/* ------------------------------------------------------------- */

//!		File typing functionality.
/*! 	The BMimeType class provides access to the file typing system, which
		provides the following functionality:		
			- Basic MIME string manipulation 
			- Access to file type information in the MIME database
			- Ways to receive notifications when parts of the MIME database are updated
			- Methods to determine/help determine the types of untyped files
		
		\author <a href='mailto:tylerdauwalder@users.sf.net'>Tyler Dauwalder</a>
		\author <a href='bonefish@users.sf.net'>Ingo Weinhold</a>
		\version 0.0.0
*/
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

	/* These functions are for managing data in the meta mime file*/
 	static bool IsValid(const char *mimeType);

	/* Deprecated  Use SetTo instead. */
	status_t SetType(const char *mimeType);

	static status_t GuessMimeType(const entry_ref *file, BMimeType *type);
	static status_t GuessMimeType(const void *buffer, int32 length,
								  BMimeType *type);
	static status_t GuessMimeType(const char *filename, BMimeType *type);

private:
	BMimeType(const char *mimeType, const char *mimePath);
		// if mimePath is NULL, defaults to "/boot/home/config/settings/beos_mime/"
		
	friend class MimeTypeTest;

// Uncomment, when needed...

	friend class BAppFileInfo;
//	friend class BRoster;
//	friend class TRosterApp;
//	friend class TMimeWorker;

//	friend status_t _update_mime_info_(const char *, int32);
//	friend status_t _real_update_app_(BAppFileInfo *, const char *, bool);

//	static void _set_local_dispatch_target_(BMessenger *, void (*)(BMessage *));
//	void _touch_(); 

	virtual void _ReservedMimeType1();
	virtual void _ReservedMimeType2();
	virtual void _ReservedMimeType3();

	BMimeType &operator=(const BMimeType &);
	BMimeType(const BMimeType &);


//	void InitData(const char *type);
//	void InitData(const char *type);
//	status_t OpenFile(bool create_file = false, dev_t dev = -1) const;
//	status_t CloseFile() const;
	status_t GetSupportedTypes(BMessage *types);
	status_t SetSupportedTypes(const BMessage *types, bool fullSync = true);
	
	static status_t GetAssociatedTypes(const char *extension, BMessage *types);
	
//	void MimeChanged(int32 w, const char *type = NULL,
//					 bool large = true) const;


	char		*fType;
	BFile		*fMeta;
	void		*_unused;
	entry_ref	fRef;
	int			fWhere;
	status_t	fCStatus;
	uint32		_reserved[3];
};


#endif	// _MIME_TYPE_H


