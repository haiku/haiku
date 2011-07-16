/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/PackageRoster.h>

#include <errno.h>
#include <sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <StringList.h>

#include <package/PackageInfo.h>
#include <package/PackageInfoContentHandler.h>
#include <package/PackageInfoSet.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>

#include <package/hpkg/PackageReader.h>


namespace BPackageKit {


using namespace BHPKG;


BPackageRoster::BPackageRoster()
{
}


BPackageRoster::~BPackageRoster()
{
}


status_t
BPackageRoster::GetCommonRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_COMMON_SETTINGS_DIRECTORY);
}


status_t
BPackageRoster::GetUserRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_USER_SETTINGS_DIRECTORY);
}


status_t
BPackageRoster::GetCommonRepositoryCachePath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_COMMON_CACHE_DIRECTORY);
}


status_t
BPackageRoster::GetUserRepositoryCachePath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_USER_CACHE_DIRECTORY);
}


status_t
BPackageRoster::VisitCommonRepositoryConfigs(BRepositoryConfigVisitor& visitor)
{
	BPath commonRepositoryConfigPath;
	status_t result
		= GetCommonRepositoryConfigPath(&commonRepositoryConfigPath);
	if (result != B_OK)
		return result;

	return _VisitRepositoryConfigs(commonRepositoryConfigPath, visitor);
}


status_t
BPackageRoster::VisitUserRepositoryConfigs(BRepositoryConfigVisitor& visitor)
{
	BPath userRepositoryConfigPath;
	status_t result = GetUserRepositoryConfigPath(&userRepositoryConfigPath);
	if (result != B_OK)
		return result;

	return _VisitRepositoryConfigs(userRepositoryConfigPath, visitor);
}


status_t
BPackageRoster::GetRepositoryNames(BStringList& names)
{
	struct RepositoryNameCollector : public BRepositoryConfigVisitor {
		RepositoryNameCollector(BStringList& _names)
			: names(_names)
		{
		}
		status_t operator()(const BEntry& entry)
		{
			char name[B_FILE_NAME_LENGTH];
			status_t result = entry.GetName(name);
			if (result != B_OK)
				return result;
			int32 count = names.CountStrings();
			for (int i = 0; i < count; ++i) {
				if (names.StringAt(i).Compare(name) == 0)
					return B_OK;
			}
			names.Add(name);
			return B_OK;
		}
		BStringList& names;
	};
	RepositoryNameCollector repositoryNameCollector(names);
	status_t result = VisitUserRepositoryConfigs(repositoryNameCollector);
	if (result != B_OK)
		return result;

	return VisitCommonRepositoryConfigs(repositoryNameCollector);
}


status_t
BPackageRoster::GetRepositoryCache(const BString& name,
	BRepositoryCache* repositoryCache)
{
	if (repositoryCache == NULL)
		return B_BAD_VALUE;

	// user path has higher precedence than common path
	BPath path;
	status_t result = GetUserRepositoryCachePath(&path);
	if (result != B_OK)
		return result;
	path.Append(name.String());

	BEntry repoCacheEntry(path.Path());
	if (repoCacheEntry.Exists())
		return repositoryCache->SetTo(repoCacheEntry);

	if ((result = GetCommonRepositoryCachePath(&path, true)) != B_OK)
		return result;
	path.Append(name.String());

	repoCacheEntry.SetTo(path.Path());
	return repositoryCache->SetTo(repoCacheEntry);
}


status_t
BPackageRoster::GetRepositoryConfig(const BString& name,
	BRepositoryConfig* repositoryConfig)
{
	if (repositoryConfig == NULL)
		return B_BAD_VALUE;

	// user path has higher precedence than common path
	BPath path;
	status_t result = GetUserRepositoryConfigPath(&path);
	if (result != B_OK)
		return result;
	path.Append(name.String());

	BEntry repoConfigEntry(path.Path());
	if (repoConfigEntry.Exists())
		return repositoryConfig->SetTo(repoConfigEntry);

	if ((result = GetCommonRepositoryConfigPath(&path, true)) != B_OK)
		return result;
	path.Append(name.String());

	repoConfigEntry.SetTo(path.Path());
	return repositoryConfig->SetTo(repoConfigEntry);
}


status_t
BPackageRoster::GetActivePackages(BPackageInstallationLocation location,
	BPackageInfoSet& packageInfos)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	// check the given location
	directory_which packagesDirectory;
	switch (location) {
		case B_PACKAGE_INSTALLATION_LOCATION_SYSTEM:
			packagesDirectory = B_SYSTEM_PACKAGES_DIRECTORY;
			break;
		case B_PACKAGE_INSTALLATION_LOCATION_COMMON:
			packagesDirectory = B_COMMON_PACKAGES_DIRECTORY;
			break;
		case B_PACKAGE_INSTALLATION_LOCATION_HOME:
			packagesDirectory = B_USER_PACKAGES_DIRECTORY;
			break;
		default:
			return B_BAD_VALUE;
	}

	// find the package links directory
	BPath packageLinksPath;
	status_t error = find_directory(B_PACKAGE_LINKS_DIRECTORY,
		&packageLinksPath);
	if (error != B_OK)
		return error;

	// find and open the packages directory
	BPath packagesDirPath;
	error = find_directory(packagesDirectory, &packagesDirPath);
	if (error != B_OK)
		return error;

	BDirectory directory;
	error = directory.SetTo(packagesDirPath.Path());
	if (error != B_OK)
		return error;

	// TODO: Implement that correctly be reading the activation files/directory!

	// iterate through the packages
	char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
	dirent* entry = (dirent*)&buffer;
	while (directory.GetNextDirents(entry, sizeof(buffer), 1) == 1) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		// get the full package file path
		BPath packagePath;
		error = packagePath.SetTo(packagesDirPath.Path(), entry->d_name);
		if (error != B_OK)
			continue;

		// read the package info from the file
		BPackageReader packageReader(NULL);
		error = packageReader.Init(packagePath.Path());
		if (error != B_OK)
			continue;

		BPackageInfo info;
		BPackageInfoContentHandler handler(info);
		error = packageReader.ParseContent(&handler);
		if (error != B_OK || info.InitCheck() != B_OK)
			continue;

		// check whether the package is really active by verifying that a
		// package link exists for it
		BString packageLinkName(info.Name());
		packageLinkName << '-' << info.Version().ToString();
		BPath packageLinkPath;
		struct stat st;
		if (packageLinkPath.SetTo(packageLinksPath.Path(), packageLinkName)
				!= B_OK
			|| lstat(packageLinkPath.Path(), &st) != 0) {
			continue;
		}

		// add the info
		error = packageInfos.AddInfo(info);
		if (error != B_OK)
			return error;
	}

	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BPackageRoster::_GetRepositoryPath(BPath* path, bool create,
	directory_which whichDir) const
{
	if (path == NULL)
		return B_BAD_VALUE;

	status_t result = find_directory(whichDir, path);
	if (result != B_OK)
		return result;
	if ((result = path->Append("package-repositories")) != B_OK)
		return result;

	if (create) {
		BEntry entry(path->Path(), true);
		if (!entry.Exists()) {
			if (mkdir(path->Path(), 0755) != 0)
				return errno;
		}
	}

	return B_OK;
}


status_t
BPackageRoster::_VisitRepositoryConfigs(const BPath& path,
	BRepositoryConfigVisitor& visitor)
{
	BDirectory directory(path.Path());
	status_t result = directory.InitCheck();
	if (result == B_ENTRY_NOT_FOUND)
		return B_OK;
	if (result != B_OK)
		return result;

	BEntry entry;
	while (directory.GetNextEntry(&entry, true) == B_OK) {
		if ((result = visitor(entry)) != B_OK)
			return result;
	}

	return B_OK;
}


}	// namespace BPackageKit
