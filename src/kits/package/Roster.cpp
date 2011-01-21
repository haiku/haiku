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
#include <Path.h>
#include <FindDirectory.h>


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
	if (path == NULL)
		return B_BAD_VALUE;

	status_t result = find_directory(B_COMMON_SETTINGS_DIRECTORY, path);
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
Roster::GetUserRepositoryConfigPath(BPath* path, bool create) const
{
	if (path == NULL)
		return B_BAD_VALUE;

	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, path);
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
Roster::VisitCommonRepositoryConfigs(RepositoryConfigVisitor& visitor)
{
	BPath commonRepositoryConfigPath;
	status_t result
		= GetCommonRepositoryConfigPath(&commonRepositoryConfigPath);
	if (result == B_OK)
		result = _VisitRepositoryConfigs(commonRepositoryConfigPath, visitor);

	return result;
}


status_t
Roster::VisitUserRepositoryConfigs(RepositoryConfigVisitor& visitor)
{
	BPath userRepositoryConfigPath;
	status_t result = GetUserRepositoryConfigPath(&userRepositoryConfigPath);
	if (result == B_OK)
		result = _VisitRepositoryConfigs(userRepositoryConfigPath, visitor);

	return result;
}


status_t
Roster::_VisitRepositoryConfigs(const BPath& path,
	RepositoryConfigVisitor& visitor)
{
	BDirectory directory(path.Path());
	status_t result = directory.InitCheck();
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
