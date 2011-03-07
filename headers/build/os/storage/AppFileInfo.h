//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file AppFileInfo.h
	BAppFileInfo and related structures' interface declarations.
*/
#ifndef _APP_FILE_INFO_H
#define _APP_FILE_INFO_H

#include <NodeInfo.h>

class BBitmap;
class BFile;
class BMessage;
class BResources;

struct	version_info {
	uint32	major;
	uint32	middle;
	uint32	minor;
	uint32	variety;
	uint32	internal;
	char	short_info[64];
	char	long_info[256];
};

enum info_location {
	B_USE_ATTRIBUTES		= 0x1,
	B_USE_RESOURCES			= 0x2,
	B_USE_BOTH_LOCATIONS	= 0x3	// == B_USE_ATTRIBUTES | B_USE_RESOURCES
};

enum version_kind {
	B_APP_VERSION_KIND,
	B_SYSTEM_VERSION_KIND
};

/*!	\brief Executable meta information handling.
	The BAppFileInfo class provides access to meta data that can be associated
	with executables, libraries and add-ons.

	\author <a href='bonefish@users.sf.net'>Ingo Weinhold</a>
	\version 0.0.0
*/
class BAppFileInfo: public BNodeInfo {
public:
	BAppFileInfo();
	BAppFileInfo(BFile *file);
	virtual ~BAppFileInfo();

	status_t SetTo(BFile *file);

	virtual status_t GetType(char *type) const;
	virtual status_t SetType(const char *type);

	status_t GetSignature(char *signature) const;
	status_t SetSignature(const char *signature);

	status_t GetCatalogEntry(char *catalogEntry) const;
	status_t SetCatalogEntry(const char *catalogEntry);

	status_t GetAppFlags(uint32 *flags) const;
	status_t SetAppFlags(uint32 flags);

	status_t GetSupportedTypes(BMessage *types) const;
	status_t SetSupportedTypes(const BMessage *types, bool syncAll);
	status_t SetSupportedTypes(const BMessage *types);
	bool IsSupportedType(const char *type) const;
	bool Supports(BMimeType *type) const;

	virtual status_t GetIcon(BBitmap *icon, icon_size which) const;
	virtual status_t SetIcon(const BBitmap *icon, icon_size which);

	status_t GetVersionInfo(version_info *info, version_kind kind) const;
	status_t SetVersionInfo(const version_info *info, version_kind kind);

	status_t GetIconForType(const char *type, BBitmap *icon,
							icon_size which) const;
	status_t SetIconForType(const char *type, const BBitmap *icon,
							icon_size which);

	void SetInfoLocation(info_location location);
	bool IsUsingAttributes() const;
	bool IsUsingResources() const;

private:
// uncomment when needed
//	friend status_t _update_mime_info_(const char *, int32);
//	friend status_t _real_update_app_(BAppFileInfo *, const char *, bool);
//	friend status_t _query_for_app_(BMimeType *, const char *, entry_ref *,
//									version_info *);
//	friend class BRoster;

	virtual void _ReservedAppFileInfo1();
	virtual void _ReservedAppFileInfo2();
	virtual void _ReservedAppFileInfo3();

// uncomment when needed
//	static status_t SetSupTypesForAll(BMimeType *, const BMessage *);

	BAppFileInfo &operator=(const BAppFileInfo &);
	BAppFileInfo(const BAppFileInfo &);

// uncomment when needed
//	status_t _SetSupportedTypes(const BMessage *types);
//	status_t UpdateFromRsrc();
//	status_t RealUpdateRsrcToAttr();
//	status_t UpdateMetaMime(const char *path, bool force,
//							uint32 *changesMask) const;
//	bool IsApp();
	status_t GetMetaMime(BMimeType *meta) const;

	status_t _ReadData(const char *name, int32 id, type_code type,
					   void *buffer, size_t bufferSize,
					   size_t &bytesRead, void **allocatedBuffer = NULL) const;
	status_t _WriteData(const char *name, int32 id, type_code type,
						const void *buffer, size_t bufferSize,
						bool findID = false);
	status_t _RemoveData(const char *name, type_code type);

	BResources		*fResources;
	info_location	fWhere;
	uint32			_reserved[2];
};

#endif	// _APP_FILE_INFO_H
