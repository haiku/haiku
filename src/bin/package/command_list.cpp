/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Entry.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/StandardErrorOutput.h>
#include <package/hpkg/v1/PackageContentHandler.h>
#include <package/hpkg/v1/PackageEntry.h>
#include <package/hpkg/v1/PackageEntryAttribute.h>
#include <package/hpkg/v1//PackageReader.h>

#include <package/PackageInfo.h>

#include "package.h"
#include "PackageInfoPrinter.h"


using namespace BPackageKit;
using BPackageKit::BHPKG::BErrorOutput;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BStandardErrorOutput;


struct VersionPolicyV1 {
	typedef BPackageKit::BHPKG::V1::BPackageContentHandler
		PackageContentHandler;
	typedef BPackageKit::BHPKG::V1::BPackageEntry PackageEntry;
	typedef BPackageKit::BHPKG::V1::BPackageEntryAttribute
		PackageEntryAttribute;
	typedef BPackageKit::BHPKG::V1::BPackageReader PackageReader;

	static inline uint64 PackageDataSize(
		const BPackageKit::BHPKG::V1::BPackageData& data)
	{
		return data.UncompressedSize();
	}

	static inline status_t InitReader(PackageReader& packageReader,
		const char* fileName)
	{
		return packageReader.Init(fileName);
	}
};

struct VersionPolicyV2 {
	typedef BPackageKit::BHPKG::BPackageContentHandler PackageContentHandler;
	typedef BPackageKit::BHPKG::BPackageEntry PackageEntry;
	typedef BPackageKit::BHPKG::BPackageEntryAttribute PackageEntryAttribute;
	typedef BPackageKit::BHPKG::BPackageReader PackageReader;

	static inline uint64 PackageDataSize(
		const BPackageKit::BHPKG::BPackageData& data)
	{
		return data.Size();
	}

	static inline status_t InitReader(PackageReader& packageReader,
		const char* fileName)
	{
		return packageReader.Init(fileName,
			BPackageKit::BHPKG
				::B_HPKG_READER_DONT_PRINT_VERSION_MISMATCH_MESSAGE);
	}
};


enum ListMode {
	LIST_ALL,
	LIST_PATHS_ONLY,
	LIST_META_INFO_ONLY
};


template<typename VersionPolicy>
struct PackageContentListHandler : VersionPolicy::PackageContentHandler {
	PackageContentListHandler(bool listEntries, bool listAttributes)
		:
		fPrinter(),
		fLevel(0),
		fListEntries(listEntries),
		fListAttribute(listEntries && listAttributes)
	{
	}

	virtual status_t HandleEntry(typename VersionPolicy::PackageEntry* entry)
	{
		if (!fListEntries)
			return B_OK;

		fLevel++;

		int indentation = (fLevel - 1) * 2;
		printf("%*s", indentation, "");

		// name and size
		printf("%-*s", indentation < 32 ? 32 - indentation : 0, entry->Name());
		printf("  %8llu",
			(unsigned long long)VersionPolicy::PackageDataSize(entry->Data()));

		// time
		struct tm* time = localtime(&entry->ModifiedTime().tv_sec);
		printf("  %04d-%02d-%02d %02d:%02d:%02d",
			1900 + time->tm_year, time->tm_mon + 1, time->tm_mday,
			time->tm_hour, time->tm_min, time->tm_sec);

		// file type
		mode_t mode = entry->Mode();
		if (S_ISREG(mode))
			printf("  -");
		else if (S_ISDIR(mode))
			printf("  d");
		else if (S_ISLNK(mode))
			printf("  l");
		else
			printf("  ?");

		// permissions
		char buffer[4];
		printf("%s", _PermissionString(buffer, mode >> 6,
			(mode & S_ISUID) != 0));
		printf("%s", _PermissionString(buffer, mode >> 3,
			(mode & S_ISGID) != 0));
		printf("%s", _PermissionString(buffer, mode, false));

		// print the symlink path
		if (S_ISLNK(mode))
			printf("  -> %s", entry->SymlinkPath());

		printf("\n");
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(
		typename VersionPolicy::PackageEntry* entry,
		typename VersionPolicy::PackageEntryAttribute* attribute)
	{
		if (!fListAttribute)
			return B_OK;

		int indentation = fLevel * 2;
		printf("%*s<", indentation, "");
		printf("%-*s  %8llu", indentation < 31 ? 31 - indentation : 0,
			attribute->Name(),
			(unsigned long long)VersionPolicy::PackageDataSize(
				attribute->Data()));

		uint32 type = attribute->Type();
		if (isprint(type & 0xff) && isprint((type >> 8) & 0xff)
			 && isprint((type >> 16) & 0xff) && isprint(type >> 24)) {
			printf("  '%c%c%c%c'", int(type >> 24), int((type >> 16) & 0xff),
				int((type >> 8) & 0xff), int(type & 0xff));
		} else
			printf("  %#" B_PRIx32, type);

		printf(">\n");
		return B_OK;
	}

	virtual status_t HandleEntryDone(
		typename VersionPolicy::PackageEntry* entry)
	{
		if (!fListEntries)
			return B_OK;

		fLevel--;
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		if (value.attributeID == B_PACKAGE_INFO_NAME)
			printf("package-attributes:\n");

		if (!fPrinter.PrintAttribute(value)) {
			printf("*** Invalid package attribute section: unexpected "
				"package attribute id %d encountered\n", value.attributeID);
			return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	static const char* _PermissionString(char* buffer, uint32 mode, bool sticky)
	{
		buffer[0] = (mode & 0x4) != 0 ? 'r' : '-';
		buffer[1] = (mode & 0x2) != 0 ? 'w' : '-';

		if ((mode & 0x1) != 0)
			buffer[2] = sticky ? 's' : 'x';
		else
			buffer[2] = '-';

		buffer[3] = '\0';
		return buffer;
	}

	static void _PrintPackageVersion(const BPackageVersionData& version)
	{
		printf("%s", BPackageVersion(version).ToString().String());
	}

private:
	PackageInfoPrinter	fPrinter;
	int					fLevel;
	bool				fListEntries;
	bool				fListAttribute;
};


template<typename VersionPolicy>
struct PackageContentListPathsHandler : VersionPolicy::PackageContentHandler {
	PackageContentListPathsHandler()
		:
		fPathComponents()
	{
	}

	virtual status_t HandleEntry(typename VersionPolicy::PackageEntry* entry)
	{
		fPathComponents.Add(entry->Name());
		printf("%s\n", fPathComponents.Join("/").String());
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(
		typename VersionPolicy::PackageEntry* entry,
		typename VersionPolicy::PackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(
		typename VersionPolicy::PackageEntry* entry)
	{
		fPathComponents.Remove(fPathComponents.CountStrings() - 1);
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	BStringList	fPathComponents;
};


template<typename VersionPolicy>
static void
do_list(const char* packageFileName, bool listAttributes, ListMode listMode,
	bool ignoreVersionError)
{
	// open package
	BStandardErrorOutput errorOutput;
	typename VersionPolicy::PackageReader packageReader(&errorOutput);
	status_t error = VersionPolicy::InitReader(packageReader, packageFileName);
	if (error != B_OK) {
		if (ignoreVersionError && error == B_MISMATCHED_VALUES)
			return;
		exit(1);
	}

	// list
	switch (listMode) {
		case LIST_PATHS_ONLY:
		{
			PackageContentListPathsHandler<VersionPolicy> handler;
			error = packageReader.ParseContent(&handler);
			break;
		}

		case LIST_ALL:
		case LIST_META_INFO_ONLY:
		{
			PackageContentListHandler<VersionPolicy> handler(
				listMode != LIST_META_INFO_ONLY, listAttributes);
			error = packageReader.ParseContent(&handler);
		}
	}

	if (error != B_OK)
		exit(1);

	exit(0);
}


int
command_list(int argc, const char* const* argv)
{
	ListMode listMode = LIST_ALL;
	bool listAttributes = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+ahip", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				listAttributes = true;
				break;

			case 'i':
				listMode = LIST_META_INFO_ONLY;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'p':
				listMode = LIST_PATHS_ONLY;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the package file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// If the file doesn't look like a package file, try to load it as a
	// package info file.
	if (!BString(packageFileName).EndsWith(".hpkg")) {
		struct ErrorListener : BPackageInfo::ParseErrorListener {
			virtual void OnError(const BString& msg, int line, int col)
			{
				fprintf(stderr, "%s:%d:%d: %s\n", fPath, line, col,
					msg.String());
			}

			const char*	fPath;
		} errorListener;
		errorListener.fPath = packageFileName;

		BPackageInfo info;
		if (info.ReadFromConfigFile(BEntry(packageFileName), &errorListener)
				!= B_OK) {
			return 1;
		}

		printf("package-attributes:\n");
		PackageInfoPrinter().PrintPackageInfo(info);
		return 0;
	}

	BHPKG::BStandardErrorOutput errorOutput;

	// current package file format version
	do_list<VersionPolicyV2>(packageFileName, listAttributes, listMode, true);
	do_list<VersionPolicyV1>(packageFileName, listAttributes, listMode, false);

	return 0;
}
