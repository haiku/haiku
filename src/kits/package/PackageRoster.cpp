/*
 * Copyright 2011-2014, Haiku, Inc. All Rights Reserved.
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
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <StringList.h>

#include <package/InstallationLocationInfo.h>
#include <package/PackageInfo.h>
#include <package/PackageInfoContentHandler.h>
#include <package/PackageInfoSet.h>
#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>

#include <package/hpkg/PackageReader.h>


#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
#	include <package/DaemonClient.h>
#	include <RegistrarDefs.h>
#	include <RosterPrivate.h>
#endif


namespace BPackageKit {


using namespace BHPKG;


BPackageRoster::BPackageRoster()
{
}


BPackageRoster::~BPackageRoster()
{
}


bool
BPackageRoster::IsRebootNeeded()
{
	BInstallationLocationInfo info;

	// We get information on the system package installation location.
	// If we fail, we just have to assume a reboot is not needed.
	if (GetInstallationLocationInfo(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM,
		info) != B_OK)
		return false;

	// CurrentlyActivePackageInfos() will return 0 if no packages need to be
	// activated with a reboot. Otherwise, the method will return the total
	// number of packages in the system package directory.
	if (info.CurrentlyActivePackageInfos().CountInfos() != 0)
		return true;

	return false;
}


status_t
BPackageRoster::GetCommonRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_SYSTEM_SETTINGS_DIRECTORY);
}


status_t
BPackageRoster::GetUserRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_USER_SETTINGS_DIRECTORY);
}


status_t
BPackageRoster::GetCommonRepositoryCachePath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_SYSTEM_CACHE_DIRECTORY);
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

	result = repoCacheEntry.SetTo(path.Path());
	if (result != B_OK)
		return result;
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

	result = repoConfigEntry.SetTo(path.Path());
	if (result != B_OK)
		return result;
	return repositoryConfig->SetTo(repoConfigEntry);
}


status_t
BPackageRoster::GetInstallationLocationInfo(
	BPackageInstallationLocation location, BInstallationLocationInfo& _info)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	return BPackageKit::BPrivate::BDaemonClient().GetInstallationLocationInfo(
		location, _info);
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BPackageRoster::GetActivePackages(BPackageInstallationLocation location,
	BPackageInfoSet& packageInfos)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	BInstallationLocationInfo info;
	status_t error = GetInstallationLocationInfo(location, info);
	if (error != B_OK)
		return error;

	packageInfos = info.LatestActivePackageInfos();
	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BPackageRoster::IsPackageActive(BPackageInstallationLocation location,
	const BPackageInfo info, bool* active)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	BPackageInfoSet packageInfos;
	status_t error = GetActivePackages(location, packageInfos);
	if (error != B_OK)
		return error;

	BRepositoryCache::Iterator it = packageInfos.GetIterator();
	while (const BPackageInfo* packageInfo = it.Next()) {
		if (info.Name() == packageInfo->Name() &&
			info.Version().Compare(packageInfo->Version()) == 0) {
			*active = true;
			break;
		}
	}

	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BPackageRoster::StartWatching(const BMessenger& target, uint32 eventMask)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	// compose the registrar request
	BMessage request(::BPrivate::B_REG_PACKAGE_START_WATCHING);
	status_t error;
	if ((error = request.AddMessenger("target", target)) != B_OK
		|| (error = request.AddUInt32("events", eventMask)) != B_OK) {
		return error;
	}

	// send it
	BMessage reply;
	error = BRoster::Private().SendTo(&request, &reply, false);
	if (error != B_OK)
		return error;

	// get result
	if (reply.what != ::BPrivate::B_REG_SUCCESS) {
		int32 result;
		if (reply.FindInt32("error", &result) != B_OK)
			result = B_ERROR;
		return (status_t)error;
	}

	return B_OK;
#else
	return B_NOT_SUPPORTED;
#endif
}


status_t
BPackageRoster::StopWatching(const BMessenger& target)
{
// This method makes sense only on an installed Haiku, but not for the build
// tools.
#if defined(__HAIKU__) && !defined(HAIKU_HOST_PLATFORM_HAIKU)
	// compose the registrar request
	BMessage request(::BPrivate::B_REG_PACKAGE_STOP_WATCHING);
	status_t error = request.AddMessenger("target", target);
	if (error  != B_OK)
		return error;

	// send it
	BMessage reply;
	error = BRoster::Private().SendTo(&request, &reply, false);
	if (error != B_OK)
		return error;

	// get result
	if (reply.what != ::BPrivate::B_REG_SUCCESS) {
		int32 result;
		if (reply.FindInt32("error", &result) != B_OK)
			result = B_ERROR;
		return (status_t)error;
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
