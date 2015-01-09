/*
 * Copyright 2015, TigerKid001.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageContentsView.h"

#include <algorithm>
#include <stdio.h>

#include <Autolock.h>
#include <Catalog.h>
#include <MessageFormat.h>
#include <ScrollBar.h>
#include <Window.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <OutlineListView.h>
#include <Path.h>
#include <ScrollView.h>

#include <package/hpkg/PackageReader.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include "MainWindow.h"
#include "PackageAction.h"

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
	}
	
	inline const BPackageEntry* PackageEntry() const
	{
		return fEntry;
	}

private:
	const BPackageEntry* fEntry;
};


// #pragma mark - PackageContentOutliner


class PackageContentsView::PackageContentOutliner
	: public BPackageContentHandler {

public:
	PackageContentOutliner(BOutlineListView* listView)
		:
		fListView(listView),
		fLastParentEntry(NULL),
		fLastParentItem(NULL),
		fLastEntry(NULL),
		fLastItem(NULL)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		PackageEntryItem* item = new PackageEntryItem(entry);

		if (entry->Parent() == NULL) {
			fListView->AddItem(item);
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
			for (int32 i = 0; i < fListView->CountItems(); i++) {
				PackageEntryItem* listItem
					= dynamic_cast<PackageEntryItem*>(fListView->ItemAt(i));
				if (listItem == NULL)
					continue;
				if (listItem->PackageEntry() == entry->Parent()) {
					fLastParentEntry = listItem->PackageEntry();
					fLastParentItem = listItem;
					fListView->AddUnder(item, fLastParentItem);
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
			}
		}

		fLastEntry = entry;
		fLastItem = item;

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
};


// #pragma mark - PackageContentView


PackageContentsView::PackageContentsView(const char* name)
	:
	BView("package_contents_view", B_WILL_DRAW),
	fLayout(new BGroupLayout(B_HORIZONTAL))
{
	fContentListView = new BOutlineListView("content list view", 
		B_SINGLE_SELECTION_LIST);
	
	BScrollView* scrollView = new CustomScrollView("contents scroll view", 
		fContentListView);
															
	BLayoutBuilder::Group<>(this)
		.Add(scrollView, 1.0f)
		.SetInsets(0.0f, -1.0f, -1.0f, -1.0f)
	;
}


PackageContentsView::~PackageContentsView()
{
	Clear();
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
PackageContentsView::SetPackage(const PackageInfo& package)
{
	Clear();

	if (package.IsLocalFile() ) {
		BString pathString = package.LocalFilePath();
		BPath packagePath;
		packagePath.SetTo(pathString.String());
	
		BNoErrorOutput errorOutput;
		BPackageReader reader(&errorOutput);
	
		status_t status = reader.Init(packagePath.Path());
		if (status != B_OK) {
			printf("PackageContentsView::SetPackage(): failed to init "
				"BPackageReader(%s): %s\n",
				packagePath.Path(), strerror(status));
			return;
		}
	
		// Scan package contents and populate list
		PackageContentOutliner contentHandler(fContentListView);
		status = reader.ParseContent(&contentHandler);
		if (status != B_OK) {
			printf("PackageContentsView::SetPackage(): "
				"failed parse package contents: %s\n", strerror(status));
		}
	}
}


void 
PackageContentsView::Clear()
{
	fContentListView->MakeEmpty();
}
