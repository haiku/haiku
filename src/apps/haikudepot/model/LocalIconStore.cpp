/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "LocalIconStore.h"

#include <stdio.h>

#include <Directory.h>
#include <FindDirectory.h>

#include "Logger.h"
#include "ServerIconExportUpdateProcess.h"
#include "StorageUtils.h"


LocalIconStore::LocalIconStore(const BPath& path)
{
	fIconStoragePath = path;
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
	BPath bestIconPath;
	BPath iconPkgPath(fIconStoragePath);
	bool exists;
	bool isDir;

	if ( (iconPkgPath.Append("hicn") == B_OK)
		&& (iconPkgPath.Append(pkgName) == B_OK)
		&& (StorageUtils::ExistsObject(iconPkgPath, &exists, &isDir, NULL)
			== B_OK)
		&& exists
		&& isDir
		&& (_IdentifyBestIconFileAtDirectory(iconPkgPath, bestIconPath)
			== B_OK)
	) {
		path = bestIconPath;
		return B_OK;
	}

	path.Unset();
	return B_FILE_NOT_FOUND;
}


status_t
LocalIconStore::_IdentifyBestIconFileAtDirectory(const BPath& directory,
	BPath& bestIconPath) const
{
	static const char* kIconLeafnames[] = {
		"icon.hvif",
		"64.png",
		"32.png",
		"16.png",
		NULL
	};

	bestIconPath.Unset();

	for (int32 i = 0; kIconLeafnames[i] != NULL; i++) {
		BPath workingPath(directory);
		bool exists;
		bool isDir;

		if ( (workingPath.Append(kIconLeafnames[i]) == B_OK
			&& StorageUtils::ExistsObject(
				workingPath, &exists, &isDir, NULL) == B_OK)
			&& exists
			&& !isDir) {
			bestIconPath.SetTo(workingPath.Path());
			return B_OK;
		}
	}

	return B_FILE_NOT_FOUND;
}
