/*
 * Copyright 2007-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGE_ITEM_H
#define PACKAGE_ITEM_H


#include <stdio.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <String.h>


// Local macro for the parser debug output
//#define DEBUG_PARSER
#ifdef DEBUG_PARSER
#	define parser_debug(format, args...) fprintf(stderr, format, ##args)
#else
#	define parser_debug(format, args...)
#endif


enum {
	P_INSTALL_PATH = 0,
	P_SYSTEM_PATH,
	P_USER_PATH
};

// Existing item overwriting policy of a single file
enum {
	P_EXISTS_ASK = 0,
	P_EXISTS_OVERWRITE,
	P_EXISTS_SKIP,
	P_EXISTS_ABORT,
	P_EXISTS_NONE
};

const uint32 P_NO_KIND = 0;
const uint32 P_KIND_SCRIPT = 1;
const uint32 P_KIND_FILE = 2;
const uint32 P_KIND_DIRECTORY = 3;
const uint32 P_KIND_SYM_LINK = 4;

extern status_t inflate_data(uint8* in, uint32 inSize, uint8* out,
	uint32 outSize);


struct ItemState {
	ItemState() : policy(P_EXISTS_NONE), status(B_NO_INIT) {}
	~ItemState() {}

	inline void Reset(int32 currentPolicy)
	{
		destination.Unset();
		parent.Unset();
		status = B_NO_INIT;
		policy = currentPolicy;
	}

	BPath		destination;
	BDirectory	parent;
	uint8		policy;
	status_t	status;
};


class PackageItem {
public:
							PackageItem(BFile* parent, const BString& path,
								uint8 type, uint32 ctime, uint32 mtime,
								uint64 offset = 0, uint64 size = 0);
	virtual					~PackageItem();

	virtual	status_t		DoInstall(const char* path = NULL,
								ItemState *state = NULL) = 0;
	virtual	void			SetTo(BFile* parent, const BString& path,
								uint8 type, uint32 ctime, uint32 mtime,
								uint64 offset = 0, uint64 size = 0);
	virtual	const uint32	ItemKind() {return P_NO_KIND;};

	protected:
			status_t		InitPath(const char* path, BPath* destination);
			status_t		HandleAttributes(BPath* destination, BNode* node,
								const char* header);

			status_t		ParseAttribute(uint8* buffer, BNode* node,
								char** attrName, uint32* nameSize,
								uint32* attrType, uint8** attrData,
								uint64* dataSize, uint8** temp,
								uint64* tempSize, uint64* attrCSize,
								uint64* attrOSize, bool* attrStarted,
								bool* done);
			status_t		SkipAttribute(uint8 *buffer, bool *attrStarted, 
								bool *done);
			status_t		ParseData(uint8* buffer, BFile* file,
								uint64 originalSize, bool* done);

			BString			fPath;
			uint64			fOffset;
			uint64			fSize;
			uint8			fPathType;
			uint32			fCreationTime;
			uint32			fModificationTime;

			BFile*			fPackage;
};


class PackageDirectory : public PackageItem {
public:
							PackageDirectory(BFile* parent, const BString& path,
								uint8 type, uint32 ctime, uint32 mtime,
								uint64 offset = 0, uint64 size = 0);

	virtual	status_t		DoInstall(const char* path = NULL,
								ItemState *state = NULL);
	virtual	const uint32	ItemKind();
};


class PackageScript : public PackageItem {
public:
							PackageScript(BFile* parent, uint64 offset = 0, 
								uint64 size = 0, uint64 originalSize = 0);

	virtual	status_t		DoInstall(const char* path = NULL,
								ItemState *state = NULL);
	virtual	const uint32	ItemKind();

			thread_id		GetThreadId() { return fThreadId; }
			void			SetThreadId(thread_id id) { fThreadId = id; }

private:
			status_t		_ParseScript(uint8 *buffer, uint64 originalSize, 
								bool *done);
			status_t		_RunScript(uint8 *script, uint32 len);

			uint64			fOriginalSize;
			thread_id		fThreadId;
};


class PackageFile : public PackageItem {
public:
							PackageFile(BFile* parent, const BString& path,
								uint8 type, uint32 ctime, uint32 mtime,
								uint64 offset, uint64 size, uint64 originalSize,
								uint32 platform, const BString& mime,
								const BString& signature, uint32 mode);

	virtual	status_t		DoInstall(const char* path = NULL,
								ItemState *state = NULL);
	virtual	const uint32	ItemKind();

private:
			uint64			fOriginalSize;
			uint32			fPlatform;
			uint32			fMode;

			BString			fMimeType;
			BString			fSignature;
};


class PackageLink : public PackageItem {
public:
							PackageLink(BFile* parent, const BString& path,
								const BString& link, uint8 type,  uint32 ctime,
								uint32 mtime, uint32 mode, uint64 offset = 0,
								uint64 size = 0);

	virtual	status_t		DoInstall(const char* path = NULL,
								ItemState *state = NULL);
	virtual	const uint32	ItemKind();

private:
			uint32			fMode;
			BString			fLink;
};

#endif	// PACKAGE_ITEM_H
