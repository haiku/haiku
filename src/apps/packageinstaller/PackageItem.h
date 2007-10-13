/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEITEM_H
#define PACKAGEITEM_H

#include <String.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <stdio.h>

//#define DEBUG_PARSER

// Local macro for the parser debug output
#ifdef DEBUG_PARSER
	#define parser_debug(format, args...) fprintf(stderr, format, ##args)
#else
	#define parser_debug(format, args...)
#endif


class PkgDirectory;

// Since files are derive from directories, which is not too obvious,
// we define a type PkgItem to use for base type iterations
typedef PkgDirectory PkgItem;


enum {
	P_INSTALL_PATH = 0,
	P_SYSTEM_PATH,
	P_USER_PATH
};


status_t inflate_data(uint8 *in, uint32 in_size, uint8 *out, uint32 out_size);


class PkgDirectory {
	public:
		PkgDirectory(BFile *parent, BString path, uint8 type, uint32 ctime, 
				uint32 mtime, uint64 offset = 0, uint64 size = 0);
		virtual ~PkgDirectory();

		virtual status_t WriteToPath(const char *path = NULL, BPath *final = NULL);
		virtual void SetTo(BFile *parent, BString path, uint8 type, 
				uint32 ctime, uint32 mtime, uint64 offset = 0, uint64 size = 0);

	protected:
		int32 _ItemExists(const char *name);
		status_t _InitPath(const char *path, BPath *destination);
		status_t _HandleAttributes(BPath *destination, BNode *node, 
				const char *header);

		inline status_t _ParseAttribute(uint8 *buffer, BNode *node, char **attrName,
				uint32 *nameSize, uint32 *attrType, uint8 **attrData, uint64 *dataSize, 
				uint8 **temp, uint64 *tempSize, uint64 *attrCSize, uint64 *attrOSize, 
				bool *attrStarted, bool *done);
		inline status_t _ParseData(uint8 *buffer, BFile *file, uint64 originalSize,
				bool *done);

		BString fPath;
		uint64 fOffset;
		uint64 fSize;
		uint8	 fPathType;
		uint32 fCreationTime;
		uint32 fModificationTime;

		BFile *fPackage;
};


class PkgFile : public PkgItem {
	public:
		PkgFile(BFile *parent, BString path, uint8 type, uint32 ctime, 
				uint32 mtime, uint64 offset, uint64 size, uint64 originalSize, 
				uint32 platform, BString mime, BString signature, uint32 mode);
		~PkgFile();

		status_t WriteToPath(const char *path = NULL, BPath *final = NULL);

	private:
		uint64 fOriginalSize;
		uint32 fPlatform;
		uint32 fMode;
		
		BString fMimeType;
		BString fSignature;
};


class PkgLink : public PkgItem {
	public:
		PkgLink(BFile *parent, BString path, BString link, uint8 type, 
				uint32 ctime, uint32 mtime, uint32 mode, uint64 offset = 0, 
				uint64 size = 0);
		~PkgLink();

		status_t WriteToPath(const char *path = NULL, BPath *final = NULL);

	private:
		uint32 fMode;

		BString fLink;
};


#endif

