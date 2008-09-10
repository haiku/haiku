/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#include "UninstallView.h"
#include <Alert.h>
#include <String.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <Button.h>
#include <Box.h>
#include <StringView.h>
#include <NodeMonitor.h>
#include <FindDirectory.h>

#include <stdio.h>


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


class UninstallView::InfoItem : public BStringItem {
	public:
		InfoItem(const char *name, const char *version, node_ref ref)
			:	BStringItem(name),
				fVersion(version),
				fNodeRef(ref)
		{
		}

		const char * GetName() { return BStringItem::Text(); }
		const char * GetVersion() { return fVersion.String(); };
		node_ref GetNodeRef() { return fNodeRef; };

	private:
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
	_ClearAppList();

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
	}
	else
		fWatcherRunning = true;

	dir.GetNodeRef(&ref);

	if (watch_node(&ref, B_WATCH_DIRECTORY, this) != B_OK) {
		fWatcherRunning = false;
		return;
	}
}


void
UninstallView::MessageReceived(BMessage *msg)
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
					name = info_get_package_name(filename.String());
					version = info_get_package_version(filename.String());
					fAppList->AddItem(new InfoItem(name.String(), version.String(), ref));
				}
				else {
					// This most likely means we were waiting for the packages/ dir
					// to appear
					if (filename == "packages") {
						if (watch_node(&ref, B_WATCH_DIRECTORY, this) == B_OK)
							fWatcherRunning = true;
					}
				}
			}
			else if (opcode == B_ENTRY_REMOVED) {
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
							fDescription->SetText(T("No package selected"));
						fAppList->RemoveItem(i);
						delete iter;
					}
				}
			}
			else if (opcode == B_ENTRY_MOVED) {
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
				}
				else if (ref.node == from) {
					// Package removed
					// TODO
				}
			}
			break;
		}
		case P_MSG_SELECT:
		{
			int32 index = fAppList->CurrentSelection();
			if (index < 0)
				break;

			fprintf(stderr, "Another debug message...\n");

			InfoItem *item = dynamic_cast<InfoItem *>(fAppList->ItemAt(index));
			if (!item)
				break;

			fprintf(stderr, "Uh: %s and %s\n", item->GetName(), item->GetVersion());

			if (fCurrentSelection.SetTo(item->GetName(), item->GetVersion()) != B_OK)
				break;

			fprintf(stderr, "                     ...how much longer?\n");

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

			BAlert *notify;
			if (fCurrentSelection.Uninstall() == B_OK) {
				BListItem *item = static_cast<BListItem *>(fAppList->ItemAt(index));
				fAppList->RemoveItem(index);
				delete item;

				fDescription->SetText(T("No package selected"));

				notify = new BAlert("removal_success",
						T("The package you selected has been successfully removed "
							"from your system."), T("OK"));
			}
			else
				notify = new BAlert("removal_failed",
						T("The selected package was not removed from your system. The "
							"given installed package information file might have been "
							"corrupted."), T("OK"), NULL, 
						NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

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
	BRect rect = Bounds().InsetBySelf(10.0f, 10.0f);
	rect.bottom = 125.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	fAppList = new BListView(rect, "pkg_list", B_SINGLE_SELECTION_LIST, 
			B_FOLLOW_NONE);
	fAppList->SetSelectionMessage(new BMessage(P_MSG_SELECT));

	BScrollView *scroll = new BScrollView("list_scroll", fAppList, B_FOLLOW_NONE,
			0, false, true);
	AddChild(scroll);

	rect.top = rect.bottom + 8.0f;
	rect.bottom += 120.0f;
	rect.right += B_V_SCROLL_BAR_WIDTH;
	BBox *box = new BBox(rect, "desc_box");

	BStringView *descLabel = new BStringView(BRect(3, 3, 10, 10), "desc_label",
			T("Package description:"));
	descLabel->ResizeToPreferred();
	box->AddChild(descLabel);

	BRect inside = box->Bounds().InsetBySelf(3.0f, 3.0f);
	inside.top = descLabel->Frame().bottom + 10.0f;
	inside.right -= B_V_SCROLL_BAR_WIDTH;
	fDescription = new BTextView(inside, "description", 
			BRect(0, 0, inside.Width(), inside.Height()), B_FOLLOW_NONE, B_WILL_DRAW);
	fDescription->MakeSelectable(true);
	fDescription->MakeEditable(false);
	fDescription->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fDescription->SetText(T("No package selected."));
	
	fDescScroll = new BScrollView("desc_scroll", fDescription, B_FOLLOW_NONE,
			0, false, true, B_NO_BORDER);
	box->AddChild(fDescScroll);

	AddChild(box);

	fButton = new BButton(BRect(0, 0, 1, 1), "removal", T("Remove"),
			new BMessage(P_MSG_REMOVE));
	fButton->ResizeToPreferred();

	rect.top = rect.bottom + 5.0f;
	rect.left = Bounds().Width() - 5.0f - fButton->Bounds().Width();

	fButton->MoveTo(rect.LeftTop());
	AddChild(fButton);

	ResizeTo(Bounds().Width(), fButton->Frame().bottom + 10.0f);
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
	node_ref ref;
	char filename[B_FILE_NAME_LENGTH];
	BString name, version;
	while (1) {
		ret = dir.GetNextEntry(&iter);
		if (ret != B_OK)
			break;
		
		fprintf(stderr, "Found one!\n");
		iter.GetName(filename);
		iter.GetNodeRef(&ref);
		name = info_get_package_name(filename);
		version = info_get_package_version(filename);
		fAppList->AddItem(new InfoItem(name.String(), version.String(), ref));
	}

	if (ret != B_ENTRY_NOT_FOUND)
		return ret;

	return B_OK;
}


void
UninstallView::_ClearAppList()
{
	int32 i, count = fAppList->CountItems();
	BListItem *iter = 0;

	for (i = 0;i < count;i++) {
		iter = static_cast<BListItem *>(fAppList->ItemAt(0));
		fAppList->RemoveItem((int32)0);
		delete iter;
	}
}


void
UninstallView::_CachePathToPackages()
{
	if (find_directory(B_USER_CONFIG_DIRECTORY, &fToPackages) != B_OK)
		return;
	if (fToPackages.Append(kPackagesDir) != B_OK)
		return;
}

