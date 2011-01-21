/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/TempEntryManager.h>


namespace Haiku {

namespace Package {


const BString TempEntryManager::kDefaultName = "tmp-pkgkit-file-";


TempEntryManager::TempEntryManager()
	:
	fNextNumber(1)
{
}


TempEntryManager::~TempEntryManager()
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
TempEntryManager::SetBaseDirectory(const BDirectory& baseDirectory)
{
	fBaseDirectory = baseDirectory;
}


BEntry
TempEntryManager::Create(const BString& baseName)
{
	BString name = BString(baseName) << atomic_add(&fNextNumber, 1);

	return BEntry(&fBaseDirectory, name.String());
}


}	// namespace Package

}	// namespace Haiku
