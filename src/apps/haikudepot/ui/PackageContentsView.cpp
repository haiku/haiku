/*
 * Copyright 2015, TigerKid001.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageContentsView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <FindDirectory.h>
#include <MessageFormat.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <OutlineListView.h>
#include <Path.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <StringItem.h>

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


//! Layouts the scrollbar so it looks nice with no border and the document
// window look.
class CustomScrollView : public BScrollView {
public:
	CustomScrollView(const char* name, BView* target)
		:
		BScrollView(name, target, 0, false, true, B_NO_BORDER)
	{
	}

	virtual void DoLayout()
	{
		BRect innerFrame = Bounds();
		innerFrame.right -= B_V_SCROLL_BAR_WIDTH + 1;

		BView* target = Target();
		if (target != NULL) {
			Target()->MoveTo(innerFrame.left, innerFrame.top);
			Target()->ResizeTo(innerFrame.Width(), innerFrame.Height());
		}

		BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
		if (scrollBar != NULL) {
			BRect rect = innerFrame;
			rect.left = rect.right + 1;
			rect.right = rect.left + B_V_SCROLL_BAR_WIDTH;
			rect.bottom -= B_H_SCROLL_BAR_HEIGHT;

			scrollBar->MoveTo(rect.left, rect.top);
			scrollBar->ResizeTo(rect.Width(), rect.Height());
		}
	}
};

// #pragma mark - PackageEntryItem


class PackageEntryItem : public BStringItem {
public:
	PackageEntryItem(const BPackageEntry* entry)
		:
		BStringItem(entry->Name()),
		fEntry(entry)
	{
		printf("PackageEntryItem(%s)\n", entry->Name());
	}
	
	inline const BPackageEntry* PackageEntry() const
	{
		return fEntry;
	}

private:
	const BPackageEntry* fEntry;
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
		printf("HandleEntry(%s/%s)\n",
			entry->Parent() != NULL ? entry->Parent()->Name() : "NULL",
			entry->Name());

		{
			// Check if we are still supposed to popuplate the list
			BAutolock lock(&fPackageLock);
			if (fPackageInfoRef.Get() != fPackageInfoToPopulate)
				return B_ERROR;
		}

		if (!fListView->LockLooper())
			return B_ERROR;

		PackageEntryItem* item = new PackageEntryItem(entry);

		if (entry->Parent() == NULL) {
			printf("  adding root entry\n");
			fListView->AddItem(item);
			fLastParentEntry = NULL;
			fLastParentItem = NULL;
		} else if (entry->Parent() == fLastEntry) {
			printf("  adding to last entry %s\n", fLastEntry->Name());
			fListView->AddUnder(item, fLastItem);
			fLastParentEntry = fLastEntry;
			fLastParentItem = fLastItem;
		} else if (entry->Parent() == fLastParentEntry) {
			printf("  adding to last parent %s\n", fLastParentEntry->Name());
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
				if (listItem->PackageEntry() == entry->Parent()) {
					fLastParentEntry = listItem->PackageEntry();
					fLastParentItem = listItem;
					printf("  found parent %s\n", listItem->Text());
					fListView->AddUnder(item, listItem);
					foundParent = true;
					break;
				}
			}
			if (!foundParent) {
				// NOTE: Should not happen. Just add this entry at the
				// root level.
				printf("Did not find parent entry for %s (%s)!\n",
					entry->Name(), entry->Parent()->Name());
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
	fPackageLock("package contents populator lock")
{
	fContentListView = new BOutlineListView("content list view", 
		B_SINGLE_SELECTION_LIST);
	
	BScrollView* scrollView = new CustomScrollView("contents scroll view", 
		fContentListView);
															
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
	if (fPackage == package)
		return;

	printf("PackageContentsView::SetPackage(%s)\n",
		package.Get() != NULL ? package->Title().String() : "NULL");

	Clear();

	{
		BAutolock lock(&fPackageLock);
		fPackage = package;
	}
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

		if (package.Get() != NULL)
			view->_PopuplatePackageContens(*package.Get());
	}

	return 0;
}


void
PackageContentsView::_PopuplatePackageContens(const PackageInfo& package)
{
	BPath packagePath;

	// Obtain path to the package file
	if (package.IsLocalFile()) {
		BString pathString = package.LocalFilePath();
		packagePath.SetTo(pathString.String());
	} else {
		int32 installLocation = _InstallLocation(package);
		if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
			if (find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &packagePath)
				!= B_OK) {
				return;
			}
		} else if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_HOME) {
			if (find_directory(B_USER_PACKAGES_DIRECTORY, &packagePath)
				!= B_OK) {
				return;
			}
		} else {
			printf("PackageContentsView::_PopuplatePackageContens(): "
				"unknown install location");
			return;
		}

		packagePath.Append(package.FileName());
	}

	// Setup a BPackageReader
	BNoErrorOutput errorOutput;
	BPackageReader reader(&errorOutput);

	status_t status = reader.Init(packagePath.Path());
	if (status != B_OK) {
		printf("PackageContentsView::_PopuplatePackageContens(): "
			"failed to init BPackageReader(%s): %s\n",
			packagePath.Path(), strerror(status));
		return;
	}

	// Scan package contents and populate list
	PackageContentOutliner contentHandler(fContentListView, &package,
		fPackageLock, fPackage);
	status = reader.ParseContent(&contentHandler);
	if (status != B_OK) {
		printf("PackageContentsView::SetPackage(): "
			"failed parse package contents: %s\n", strerror(status));
	}
}


int32
PackageContentsView::_InstallLocation(const PackageInfo& package) const
{
	const PackageInstallationLocationSet& locations
		= package.InstallationLocations();

	// If the package is already installed, return its first installed location
	if (locations.size() != 0)
		return *locations.begin();

	return B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
}
