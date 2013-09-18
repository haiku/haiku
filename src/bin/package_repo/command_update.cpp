/*
 * Copyright 2011-2013, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>

#include <Entry.h>
#include <ObjectList.h>
#include <Path.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/RepositoryContentHandler.h>
#include <package/hpkg/RepositoryReader.h>
#include <package/hpkg/RepositoryWriter.h>
#include <package/hpkg/StandardErrorOutput.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>

#include "package_repo.h"


using BPackageKit::BHPKG::BRepositoryWriterListener;
using BPackageKit::BHPKG::BRepositoryWriter;
using namespace BPackageKit::BHPKG;
using namespace BPackageKit;


bool operator< (const BPackageInfo & left, const BPackageInfo & right)
{
	if (left.Name() != right.Name())
		return left.Name() < right.Name();
	return left.Version().Compare(right.Version()) < 0;
}


namespace
{


typedef std::map<BPackageInfo, bool> PackageInfos;


status_t
parsePackageListFile(const char* packageListFileName,
	BObjectList<BString>* packageFileNames)
{
	FILE* packageListFile = fopen(packageListFileName, "r");
	if (packageListFile == NULL) {
		printf("Error: Unable to open %s\n", packageListFileName);
		return B_ENTRY_NOT_FOUND;
	}
	char buffer[128];
	while (fgets(buffer, sizeof(buffer), packageListFile) != NULL) {
		BString* packageFileName = new(std::nothrow) BString(buffer);
		if (packageFileName == NULL) {
			printf("Error: Out of memory when reading from %s\n",
				packageListFileName);
			fclose(packageListFile);
			return B_NO_MEMORY;
		}
		packageFileName->Trim();
		packageFileNames->AddItem(packageFileName);
	}
	fclose(packageListFile);
	return B_OK;
}


struct PackageInfosCollector : BRepositoryContentHandler {
	PackageInfosCollector(PackageInfos& packageInfos)
		:
		fPackageInfos(packageInfos)
	{
	}

	virtual status_t HandlePackage(const char* packageName)
	{
		fPackageInfo.Clear();
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				fPackageInfo.SetName(value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				fPackageInfo.SetSummary(value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				fPackageInfo.SetDescription(value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				fPackageInfo.SetVendor(value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				fPackageInfo.SetPackager(value.string);
				break;

			case B_PACKAGE_INFO_BASE_PACKAGE:
				fPackageInfo.SetBasePackage(value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				fPackageInfo.SetFlags(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				fPackageInfo.SetArchitecture(BPackageArchitecture(
					value.unsignedInt));
				break;

			case B_PACKAGE_INFO_VERSION:
				fPackageInfo.SetVersion(value.version);
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				fPackageInfo.AddCopyright(value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				fPackageInfo.AddLicense(value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				fPackageInfo.AddURL(value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				fPackageInfo.AddSourceURL(value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				fPackageInfo.AddProvides(value.resolvable);
				break;

			case B_PACKAGE_INFO_REQUIRES:
				fPackageInfo.AddRequires(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				fPackageInfo.AddSupplements(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				fPackageInfo.AddConflicts(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_FRESHENS:
				fPackageInfo.AddFreshens(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_REPLACES:
				fPackageInfo.AddReplaces(value.string);
				break;

			case B_PACKAGE_INFO_GLOBAL_WRITABLE_FILES:
				fPackageInfo.AddGlobalWritableFileInfo(
					value.globalWritableFileInfo);
				break;

			case B_PACKAGE_INFO_USER_SETTINGS_FILES:
				fPackageInfo.AddUserSettingsFileInfo(
					value.userSettingsFileInfo);
				break;

			case B_PACKAGE_INFO_USERS:
				fPackageInfo.AddUser(value.user);
				break;

			case B_PACKAGE_INFO_GROUPS:
				fPackageInfo.AddGroup(value.string);
				break;

			case B_PACKAGE_INFO_POST_INSTALL_SCRIPTS:
				fPackageInfo.AddPostInstallScript(value.string);
				break;

			case B_PACKAGE_INFO_INSTALL_PATH:
				fPackageInfo.SetInstallPath(value.string);
				break;

			case B_PACKAGE_INFO_CHECKSUM:
				fPackageInfo.SetChecksum(value.string);
				break;

			default:
				printf("Error: Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", value.attributeID);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t HandlePackageDone(const char* packageName)
	{
		if (fPackageInfo.InitCheck() != B_OK) {
			const BString& name = fPackageInfo.Name();
			printf("Error: package-info in repository for '%s' is incomplete\n",
				name.IsEmpty() ? "<unknown-package>" : name.String());
			return B_BAD_DATA;
		}
		fPackageInfos[fPackageInfo] = false;
		return B_OK;
	}

	virtual status_t HandleRepositoryInfo(const BRepositoryInfo& repositoryInfo)
	{
		fRepositoryInfo = repositoryInfo;
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

	const BRepositoryInfo& RepositoryInfo() const
	{
		return fRepositoryInfo;
	}

private:
	BRepositoryInfo fRepositoryInfo;
	BPackageInfo fPackageInfo;
	PackageInfos& fPackageInfos;
};


class RepositoryWriterListener	: public BRepositoryWriterListener {
public:
	RepositoryWriterListener(bool verbose, bool quiet)
		: fVerbose(verbose), fQuiet(quiet)
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
		vfprintf(stderr, format, args);
	}

	virtual void OnPackageAdded(const BPackageInfo& packageInfo)
	{
	}

	virtual void OnRepositoryInfoSectionDone(uint32 uncompressedSize)
	{
		if (fQuiet || !fVerbose)
			return;

		printf("----- Repository Info Section --------------------\n");
		printf("repository info size:    %10" B_PRIu32 " (uncompressed)\n",
			uncompressedSize);
	}

	virtual void OnPackageAttributesSectionDone(uint32 stringCount,
		uint32 uncompressedSize)
	{
		if (fQuiet || !fVerbose)
			return;

		printf("----- Package Attribute Section -------------------\n");
		printf("string count:            %10" B_PRIu32 "\n", stringCount);
		printf("package attributes size: %10" B_PRIu32 " (uncompressed)\n",
			uncompressedSize);
	}

	virtual void OnRepositoryDone(uint32 headerSize, uint32 repositoryInfoSize,
		uint32 licenseCount, uint32 packageCount, uint32 packageAttributesSize,
		uint64 totalSize)
	{
		if (fQuiet || !fVerbose)
			return;

		printf("----- Package Repository Info -----\n");
		if (fVerbose)
			printf("embedded license count   %10" B_PRIu32 "\n", licenseCount);
		printf("package count            %10" B_PRIu32 "\n", packageCount);
		printf("-----------------------------------\n");
		printf("header size:             %10" B_PRIu32 "\n", headerSize);
		printf("repository header size:  %10" B_PRIu32 "\n",
			repositoryInfoSize);
		printf("package attributes size: %10" B_PRIu32 "\n",
			packageAttributesSize);
		printf("total size:              %10" B_PRIu64 "\n", totalSize);
		printf("-----------------------------------\n");
	}

private:
	bool fVerbose;
	bool fQuiet;
};


}	// anonymous namespace


int
command_update(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;
	bool quiet = false;
	bool verbose = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:hqv", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'q':
				quiet = true;
				break;

			case 'v':
				verbose = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining three arguments are the source and target repository file
	// plus the package list file.
	if (optind + 3 != argc)
		print_usage_and_exit(true);

	const char* sourceRepositoryFileName = argv[optind++];
	const char* targetRepositoryFileName = argv[optind++];
	const char* packageListFileName = argv[optind++];

	// open source repository
	BStandardErrorOutput errorOutput;
	BRepositoryReader repositoryReader(&errorOutput);
	status_t error = repositoryReader.Init(sourceRepositoryFileName);
	if (error != B_OK)
		return 1;

	// collect package infos and repository info from source repository
	PackageInfos packageInfos;
	PackageInfosCollector packageInfosCollector(packageInfos);
	error = repositoryReader.ParseContent(&packageInfosCollector);
	if (error != B_OK)
		return 1;

	RepositoryWriterListener listener(verbose, quiet);
	BRepositoryInfo repositoryInfo = packageInfosCollector.RepositoryInfo();
	status_t result = repositoryInfo.InitCheck();
	if (result != B_OK) {
		listener.PrintError(
			"Error: didn't get a proper repository-info from source repository"
			": %s - error: %s\n", sourceRepositoryFileName, strerror(result));
		return 1;
	}

	// create new repository
	BRepositoryWriter repositoryWriter(&listener, &repositoryInfo);
	BString tempRepositoryFileName(targetRepositoryFileName);
	tempRepositoryFileName += ".new";
	if ((result = repositoryWriter.Init(tempRepositoryFileName.String()))
			!= B_OK) {
		listener.PrintError("Error: can't initialize repository-writer : %s\n",
			strerror(result));
		return 1;
	}

	BObjectList<BString> packageNames(100, true);
	if ((result = parsePackageListFile(packageListFileName, &packageNames))
			!= B_OK) {
		listener.PrintError(
			"Error: Failed to read package-list-file \"%s\": %s\n",
			packageListFileName, strerror(result));
		return 1;
	}

	// change directory, if requested
	if (changeToDirectory != NULL) {
		if (chdir(changeToDirectory) != 0) {
			listener.PrintError(
				"Error: Failed to change the current working directory to "
				"\"%s\": %s\n", changeToDirectory, strerror(errno));
			return 1;
		}
	}

	// add all given package files
	for (int i = 0; i < packageNames.CountItems(); ++i) {
		BPackageInfo packageInfo;
		if ((result = packageInfo.ReadFromPackageFile(
					packageNames.ItemAt(i)->String())) != B_OK) {
			listener.PrintError(
				"Error: Failed to read package-info from \"%s\": %s\n",
				packageNames.ItemAt(i)->String(), strerror(result));
			return 1;
		}
		PackageInfos::iterator infoIter = packageInfos.find(packageInfo);
		if (infoIter != packageInfos.end()) {
			infoIter->second = true;
			if ((result = repositoryWriter.AddPackageInfo(packageInfo)) != B_OK)
				return 1;
			if (verbose) {
				printf("keeping '%s-%s'\n", infoIter->first.Name().String(),
					infoIter->first.Version().ToString().String());
			}
		} else {
			BEntry entry(packageNames.ItemAt(i)->String());
			if ((result = repositoryWriter.AddPackage(entry)) != B_OK)
				return 1;
			if (!quiet) {
				printf("added '%s' ...\n",
					packageNames.ItemAt(i)->String());
			}
		}
	}

	// tell about packages dropped from repository
	PackageInfos::const_iterator infoIter;
	for (infoIter = packageInfos.begin(); infoIter != packageInfos.end();
			++infoIter) {
		if (!infoIter->second) {
			printf("dropped '%s-%s'\n", infoIter->first.Name().String(),
				infoIter->first.Version().ToString().String());
		}
	}


	// write the repository
	result = repositoryWriter.Finish();
	if (result != B_OK)
		return 1;

	if (verbose) {
		printf("\nsuccessfully created repository '%s'\n",
			targetRepositoryFileName);
	}

	return 0;
}
