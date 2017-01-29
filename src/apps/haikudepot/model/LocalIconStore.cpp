/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LocalIconStore.h"

#include <stdio.h>

#include <Directory.h>
#include <FindDirectory.h>

#include "ServerIconExportUpdateProcess.h"
#include "StorageUtils.h"


LocalIconStore::LocalIconStore()
{
	if (_EnsureIconStoragePath(fIconStoragePath) != B_OK)
		fprintf(stdout, "unable to setup icon storage\n");
}


LocalIconStore::~LocalIconStore()
{
}


bool
LocalIconStore::_HasIconStoragePath() const
{
	return fIconStoragePath.InitCheck() == B_OK;
}


/* This method will try to find an icon for the package name supplied.  If an
 * icon was able to be found then the method will return B_OK and will update
 * the supplied path object with the path to the icon file.
 */

status_t
LocalIconStore::TryFindIconPath(const BString& pkgName, BPath& path) const
{
	if (_HasIconStoragePath()) {
		BPath bestIconPath;
		BPath iconPkgPath(fIconStoragePath);
		bool exists;
		bool isDir;

		if ( (iconPkgPath.Append("hicn") == B_OK)
			&& (iconPkgPath.Append(pkgName) == B_OK)
			&& (StorageUtils::ExistsDirectory(iconPkgPath, &exists, &isDir)
				== B_OK)
			&& exists
			&& isDir
			&& (_IdentifyBestIconFileAtDirectory(iconPkgPath, bestIconPath)
				== B_OK)
		) {
			path = bestIconPath;
			return B_OK;
		}
	}

	path.Unset();
	return B_FILE_NOT_FOUND;
}


void
LocalIconStore::UpdateFromServerIfNecessary() const
{
	if (_HasIconStoragePath()) {
		BPath iconStoragePath(fIconStoragePath);
		ServerIconExportUpdateProcess service(iconStoragePath);
		service.Run();
	}
}


status_t
LocalIconStore::_EnsureIconStoragePath(BPath& path) const
{
	BPath iconStoragePath;

	if (find_directory(B_USER_CACHE_DIRECTORY, &iconStoragePath) == B_OK
		&& iconStoragePath.Append("HaikuDepot") == B_OK
		&& iconStoragePath.Append("__allicons") == B_OK
		&& create_directory(iconStoragePath.Path(), 0777) == B_OK) {
		path.SetTo(iconStoragePath.Path());
		return B_OK;
	}

	path.Unset();
	fprintf(stdout, "unable to find the user cache directory for icons");
	return B_ERROR;
}


status_t
LocalIconStore::_IdentifyBestIconFileAtDirectory(const BPath& directory,
	BPath& bestIconPath) const
{
	StringList iconLeafnames;

	iconLeafnames.Add("icon.hvif");
	iconLeafnames.Add("64.png");
	iconLeafnames.Add("32.png");
	iconLeafnames.Add("16.png");

	bestIconPath.Unset();

	for (int32 i = 0; i < iconLeafnames.CountItems(); i++) {
		BString iconLeafname = iconLeafnames.ItemAt(i);
		BPath workingPath(directory);
		bool exists;
		bool isDir;

		if ( (workingPath.Append(iconLeafname) == B_OK
			&& StorageUtils::ExistsDirectory(
				workingPath, &exists, &isDir) == B_OK)
			&& exists
			&& !isDir) {
			bestIconPath.SetTo(workingPath.Path());
			return B_OK;
		}
	}

	return B_FILE_NOT_FOUND;
}
