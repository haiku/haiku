/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#include "UninstallView.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <String.h>
#include <StringView.h>

#ifdef __HAIKU__
#	include <GroupLayout.h>
#	include <GroupLayoutBuilder.h>
#	include <SpaceLayoutItem.h>
#endif


enum {
	P_MSG_REMOVE = 'umrm',
	P_MSG_SELECT
};


// Reserved
#define T(x) x


// TODO list:
//	- B_ENTRY_MOVED
//	- Right now the installed package info naming convention is the same
//		as at SoftwareValet. Maybe there would be a better one?
//	- Add a status window (reuse the one from PackageInstall)


static const char* kNoPackageSelected = "No package selected.";

class UninstallView::InfoItem : public BStringItem {
	public:
		InfoItem(const BString& name, const BString& version,
				const char* filename, const node_ref& ref)
			: BStringItem(name.String()),
			  fName(name),
			  fVersion(version),
			  fNodeRef(ref)
		{
			if (fName.Length() == 0)
				SetText(filename);
		}

		const char* GetName() { return fName.String(); }
		const char* GetVersion() { return fVersion.String(); };
		node_ref GetNodeRef() { return fNodeRef; };

	private:
		BString fName;
		BString fVersion;
		node_ref fNodeRef;
};




UninstallView::UninstallView(BRect frame)
	:	BView(frame, "uninstall_view", B_FOLLOW_NONE, 0)
{
	_InitView();
}


UninstallView::~UninstallView()
{
	// Stop all node watching
	stop_watching(this);
}


void
UninstallView::AttachedToWindow()
{
	fAppList->SetTarget(this);
	fButton->SetTarget(this);

	_ReloadAppList();

	// We loaded the list, but now let's set up a node watcher for the packages
	// directory, so that we can update the list of installed packages in real
	// time
	_CachePathToPackages();
	node_ref ref;
	fWatcherRunning = false;
	BDirectory dir(fToPackages.Path());
	if (dir.InitCheck() != B_OK) {
		// The packages/ directory obviously does not exist.
		// Since this is the case, we need to watch for it to appear first

		BPath path;
		fToPackages.GetParent(&path);
		if (dir.SetTo(path.Path()) != B_OK)
			return;
	} else
		fWatcherRunning = true;

	dir.GetNodeRef(&ref);

	if (watch_node(&ref, B_WATCH_DIRECTORY, this) != B_OK) {
		fWatcherRunning = false;
		return;
	}
}


void
UninstallView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) != B_OK)
				break;

			fprintf(stderr, "Got an opcoded node monitor message\n");
			if (opcode == B_ENTRY_CREATED) {
				fprintf(stderr, "Created?...\n");
				BString filename, name, version;
				node_ref ref;
				if (msg->FindString("name", &filename) != B_OK ||
						msg->FindInt32("device", &ref.device) != B_OK ||
						msg->FindInt64("node", &ref.node) != B_OK)
					break;

				// TODO: This obviously is a hack
				//  The node watcher informs the view a bit to early, and because
				//  of this the data of the node is not ready at this moment.
				//  For this reason, we must give the filesystem some time before
				//  continuing.
				usleep(10000);

				if (fWatcherRunning) {
					_AddFile(filename.String(), ref);
				} else {
					// This most likely means we were waiting for the packages/ dir
					// to appear
					if (filename == "packages") {
						if (watch_node(&ref, B_WATCH_DIRECTORY, this) == B_OK)
							fWatcherRunning = true;
					}
				}
			} else if (opcode == B_ENTRY_REMOVED) {
				node_ref ref;
				if (msg->FindInt32("device", &ref.device) != B_OK ||
						msg->FindInt64("node", &ref.node) != B_OK)
					break;

				int32 i, count = fAppList->CountItems();
				InfoItem *iter;
				for (i = 0;i < count;i++) {
					iter = static_cast<InfoItem *>(fAppList->ItemAt(i));
					if (iter->GetNodeRef() == ref) {
						if (i == fAppList->CurrentSelection())
							fDescription->SetText(T(kNoPackageSelected));
						fAppList->RemoveItem(i);
						delete iter;
					}
				}
			} else if (opcode == B_ENTRY_MOVED) {
				ino_t from, to;
				if (msg->FindInt64("from directory", &from) != B_OK ||
						msg->FindInt64("to directory", &to) != B_OK)
					break;

				BDirectory packagesDir(fToPackages.Path());
				node_ref ref;
				packagesDir.GetNodeRef(&ref);

				if (ref.node == to) {
					// Package added
					// TODO
				} else if (ref.node == from) {
					// Package removed
					// TODO
				}
			}
			break;
		}
		case P_MSG_SELECT:
		{
			fButton->SetEnabled(false);
			fDescription->SetText(T(kNoPackageSelected));

			int32 index = fAppList->CurrentSelection();
			if (index < 0)
				break;

			fprintf(stderr, "Another debug message...\n");

			InfoItem* item = dynamic_cast<InfoItem*>(fAppList->ItemAt(index));
			if (!item)
				break;

			fprintf(stderr, "Uh: %s and %s\n", item->GetName(),
				item->GetVersion());

			if (fCurrentSelection.SetTo(item->GetName(),
					item->GetVersion()) != B_OK)
				break;

			fButton->SetEnabled(true);
			fDescription->SetText(fCurrentSelection.GetDescription());
			break;
		}
		case P_MSG_REMOVE:
		{
			if (fCurrentSelection.InitCheck() != B_OK)
				break;

			int32 index = fAppList->CurrentSelection();
			if (index < 0)
				break;

			BAlert* notify;
			if (fCurrentSelection.Uninstall() == B_OK) {
				BListItem* item = fAppList->RemoveItem(index);
				delete item;

				fDescription->SetText(T(kNoPackageSelected));

				notify = new BAlert("removal_success",
					T("The package you selected has been successfully removed "
					"from your system."), T("OK"));
			} else {
				notify = new BAlert("removal_failed",
					T("The selected package was not removed from your system. "
					"The given installed package information file might have "
					"been corrupted."), T("OK"), NULL, 
					NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			}

			notify->Go();
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
UninstallView::_InitView()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

#ifdef __HAIKU__
	BBox* descriptionBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* descriptionLayout = new BGroupLayout(B_VERTICAL, 5);
	descriptionBox->SetLayout(descriptionLayout);

	BBox* buttonBox = new BBox(B_PLAIN_BORDER, NULL);
	BGroupLayout* buttonLayout = new BGroupLayout(B_HORIZONTAL, 5);
	buttonBox->SetLayout(buttonLayout);

	fAppList = new BListView("pkg_list", B_SINGLE_SELECTION_LIST);
	fAppList->SetSelectionMessage(new BMessage(P_MSG_SELECT));
	BScrollView* scrollView = new BScrollView("list_scroll", fAppList,
		B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	BGroupLayout* scrollLayout = new BGroupLayout(B_HORIZONTAL);
	scrollView->SetLayout(scrollLayout);

	BStringView* descriptionLabel = new BStringView("desc_label",
		T("Package description"));
	descriptionLabel->SetFont(be_bold_font);

	fDescription = new BTextView("description", B_WILL_DRAW);
	fDescription->MakeSelectable(false);
	fDescription->MakeEditable(false);
	fDescription->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fDescription->SetText(T(kNoPackageSelected));

	fButton = new BButton("removal", T("Remove"), new BMessage(P_MSG_REMOVE));
	fButton->SetEnabled(false);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(scrollView)
		.Add(BGroupLayoutBuilder(descriptionLayout)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(descriptionLabel)
				.AddGlue()
			)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10))
				.Add(fDescription)
			)
			.SetInsets(5, 5, 5, 5)
		)
		.Add(BGroupLayoutBuilder(buttonLayout)
			.AddGlue()
			.Add(fButton)
			.SetInsets(5, 5, 5, 5)
		)
	);

#else

	BRect rect = Bounds().InsetBySelf(10.0f, 10.0f);
	rect.bottom = 125.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fAppList = new BListView(rect, "pkg_list", B_SINGLE_SELECTION_LIST, 
		B_FOLLOW_NONE);
	fAppList->SetSelectionMessage(new BMessage(P_MSG_SELECT));

	BScrollView *scroll = new BScrollView("list_scroll", fAppList,
		B_FOLLOW_NONE, 0, false, true);
	AddChild(scroll);

	rect.top = rect.bottom + 8.0f;
	rect.bottom += 120.0f;
	rect.right += B_V_SCROLL_BAR_WIDTH;
	BBox* box = new BBox(rect, "desc_box");

	BStringView *descLabel = new BStringView(BRect(3, 3, 10, 10), "desc_label",
		T("Package description:"));
	descLabel->ResizeToPreferred();
	box->AddChild(descLabel);

	BRect inside = box->Bounds().InsetBySelf(3.0f, 3.0f);
	inside.top = descLabel->Frame().bottom + 10.0f;
	inside.right -= B_V_SCROLL_BAR_WIDTH;
	fDescription = new BTextView(inside, "description", 
		BRect(0, 0, inside.Width(), inside.Height()), B_FOLLOW_NONE,
		B_WILL_DRAW);
	fDescription->MakeSelectable(true);
	fDescription->MakeEditable(false);
	fDescription->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fDescription->SetText(T(kNoPackageSelected));
	
	fDescScroll = new BScrollView("desc_scroll", fDescription, B_FOLLOW_NONE,
		0, false, true, B_NO_BORDER);
	box->AddChild(fDescScroll);

	AddChild(box);

	fButton = new BButton(BRect(0, 0, 1, 1), "removal", T("Remove"),
		new BMessage(P_MSG_REMOVE));
	fButton->ResizeToPreferred();
	fButton->SetEnabled(false);

	rect.top = rect.bottom + 5.0f;
	rect.left = Bounds().Width() - 5.0f - fButton->Bounds().Width();

	fButton->MoveTo(rect.LeftTop());
	AddChild(fButton);

	ResizeTo(Bounds().Width(), fButton->Frame().bottom + 10.0f);

#endif
}


status_t
UninstallView::_ReloadAppList()
{
	_ClearAppList();

	if (fToPackages.InitCheck() != B_OK)
		_CachePathToPackages();

	BDirectory dir(fToPackages.Path());
	status_t ret = dir.InitCheck();
	if (ret != B_OK)
		return ret;

	fprintf(stderr, "Ichi! %s\n", fToPackages.Path());

	BEntry iter;
	while (dir.GetNextEntry(&iter) == B_OK) {
		char filename[B_FILE_NAME_LENGTH];
		if (iter.GetName(filename) != B_OK)
			continue;

		node_ref ref;
		if (iter.GetNodeRef(&ref) != B_OK)
			continue;

		printf("Found package '%s'\n", filename);
		_AddFile(filename, ref);
	}

	if (ret != B_ENTRY_NOT_FOUND)
		return ret;

	return B_OK;
}


void
UninstallView::_ClearAppList()
{
	while (BListItem* item = fAppList->RemoveItem(0L))
		delete item;
}


void
UninstallView::_AddFile(const char* filename, const node_ref& ref)
{
	BString name;
	status_t ret = info_get_package_name(filename, name);
	if (ret != B_OK || name.Length() == 0) {
		fprintf(stderr, "Error extracting package name: %s\n",
			strerror(ret));
	}
	BString version;
	ret = info_get_package_version(filename, version);
	if (ret != B_OK || version.Length() == 0) {
		fprintf(stderr, "Error extracting package version: %s\n",
			strerror(ret));
	}
	fAppList->AddItem(new InfoItem(name, version, filename, ref));
}


void
UninstallView::_CachePathToPackages()
{
	if (find_directory(B_USER_CONFIG_DIRECTORY, &fToPackages) != B_OK)
		return;
	if (fToPackages.Append(kPackagesDir) != B_OK)
		return;
}

