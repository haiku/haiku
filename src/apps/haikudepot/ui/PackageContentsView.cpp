/*
 * Copyright 2015, TigerKid001.
 * Copyright 2020-2024, Andrew Lindesay <apl@lindesay.co.nz>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageContentsView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <OutlineListView.h>
#include <Path.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <StringFormat.h>
#include <StringItem.h>

#include "GeneralContentScrollView.h"
#include "Logger.h"
#include "PackageKitUtils.h"
#include "PackageUtils.h"

#include <package/PackageDefs.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageReader.h>

using namespace BPackageKit;

using BPackageKit::BHPKG::BNoErrorOutput;
using BPackageKit::BHPKG::BPackageContentHandler;
using BPackageKit::BHPKG::BPackageEntry;
using BPackageKit::BHPKG::BPackageEntryAttribute;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageReader;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageContentsView"


// #pragma mark - PackageEntryItem


class PackageEntryItem : public BStringItem {
public:
	PackageEntryItem(const BPackageEntry* entry, const BString& path)
		:
		BStringItem(entry->Name()),
		fPath(path)
	{
		if (fPath.Length() > 0)
			fPath.Append("/");
		fPath.Append(entry->Name());
	}

	inline const BString& EntryPath() const
	{
		return fPath;
	}

private:
	BString fPath;
};


// #pragma mark - PackageContentOutliner


class PackageContentOutliner : public BPackageContentHandler {
public:
	PackageContentOutliner(BOutlineListView* listView,
			const PackageInfo* packageInfo,
			BLocker& packageLock, PackageInfoRef& packageInfoRef)
		:
		fListView(listView),
		fLastParentEntry(NULL),
		fLastParentItem(NULL),
		fLastEntry(NULL),
		fLastItem(NULL),

		fPackageInfoToPopulate(packageInfo),
		fPackageLock(packageLock),
		fPackageInfoRef(packageInfoRef)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (fListView->LockLooperWithTimeout(1000000) != B_OK)
			return B_ERROR;

		// Check if we are still supposed to popuplate the list
		if (fPackageInfoRef.Get() != fPackageInfoToPopulate) {
			fListView->UnlockLooper();
			return B_ERROR;
		}

		BString path;
		const BPackageEntry* parent = entry->Parent();
		while (parent != NULL) {
			if (path.Length() > 0)
				path.Prepend("/");
			path.Prepend(parent->Name());
			parent = parent->Parent();
		}

		PackageEntryItem* item = new PackageEntryItem(entry, path);

		if (entry->Parent() == NULL) {
			fListView->AddItem(item);
			fLastParentEntry = NULL;
			fLastParentItem = NULL;
		} else if (entry->Parent() == fLastEntry) {
			fListView->AddUnder(item, fLastItem);
			fLastParentEntry = fLastEntry;
			fLastParentItem = fLastItem;
		} else if (entry->Parent() == fLastParentEntry) {
			fListView->AddUnder(item, fLastParentItem);
		} else {
			// Not the last parent entry, need to search for the parent
			// among the already added list items.
			bool foundParent = false;
			for (int32 i = 0; i < fListView->FullListCountItems(); i++) {
				PackageEntryItem* listItem
					= dynamic_cast<PackageEntryItem*>(
						fListView->FullListItemAt(i));
				if (listItem == NULL)
					continue;
				if (listItem->EntryPath() == path) {
					fLastParentEntry = entry->Parent();
					fLastParentItem = listItem;
					fListView->AddUnder(item, listItem);
					foundParent = true;
					break;
				}
			}
			if (!foundParent) {
				// NOTE: Should not happen. Just add this entry at the
				// root level.
				fListView->AddItem(item);
				fLastParentEntry = NULL;
				fLastParentItem = NULL;
			}
		}

		fLastEntry = entry;
		fLastItem = item;

		fListView->UnlockLooper();

		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	BOutlineListView*		fListView;

	const BPackageEntry*	fLastParentEntry;
	PackageEntryItem*		fLastParentItem;

	const BPackageEntry*	fLastEntry;
	PackageEntryItem*		fLastItem;

	const PackageInfo*		fPackageInfoToPopulate;
	BLocker&				fPackageLock;
	PackageInfoRef&			fPackageInfoRef;
};


// #pragma mark - PackageContentView


PackageContentsView::PackageContentsView(const char* name)
	:
	BView("package_contents_view", B_WILL_DRAW),
	fPackageLock("package contents populator lock"),
	fLastPackageState(NONE)
{
	fContentListView = new BOutlineListView("content list view",
		B_SINGLE_SELECTION_LIST);

	BScrollView* scrollView = new GeneralContentScrollView(
		"contents scroll view", fContentListView);

	BLayoutBuilder::Group<>(this)
		.Add(scrollView, 1.0f)
		.SetInsets(0.0f, -1.0f, -1.0f, -1.0f)
	;

	_InitContentPopulator();
}


PackageContentsView::~PackageContentsView()
{
	Clear();

	delete_sem(fContentPopulatorSem);
	if (fContentPopulator >= 0)
		wait_for_thread(fContentPopulator, NULL);
}


void
PackageContentsView::AttachedToWindow()
{
	BView::AttachedToWindow();
}


void
PackageContentsView::AllAttached()
{
	BView::AllAttached();
}


void
PackageContentsView::SetPackage(const PackageInfoRef& package)
{
	PackageState packageState = PackageUtils::State(package);

	// When getting a ref to the same package, don't return when the
	// package state has changed, since in that case, we may now be able
	// to read contents where we previously could not. (For example, the
	// package has been installed.)
	if (fPackage == package && (!package.IsSet() || packageState == fLastPackageState))
		return;

	Clear();

	{
		BAutolock lock(&fPackageLock);
		fPackage = package;
		fLastPackageState = packageState;
	}

	// if the package is not installed and is not a local file on disk then
	// there is no point in attempting to populate data for it.

	if (PackageUtils::IsActivatedOrLocalFile(package))
		release_sem_etc(fContentPopulatorSem, 1, 0);
}


void
PackageContentsView::Clear()
{
	{
		BAutolock lock(&fPackageLock);
		fPackage.Unset();
	}

	fContentListView->MakeEmpty();
}


// #pragma mark - private


void
PackageContentsView::_InitContentPopulator()
{
	fContentPopulatorSem = create_sem(0, "PopulatePackageContents");
	if (fContentPopulatorSem >= 0) {
		fContentPopulator = spawn_thread(&_ContentPopulatorThread,
			"Package Contents Populator", B_NORMAL_PRIORITY, this);
		if (fContentPopulator >= 0)
			resume_thread(fContentPopulator);
	} else
		fContentPopulator = -1;
}


/*static*/ int32
PackageContentsView::_ContentPopulatorThread(void* arg)
{
	PackageContentsView* view = reinterpret_cast<PackageContentsView*>(arg);

	while (acquire_sem(view->fContentPopulatorSem) == B_OK) {
		PackageInfoRef package;
		{
			BAutolock lock(&view->fPackageLock);
			package = view->fPackage;
		}

		if (package.IsSet()) {
			if (!view->_PopulatePackageContents(package)) {
				if (view->LockLooperWithTimeout(1000000) == B_OK) {
					view->fContentListView->AddItem(
						new BStringItem(B_TRANSLATE("<Package contents not "
							"available for remote packages>")));
					view->UnlockLooper();
				}
			}
		}
	}

	return 0;
}


bool
PackageContentsView::_PopulatePackageContents(const PackageInfoRef& package)
{
	BPath packagePath;

	if (PackageKitUtils::DeriveLocalFilePath(package, packagePath) != B_OK) {
		HDDEBUG("unable to obtain local file path");
		return false;
	}

	// Setup a BPackageReader
	BNoErrorOutput errorOutput;
	BPackageReader reader(&errorOutput);

	status_t status = reader.Init(packagePath.Path());
	if (status != B_OK) {
		HDINFO("PackageContentsView::_PopulatePackageContents(): "
			"failed to init BPackageReader(%s): %s",
			packagePath.Path(), strerror(status));
		return false;
	}

	// Scan package contents and populate list
	PackageContentOutliner contentHandler(fContentListView, package.Get(), fPackageLock, fPackage);
	status = reader.ParseContent(&contentHandler);
	if (status != B_OK) {
		HDINFO("PackageContentsView::_PopulatePackageContents(): "
			"failed parse package contents: %s", strerror(status));
		// NOTE: Do not return false, since it taken to mean this
		// is a remote package, but is it not, we simply want to stop
		// populating the contents early.
	}
	return true;
}
