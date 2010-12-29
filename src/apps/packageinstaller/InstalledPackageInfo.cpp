/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */


#include "InstalledPackageInfo.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>


const char * kPackagesDir = "packages";


static status_t
info_prepare(const char *filename, BFile *file, BMessage *info)
{
	if (filename == NULL || file == NULL || info == NULL)
		return B_BAD_VALUE;

	BPath path;
	status_t ret = find_directory(B_USER_CONFIG_DIRECTORY, &path);
	if (ret == B_OK)
		ret = path.Append(kPackagesDir);
	if (ret == B_OK)
		ret = path.Append(filename);
	if (ret == B_OK)
		ret = file->SetTo(path.Path(), B_READ_ONLY);
	if (ret == B_OK)
		ret = info->Unflatten(file);
	if (ret == B_OK && info->what != P_PACKAGE_INFO)
		ret = B_ERROR;

	return ret;
}


status_t
info_get_package_name(const char *filename, BString &name)
{
	BFile file;
	BMessage info;
	status_t ret = info_prepare(filename, &file, &info);
	if (ret == B_OK)
		ret = info.FindString("package_name", &name);
	return ret;
}


status_t
info_get_package_version(const char *filename, BString &version)
{
	BFile file;
	BMessage info;
	status_t ret = info_prepare(filename, &file, &info);
	if (ret == B_OK)
		ret = info.FindString("package_version", &version);
	return ret;
}


InstalledPackageInfo::InstalledPackageInfo()
	:	
	fStatus(B_NO_INIT),
	fIsUpToDate(false),
	fCreate(false),
	fSpaceNeeded(0),
	fInstalledItems(10)
{
}


InstalledPackageInfo::InstalledPackageInfo(const char *packageName, 
		const char *version, bool create)
	:
	fStatus(B_NO_INIT),
	fIsUpToDate(false),
	fSpaceNeeded(0),
	fInstalledItems(10)
{
	SetTo(packageName, version, create);
}


InstalledPackageInfo::~InstalledPackageInfo()
{
	_ClearItemList();
}


status_t
InstalledPackageInfo::InitCheck()
{
	return fStatus;
}


status_t
InstalledPackageInfo::SetTo(const char *packageName, const char *version, 
		bool create)
{
	_ClearItemList();
	fCreate = create;
	fStatus = B_NO_INIT;
	fVersion = version;

	if (!packageName)
		return fStatus;

	BPath configPath;
	if (find_directory(B_USER_CONFIG_DIRECTORY, &configPath) != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}
	
	if (fPathToInfo.SetTo(configPath.Path(), kPackagesDir) != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}
	
	// Check whether the directory exists
	BDirectory packageDir(fPathToInfo.Path());
	fStatus = packageDir.InitCheck();
	if (fStatus == B_ENTRY_NOT_FOUND) {
		// If not, create it
		packageDir.SetTo(configPath.Path());
		if (packageDir.CreateDirectory(kPackagesDir, &packageDir) != B_OK) {
			fStatus = B_ERROR;
			return fStatus;
		}
	}

	BString filename = packageName;
	filename << version << ".pdb";
	if (fPathToInfo.Append(filename.String()) != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}

	BFile package(fPathToInfo.Path(), B_READ_ONLY);
	fStatus = package.InitCheck();
	if (fStatus == B_OK) {
		// The given package exists, so we can unflatten the data to a message
		// and then pass it further
		BMessage info;
		if (info.Unflatten(&package) != B_OK || info.what != P_PACKAGE_INFO) {
			fStatus = B_ERROR;
			return fStatus;
		}

		int32 count;
		fStatus = info.FindString("package_name", &fName);
		fStatus |= info.FindString("package_desc", &fDescription);
		fStatus |= info.FindString("package_version", &fVersion);
		int64 spaceNeeded = 0;
		fStatus |= info.FindInt64("package_size", &spaceNeeded);
		fSpaceNeeded = static_cast<uint64>(spaceNeeded);
		fStatus |= info.FindInt32("file_count", &count);
		if (fStatus != B_OK) {
			fStatus = B_ERROR;
			return fStatus;
		}

		int32 i;
		BString itemPath;
		for (i = 0; i < count; i++) {
			if (info.FindString("items", i, &itemPath) != B_OK) {
				fStatus = B_ERROR;
				return fStatus;
			}
			fInstalledItems.AddItem(new BString(itemPath)); // Or maybe BPath better?
		}
		fIsUpToDate = true;
	}
	else if (fStatus == B_ENTRY_NOT_FOUND) {
		if (create) {
			fStatus = B_OK;
			fIsUpToDate = false;
		}
	}

	return fStatus;
}


status_t
InstalledPackageInfo::AddItem(const char *itemName)
{
	if (!itemName)
		return B_ERROR;

	return fInstalledItems.AddItem(new BString(itemName));
}


status_t
InstalledPackageInfo::Uninstall()
{
	if (fStatus != B_OK)
		return fStatus;

	BString *iter;
	uint32 i, count = fInstalledItems.CountItems();
	BEntry entry;
	status_t ret;

	// Try to remove all entries that are present in the list
	for (i = 0; i < count; i++) {
		iter = static_cast<BString *>(fInstalledItems.ItemAt(count - i - 1));
		ret = entry.SetTo(iter->String());
		if (ret == B_BUSY) {
			// The entry's directory is locked - wait a few cycles for it to
			// unlock itself
			int32 tries = 0;
			for (tries = 0; tries < P_BUSY_TRIES; tries++) {
				ret = entry.SetTo(iter->String());
				if (ret != B_BUSY)
					break;
				// Wait a moment
				usleep(1000);
			}
		}

		if (ret == B_ENTRY_NOT_FOUND)
			continue;
		else if (ret != B_OK) {
			fStatus = B_ERROR;
			return fStatus;
		}

		if (entry.Exists() && entry.Remove() != B_OK) {
			fStatus = B_ERROR;
			return fStatus;
		}
		fInstalledItems.RemoveItem(count - i - 1);
	}

	if (entry.SetTo(fPathToInfo.Path()) != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}
	if (entry.Exists() && entry.Remove() != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}

	return fStatus;
}


status_t
InstalledPackageInfo::Save()
{
	// If the package info is not up to date and everything till now was
	// done correctly, we will save all data as a flattened BMessage to the
	// package info file
	if (fIsUpToDate || fStatus != B_OK)
		return fStatus;

	BFile package;
	if (fCreate) {
		fStatus = package.SetTo(fPathToInfo.Path(), B_WRITE_ONLY | B_CREATE_FILE
			| B_ERASE_FILE);
	}
	else {
		fStatus = package.SetTo(fPathToInfo.Path(), B_WRITE_ONLY | B_ERASE_FILE);
	}

	if (fStatus != B_OK)
		return fStatus;		

	status_t ret;
	int32 i, count = fInstalledItems.CountItems();
	BMessage info(P_PACKAGE_INFO);
	ret = info.AddString("package_name", fName);
	ret |= info.AddString("package_desc", fDescription);
	ret |= info.AddString("package_version", fVersion);
	ret |= info.AddInt64("package_size", fSpaceNeeded);
	ret |= info.AddInt32("file_count", count);
	if (ret != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}
	
	BString *iter;
	for (i = 0; i < count; i++) {
		iter = static_cast<BString *>(fInstalledItems.ItemAt(i));
		if (info.AddString("items", *iter) != B_OK) {
			fStatus = B_ERROR;
			return fStatus;
		}
	}
	
	if (info.Flatten(&package) != B_OK) {
		fStatus = B_ERROR;
		return fStatus;
	}
	fIsUpToDate = true;

	return fStatus;
}


// #pragma mark -


void
InstalledPackageInfo::_ClearItemList()
{
	// Clear the items list
	BString *iter;
	uint32 i, count = fInstalledItems.CountItems();
	for (i = 0; i < count; i++) {
		iter = static_cast<BString *>(fInstalledItems.ItemAt(0));
		fInstalledItems.RemoveItem((int32)0);
		delete iter;
	}
}

