/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Roster.h>

#include <errno.h>
#include <sys/stat.h>

#include <Directory.h>
#include <Entry.h>
#include <ObjectList.h>
#include <Path.h>
#include <String.h>

#include <package/RepositoryCache.h>
#include <package/RepositoryConfig.h>


namespace Haiku {

namespace Package {


Roster::Roster()
{
}


Roster::~Roster()
{
}


status_t
Roster::GetCommonRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_COMMON_SETTINGS_DIRECTORY);
}


status_t
Roster::GetUserRepositoryConfigPath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_USER_SETTINGS_DIRECTORY);
}


status_t
Roster::GetCommonRepositoryCachePath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_COMMON_CACHE_DIRECTORY);
}


status_t
Roster::GetUserRepositoryCachePath(BPath* path, bool create) const
{
	return _GetRepositoryPath(path, create, B_USER_CACHE_DIRECTORY);
}


status_t
Roster::VisitCommonRepositoryConfigs(RepositoryConfigVisitor& visitor)
{
	BPath commonRepositoryConfigPath;
	status_t result
		= GetCommonRepositoryConfigPath(&commonRepositoryConfigPath);
	if (result != B_OK)
		return result;

	return _VisitRepositoryConfigs(commonRepositoryConfigPath, visitor);
}


status_t
Roster::VisitUserRepositoryConfigs(RepositoryConfigVisitor& visitor)
{
	BPath userRepositoryConfigPath;
	status_t result = GetUserRepositoryConfigPath(&userRepositoryConfigPath);
	if (result != B_OK)
		return result;

	return _VisitRepositoryConfigs(userRepositoryConfigPath, visitor);
}


status_t
Roster::GetRepositoryNames(BObjectList<BString>& names)
{
	struct RepositoryNameCollector : public RepositoryConfigVisitor {
		RepositoryNameCollector(BObjectList<BString>& _names)
			: names(_names)
		{
		}
		status_t operator()(const BEntry& entry)
		{
			char name[B_FILE_NAME_LENGTH];
			status_t result = entry.GetName(name);
			if (result != B_OK)
				return result;
			int32 count = names.CountItems();
			for (int i = 0; i < count; ++i) {
				if (names.ItemAt(i)->Compare(name) == 0)
					return B_OK;
			}
			names.AddItem(new (std::nothrow) BString(name));
			return B_OK;
		}
		BObjectList<BString>& names;
	};
	RepositoryNameCollector repositoryNameCollector(names);
	status_t result = VisitUserRepositoryConfigs(repositoryNameCollector);
	if (result != B_OK)
		return result;

	return VisitCommonRepositoryConfigs(repositoryNameCollector);
}


status_t
Roster::GetRepositoryCache(const BString& name,
	RepositoryCache* repositoryCache)
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

	if ((result = GetCommonRepositoryCachePath(&path)) != B_OK)
		return result;
	path.Append(name.String());

	repoCacheEntry.SetTo(path.Path());
	return repositoryCache->SetTo(repoCacheEntry);
}


status_t
Roster::GetRepositoryConfig(const BString& name,
	RepositoryConfig* repositoryConfig)
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

	if ((result = GetCommonRepositoryConfigPath(&path)) != B_OK)
		return result;
	path.Append(name.String());

	repoConfigEntry.SetTo(path.Path());
	return repositoryConfig->SetTo(repoConfigEntry);
}


status_t
Roster::_GetRepositoryPath(BPath* path, bool create,
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
Roster::_VisitRepositoryConfigs(const BPath& path,
	RepositoryConfigVisitor& visitor)
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


}	// namespace Package

}	// namespace Haiku
