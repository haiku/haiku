/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/TempfileManager.h>


namespace BPackageKit {

namespace BPrivate {


const BString TempfileManager::kDefaultName = "tmp-pkgkit-file-";


TempfileManager::TempfileManager()
	:
	fNextNumber(1)
{
}


TempfileManager::~TempfileManager()
{
	if (fBaseDirectory.InitCheck() != B_OK)
		return;

	fBaseDirectory.Rewind();
	BEntry entry;
	while (fBaseDirectory.GetNextEntry(&entry) == B_OK)
		entry.Remove();

	fBaseDirectory.GetEntry(&entry);
	entry.Remove();
}


void
TempfileManager::SetBaseDirectory(const BDirectory& baseDirectory)
{
	fBaseDirectory = baseDirectory;
}


BEntry
TempfileManager::Create(const BString& baseName)
{
	BString name = BString(baseName) << atomic_add(&fNextNumber, 1);

	return BEntry(&fBaseDirectory, name.String());
}


}	// namespace BPrivate

}	// namespace BPackageKit
