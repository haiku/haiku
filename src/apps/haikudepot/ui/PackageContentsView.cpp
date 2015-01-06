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


//	#pragma mark - PackageContentOutliner


class PackageContentsView::PackageContentOutliner
			: public BPackageContentHandler {
	public:
	PackageContentOutliner(BOutlineListView* contentsList)
	{
		fContents = contentsList;
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		if (entry->Parent() == NULL) {
			fSuperParent = entry;
			fSuperParentItem = new BStringItem(fSuperParent->Name());
			fContents->AddItem(fSuperParentItem);
			fParent = fSuperParent;
			fParentItem = fSuperParentItem;
		} else {
			if (entry->Parent() == fParent) {
				BListItem* item = new BStringItem(entry->Name());
				fContents->AddUnder(item, fParentItem);
				fParent = entry;
				fParentItem = item;
			} else if (entry->Parent() == fSuperParent) {
				BListItem* item = new BStringItem(entry->Name());
				fContents->AddUnder(item, fSuperParentItem);
				fParent = entry;
				fParentItem = item;
			}
		}
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
	
	BOutlineListView* GetContentsList() {
		return fContents;
	}
	
	private:
	BOutlineListView* fContents;
	BListItem* fParentItem;
	BListItem* fSuperParentItem;
	BPackageEntry* fParent;
	BPackageEntry* fSuperParent;
};


// #pragma mark - PackageContentView


PackageContentsView::PackageContentsView(BRect frame, const char* name)
	:
	BView("package_contents_view",B_WILL_DRAW),
	fLayout(new BGroupLayout(B_HORIZONTAL))
{
	BRect frame = Bounds();
	frame.InsetBy(5,5);
	
	fContentsList = new BOutlineListView(frame, "contents_list", 
									B_SINGLE_SELECTION_LIST);
	
	BScrollView* scrollView = new CustomScrollView("contents scroll view", 
															fContentsList);
															
	BLayoutBuilder::Group<>(this)
		.Add(scrollView, 1.0f)
		.SetInsets(B_USE_DEFAULT_SPACING, -1.0f, -1.0f, -1.0f)
	;
}


PackageContentsView::~PackageContentsView()
{
	MakeEmpty();
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
PackageContentsView::AddPackage(const PackageInfo& package)
{
	fContentsList->MakeEmpty();
	status_t status;
	
	if (package.IsLocalFile() ) {
		BString pathString = package.LocalFilePath();
		BPath packagePath;
		packagePath.SetTo(pathString.String());
	
		BNoErrorOutput errorOutput;
		BPackageReader reader(&errorOutput);
	
		status = reader.Init(packagePath.Path());
			if (status != B_OK) {
				printf("failed to init BPackageReader(%s): %s\n",
					packagePath.Path(), strerror(status));
				return;
			}
	
		// Scan package contents and populate list
		PackageContentOutliner contentHandler(fContentsList);
		status = reader.ParseContent(&contentHandler);
		if (status != B_OK) {
			printf("PackageContnetsView::PackageContentsView(): "
				"failed parse package contents: %s\n", strerror(status));
		} else {
			fContentsList = contentHandler.GetContentsList();
		}
	}
	Invalidate();
}


void 
PackageContentsView::MakeEmpty() {
	fContentsList->MakeEmpty();
}
