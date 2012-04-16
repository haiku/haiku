/*
 * Copyright (c) 2007-2010, Haiku, Inc.
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
#include <Catalog.h>
#include <ControlLook.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <NodeMonitor.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <TextView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UninstallView"


enum {
	P_MSG_REMOVE = 'umrm',
	P_MSG_SELECT
};


// TODO list:
//	- B_ENTRY_MOVED
//	- Right now the installed package info naming convention is the same
//		as at SoftwareValet. Maybe there would be a better one?
//	- Add a status window (reuse the one from PackageInstall)


class UninstallView::InfoItem : public BStringItem {
public:
	InfoItem(const BString& name, const BString& version,
			const char* filename, const node_ref& ref)
		:
		BStringItem(name.String()),
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
	BString		fName;
	BString 	fVersion;
	node_ref	fNodeRef;
};




UninstallView::UninstallView()
	:
	BGroupView(B_VERTICAL)
{
	fNoPackageSelectedString = B_TRANSLATE("No package selected.");
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
				if (msg->FindString("name", &filename) != B_OK
					|| msg->FindInt32("device", &ref.device) != B_OK
					|| msg->FindInt64("node", &ref.node) != B_OK)
					break;

				// TODO: This obviously is a hack
				// The node watcher informs the view a bit to early, and
				// because of this the data of the node is not ready at this
				// moment. For this reason, we must give the filesystem some
				// time before continuing.
				usleep(10000);

				if (fWatcherRunning) {
					_AddFile(filename.String(), ref);
				} else {
					// This most likely means we were waiting for
					// the packages/ dir to appear
					if (filename == "packages") {
						if (watch_node(&ref, B_WATCH_DIRECTORY, this) == B_OK)
							fWatcherRunning = true;
					}
				}
			} else if (opcode == B_ENTRY_REMOVED) {
				node_ref ref;
				if (msg->FindInt32("device", &ref.device) != B_OK
					|| msg->FindInt64("node", &ref.node) != B_OK)
					break;

				int32 i, count = fAppList->CountItems();
				InfoItem* iter;
				for (i = 0; i < count; i++) {
					iter = static_cast<InfoItem *>(fAppList->ItemAt(i));
					if (iter->GetNodeRef() == ref) {
						if (i == fAppList->CurrentSelection())
							fDescription->SetText(fNoPackageSelectedString);
						fAppList->RemoveItem(i);
						delete iter;
					}
				}
			} else if (opcode == B_ENTRY_MOVED) {
				ino_t from, to;
				if (msg->FindInt64("from directory", &from) != B_OK
					|| msg->FindInt64("to directory", &to) != B_OK)
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
			fDescription->SetText(fNoPackageSelectedString);

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

				fDescription->SetText(fNoPackageSelectedString);

				notify = new BAlert("removal_success",
					B_TRANSLATE("The package you selected has been "
					"successfully removed from your system."),
					B_TRANSLATE("OK"));
			} else {
				notify = new BAlert("removal_failed",
					B_TRANSLATE(
					"The selected package was not removed from your system. "
					"The given installed package information file might have "
					"been corrupted."), B_TRANSLATE("OK"), NULL,
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

	fAppList = new BListView("pkg_list", B_SINGLE_SELECTION_LIST);
	fAppList->SetSelectionMessage(new BMessage(P_MSG_SELECT));
	BScrollView* scrollView = new BScrollView("list_scroll", fAppList,
		0, false, true, B_NO_BORDER);

	BStringView* descriptionLabel = new BStringView("desc_label",
		B_TRANSLATE("Package description"));
	descriptionLabel->SetFont(be_bold_font);

	fDescription = new BTextView("description", B_WILL_DRAW);
	fDescription->MakeSelectable(false);
	fDescription->MakeEditable(false);
	fDescription->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fDescription->SetText(fNoPackageSelectedString);

	fButton = new BButton("removal", B_TRANSLATE("Remove"),
		new BMessage(P_MSG_REMOVE));
	fButton->SetEnabled(false);

	const float spacing = be_control_look->DefaultItemSpacing();

	BGroupLayoutBuilder builder(GroupLayout());
	builder.Add(scrollView)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_VERTICAL)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(descriptionLabel)
				.AddGlue()
			)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
				.Add(BSpaceLayoutItem::CreateHorizontalStrut(10))
				.Add(fDescription)
			)
			.SetInsets(spacing, spacing, spacing, spacing)
		)
		.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddGlue()
			.Add(fButton)
			.SetInsets(spacing, spacing, spacing, spacing)
		)
	;
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
	if (ret != B_OK || name.Length() == 0)
		fprintf(stderr, "Error extracting package name: %s\n", strerror(ret));
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

