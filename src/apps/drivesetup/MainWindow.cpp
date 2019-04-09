/*
 * Copyright 2002-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "MainWindow.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Catalog.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ControlLook.h>
#include <Debug.h>
#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <DiskDeviceTypes.h>
#include <DiskSystem.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <Menu.h>
#include <Path.h>
#include <Partition.h>
#include <PartitioningInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <fs_volume.h>
#include <tracker_private.h>

#include "ChangeParametersPanel.h"
#include "ColumnListView.h"
#include "CreateParametersPanel.h"
#include "DiskView.h"
#include "InitParametersPanel.h"
#include "PartitionList.h"
#include "Support.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainWindow"


enum {
	MSG_MOUNT_ALL				= 'mnta',
	MSG_MOUNT					= 'mnts',
	MSG_UNMOUNT					= 'unmt',
	MSG_FORMAT					= 'frmt',
	MSG_CREATE					= 'crtp',
	MSG_CHANGE					= 'chgp',
	MSG_INITIALIZE				= 'init',
	MSG_DELETE					= 'delt',
	MSG_EJECT					= 'ejct',
	MSG_OPEN_DISKPROBE			= 'opdp',
	MSG_SURFACE_TEST			= 'sfct',
	MSG_RESCAN					= 'rscn',
	MSG_REGISTER				= 'regi',
	MSG_UNREGISTER				= 'unrg',
	MSG_SAVE					= 'save',
	MSG_WRITE					= 'writ',
	MSG_COMPLETE				= 'comp',

	MSG_PARTITION_ROW_SELECTED	= 'prsl',
};


class ListPopulatorVisitor : public BDiskDeviceVisitor {
public:
	ListPopulatorVisitor(PartitionListView* list, int32& diskCount,
			SpaceIDMap& spaceIDMap)
		:
		fPartitionList(list),
		fDiskCount(diskCount),
		fSpaceIDMap(spaceIDMap)
	{
		fDiskCount = 0;
		fSpaceIDMap.Clear();
		// start with an empty list
		int32 rows = fPartitionList->CountRows();
		for (int32 i = rows - 1; i >= 0; i--) {
			BRow* row = fPartitionList->RowAt(i);
			fPartitionList->RemoveRow(row);
			delete row;
		}
	}

	virtual bool Visit(BDiskDevice* device)
	{
		fDiskCount++;
		// if we don't prepare the device for modifications,
		// we cannot get information about available empty
		// regions on the device or child partitions
		device->PrepareModifications();
		_AddPartition(device);
		return false; // Don't stop yet!
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		_AddPartition(partition);
		return false; // Don't stop yet!
	}

private:
	void _AddPartition(BPartition* partition) const
	{
		// add the partition itself
		fPartitionList->AddPartition(partition);

		// add any available space on it
		BPartitioningInfo info;
		status_t status = partition->GetPartitioningInfo(&info);
		if (status >= B_OK) {
			partition_id parentID = partition->ID();
			off_t offset;
			off_t size;
			for (int32 i = 0;
					info.GetPartitionableSpaceAt(i, &offset, &size) >= B_OK;
					i++) {
				// TODO: remove again once Disk Device API is fixed
				if (!is_valid_partitionable_space(size))
					continue;
				//
				partition_id id = fSpaceIDMap.SpaceIDFor(parentID, offset);
				fPartitionList->AddSpace(parentID, id, offset, size);
			}
		}
	}

	PartitionListView*	fPartitionList;
	int32&				fDiskCount;
	SpaceIDMap&			fSpaceIDMap;
	BDiskDevice*		fLastPreparedDevice;
};


class MountAllVisitor : public BDiskDeviceVisitor {
public:
	MountAllVisitor()
	{
	}

	virtual bool Visit(BDiskDevice* device)
	{
		if (device->ContainsFileSystem())
			device->Mount();

		return false; // Don't stop yet!
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		partition->Mount();
		return false; // Don't stop yet!
	}

private:
	PartitionListView* fPartitionList;
};


class ModificationPreparer {
public:
	ModificationPreparer(BDiskDevice* disk)
		:
		fDisk(disk),
		fModificationStatus(fDisk->PrepareModifications())
	{
	}
	~ModificationPreparer()
	{
		if (fModificationStatus == B_OK)
			fDisk->CancelModifications();
	}
	status_t ModificationStatus() const
	{
		return fModificationStatus;
	}
	status_t CommitModifications()
	{
		status_t status = fDisk->CommitModifications();
		if (status == B_OK)
			fModificationStatus = B_ERROR;

		return status;
	}

private:
	BDiskDevice*	fDisk;
	status_t		fModificationStatus;
};


// #pragma mark -


MainWindow::MainWindow()
	:
	BWindow(BRect(50, 50, 600, 500), B_TRANSLATE_SYSTEM_NAME("DriveSetup"),
		B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fCurrentDisk(NULL),
	fCurrentPartitionID(-1),
	fSpaceIDMap()
{
	fMenuBar = new BMenuBar("root menu");

	// create all the menu items
	fWipeMenuItem = new BMenuItem(B_TRANSLATE("Wipe (not implemented)"),
		new BMessage(MSG_FORMAT));
	fEjectMenuItem = new BMenuItem(B_TRANSLATE("Eject"),
		new BMessage(MSG_EJECT), 'E');
	fOpenDiskProbeMenuItem = new BMenuItem(B_TRANSLATE("Open with DiskProbe"),
		new BMessage(MSG_OPEN_DISKPROBE));

	fSurfaceTestMenuItem = new BMenuItem(
		B_TRANSLATE("Surface test (not implemented)"),
		new BMessage(MSG_SURFACE_TEST));
	fRescanMenuItem = new BMenuItem(B_TRANSLATE("Rescan"),
		new BMessage(MSG_RESCAN));

	fCreateMenuItem = new BMenuItem(B_TRANSLATE("Create" B_UTF8_ELLIPSIS),
		new BMessage(MSG_CREATE), 'C');
	fChangeMenuItem = new BMenuItem(
		B_TRANSLATE("Change parameters" B_UTF8_ELLIPSIS),
		new BMessage(MSG_CHANGE));
	fDeleteMenuItem = new BMenuItem(B_TRANSLATE("Delete"),
		new BMessage(MSG_DELETE), 'D');

	fMountMenuItem = new BMenuItem(B_TRANSLATE("Mount"),
		new BMessage(MSG_MOUNT), 'M');
	fUnmountMenuItem = new BMenuItem(B_TRANSLATE("Unmount"),
		new BMessage(MSG_UNMOUNT), 'U');
	fMountAllMenuItem = new BMenuItem(B_TRANSLATE("Mount all"),
		new BMessage(MSG_MOUNT_ALL), 'M', B_SHIFT_KEY);

	fRegisterMenuItem = new BMenuItem(
		B_TRANSLATE("Register disk image" B_UTF8_ELLIPSIS),
		new BMessage(MSG_REGISTER));
	fUnRegisterMenuItem = new BMenuItem(
		B_TRANSLATE("Unregister disk image"),
		new BMessage(MSG_UNREGISTER));
	fSaveMenuItem = new BMenuItem(
		B_TRANSLATE("Save image" B_UTF8_ELLIPSIS),
		new BMessage(MSG_SAVE));
	fWriteMenuItem = new BMenuItem(
		B_TRANSLATE("Write image" B_UTF8_ELLIPSIS),
		new BMessage(MSG_WRITE));

	// Disk menu
	fDiskMenu = new BMenu(B_TRANSLATE("Disk"));

	// fDiskMenu->AddItem(fWipeMenuItem);
	fDiskInitMenu = new BMenu(B_TRANSLATE("Initialize"));
	fDiskMenu->AddItem(fDiskInitMenu);

	fDiskMenu->AddSeparatorItem();

	fDiskMenu->AddItem(fEjectMenuItem);
	// fDiskMenu->AddItem(fSurfaceTestMenuItem);
	fDiskMenu->AddItem(fRescanMenuItem);

	fMenuBar->AddItem(fDiskMenu);

	// Parition menu
	fPartitionMenu = new BMenu(B_TRANSLATE("Partition"));
	fPartitionMenu->AddItem(fCreateMenuItem);

	fFormatMenu = new BMenu(B_TRANSLATE("Format"));
	fPartitionMenu->AddItem(fFormatMenu);

	fPartitionMenu->AddItem(fChangeMenuItem);
	fPartitionMenu->AddItem(fDeleteMenuItem);

	fPartitionMenu->AddSeparatorItem();

	fPartitionMenu->AddItem(fMountMenuItem);
	fPartitionMenu->AddItem(fUnmountMenuItem);

	fPartitionMenu->AddSeparatorItem();

	fPartitionMenu->AddItem(fMountAllMenuItem);

	fPartitionMenu->AddSeparatorItem();

	fPartitionMenu->AddItem(fOpenDiskProbeMenuItem);
	fMenuBar->AddItem(fPartitionMenu);

	// Disk image menu
	fDiskImageMenu = new BMenu(B_TRANSLATE("Disk image"));

	fDiskImageMenu->AddItem(fRegisterMenuItem);
	fDiskImageMenu->AddItem(fUnRegisterMenuItem);

	fDiskImageMenu->AddSeparatorItem();

	fDiskImageMenu->AddItem(fSaveMenuItem);
	fDiskImageMenu->AddItem(fWriteMenuItem);

	fMenuBar->AddItem(fDiskImageMenu);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	layout->SetInsets(-1, 0, -1, -1);
	SetLayout(layout);
	AddChild(fMenuBar);

	// Partition / Drives context menu
	fContextMenu = new BPopUpMenu("Partition", false, false);
	fCreateContextMenuItem = new BMenuItem(B_TRANSLATE("Create" B_UTF8_ELLIPSIS),
		new BMessage(MSG_CREATE), 'C');
	fChangeContextMenuItem = new BMenuItem(
		B_TRANSLATE("Change parameters" B_UTF8_ELLIPSIS),
		new BMessage(MSG_CHANGE));
	fDeleteContextMenuItem = new BMenuItem(B_TRANSLATE("Delete"),
		new BMessage(MSG_DELETE), 'D');
	fMountContextMenuItem = new BMenuItem(B_TRANSLATE("Mount"),
		new BMessage(MSG_MOUNT), 'M');
	fUnmountContextMenuItem = new BMenuItem(B_TRANSLATE("Unmount"),
		new BMessage(MSG_UNMOUNT), 'U');
	fOpenDiskProbeContextMenuItem = new BMenuItem(B_TRANSLATE("Open with DiskProbe"),
		new BMessage(MSG_OPEN_DISKPROBE));
	fFormatContextMenuItem = new BMenu(B_TRANSLATE("Format"));

	fContextMenu->AddItem(fCreateContextMenuItem);
	fContextMenu->AddItem(fFormatContextMenuItem);
	fContextMenu->AddItem(fChangeContextMenuItem);
	fContextMenu->AddItem(fDeleteContextMenuItem);
	fContextMenu->AddSeparatorItem();
	fContextMenu->AddItem(fMountContextMenuItem);
	fContextMenu->AddItem(fUnmountContextMenuItem);
	fContextMenu->AddSeparatorItem();
	fContextMenu->AddItem(fOpenDiskProbeContextMenuItem);
	fContextMenu->SetTargetForItems(this);

	// add DiskView
	fDiskView = new DiskView(fSpaceIDMap);
	AddChild(fDiskView);

	// add PartitionListView
	fListView = new PartitionListView();
	AddChild(fListView);

	// configure PartitionListView
	fListView->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fListView->SetSelectionMessage(new BMessage(MSG_PARTITION_ROW_SELECTED));
	fListView->SetTarget(this);
	fListView->MakeFocus(true);

	fRegisterFilePanel = NULL;
	fWriteImageFilePanel = NULL;
	fReadImageFilePanel = NULL;

	fNotification = new BNotification(B_INFORMATION_NOTIFICATION);
	fNotification->SetGroup(BString(B_TRANSLATE_SYSTEM_NAME("DriveSetup")));

	app_info info;
	be_roster->GetAppInfo("application/x-vnd.Haiku-DriveSetup", &info);
	BBitmap icon(BRect(0, 0, 32, 32), B_RGBA32);
	BNode node(&info.ref);
	if (BIconUtils::GetVectorIcon(&node, "BEOS:ICON", &icon) == B_OK)
		fNotification->SetIcon(&icon);

	status_t status = fDiskDeviceRoster.StartWatching(BMessenger(this));
	if (status != B_OK) {
		fprintf(stderr, "Failed to start watching for device changes: %s\n",
			strerror(status));
	}

	// visit all disks in the system and show their contents
	_ScanDrives();

	// If DeskBar isn't running, DriveSetup was probably started from Installer.
	// Make sure to show on all workspaces, so the window doesn't disappear when using the
	// workspace switch shortcut.
	// Also make it not minimizable, because without DeskBar, there is no way to restore a
	// minimized window.
	if (!be_roster->IsRunning(kDeskbarSignature)) {
		SetFlags(Flags() | B_NOT_MINIMIZABLE);
		SetWorkspaces(B_ALL_WORKSPACES);
	}
}


MainWindow::~MainWindow()
{
	BDiskDeviceRoster().StopWatching(this);
	delete fCurrentDisk;
	delete fContextMenu;

	delete fRegisterFilePanel;
	delete fReadImageFilePanel;
	delete fWriteImageFilePanel;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MOUNT_ALL:
			_MountAll();
			break;
		case MSG_MOUNT:
			_Mount(fCurrentDisk, fCurrentPartitionID);
			break;
		case MSG_UNMOUNT:
			_Unmount(fCurrentDisk, fCurrentPartitionID);
			break;

		case MSG_FORMAT:
			printf("MSG_FORMAT\n");
			break;

		case MSG_CREATE:
			_Create(fCurrentDisk, fCurrentPartitionID);
			break;

		case MSG_INITIALIZE: {
			BString diskSystemName;
			if (message->FindString("disk system", &diskSystemName) != B_OK)
				break;
			_Initialize(fCurrentDisk, fCurrentPartitionID, diskSystemName);
			break;
		}

		case MSG_CHANGE:
			_ChangeParameters(fCurrentDisk, fCurrentPartitionID);
			break;

		case MSG_DELETE:
			_Delete(fCurrentDisk, fCurrentPartitionID);
			break;

		case MSG_EJECT:
			// TODO: completely untested, especially interesting
			// if partition list behaves when partitions disappear
			if (fCurrentDisk) {
				// TODO: only if no partitions are mounted anymore?
				fCurrentDisk->Eject(true);
				_ScanDrives();
			}
			break;
		case MSG_OPEN_DISKPROBE:
		{
			PartitionListRow* row = dynamic_cast<PartitionListRow*>(
				fListView->CurrentSelection());
			const char* args[] = { row->DevicePath(), NULL };

			be_roster->Launch("application/x-vnd.Haiku-DiskProbe", 1,
				(char**)args);

			break;
		}
		case MSG_SURFACE_TEST:
			printf("MSG_SURFACE_TEST\n");
			break;

		// TODO: this could probably be done better!
		case B_DEVICE_UPDATE:
			printf("B_DEVICE_UPDATE\n");
		case MSG_RESCAN:
			_ScanDrives();
			break;

		case MSG_PARTITION_ROW_SELECTED: {
			// selection of partitions via list view
			_AdaptToSelectedPartition();

			BPoint where;
			uint32 buttons;
			fListView->GetMouse(&where, &buttons);
			where.x += 2; // to prevent occasional select
			if (buttons & B_SECONDARY_MOUSE_BUTTON)
				fContextMenu->Go(fListView->ConvertToScreen(where),
					true, false, true);
			break;
		}
		case MSG_SELECTED_PARTITION_ID: {
			// selection of partitions via disk view
			partition_id id;
			if (message->FindInt32("partition_id", &id) == B_OK) {
				if (BRow* row = fListView->FindRow(id)) {
					fListView->DeselectAll();
					fListView->AddToSelection(row);
					_AdaptToSelectedPartition();
				}
			}
			BPoint where;
			uint32 buttons;
			fListView->GetMouse(&where, &buttons);
			where.x += 2; // to prevent occasional select
			if (buttons & B_SECONDARY_MOUSE_BUTTON)
				fContextMenu->Go(fListView->ConvertToScreen(where),
					true, false, true);
			break;
		}

		case MSG_REGISTER:
		{
			entry_ref entryRef;
			message->FindRef("refs", &entryRef);
			BEntry entry(&entryRef);
			BPath path;
			if (entry.GetPath(&path) == B_OK) {
				RegisterFileDiskDevice(path.Path());
				break;
			}

			if (fRegisterFilePanel == NULL) {
				fRegisterFilePanel = new(std::nothrow) BFilePanel(B_OPEN_PANEL,
					new BMessenger(this), NULL, B_FILE_NODE, false,
					new BMessage(MSG_REGISTER), NULL, true);
			}

			if (fRegisterFilePanel != NULL)
				fRegisterFilePanel->Show();
			break;
		}
		case MSG_UNREGISTER:
		{
			BPath path;
			fCurrentDisk->GetPath(&path);
			UnRegisterFileDiskDevice(path.Path());
			break;
		}
		case MSG_WRITE:
		{
			entry_ref entryRef;
			message->FindRef("refs", &entryRef);
			BEntry entry(&entryRef);
			BPath path;
			if (entry.GetPath(&path) == B_OK) {
				BMessage* msg = new BMessage();

				PartitionListRow* row = dynamic_cast<PartitionListRow*>(
					fListView->CurrentSelection());

				msg->AddString("source", path.Path());
				msg->AddString("target", row->DevicePath());
				msg->AddMessenger("messenger", BMessenger(this));

				thread_id write_thread = spawn_thread(WriteDiskImage,
					"WriteDiskImage", B_LOW_PRIORITY, (void*)msg);
				resume_thread(write_thread);
			} else {
				if (fWriteImageFilePanel == NULL) {
					fWriteImageFilePanel = new(std::nothrow) BFilePanel(
						B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE,
						false, new BMessage(MSG_WRITE), NULL, true);
				}
				if (fWriteImageFilePanel != NULL)
					fWriteImageFilePanel->Show();
			}
			break;
		}
		case MSG_SAVE:
		{
			PartitionListRow* row = dynamic_cast<PartitionListRow*>(
				fListView->CurrentSelection());

			if (fReadImageFilePanel == NULL) {
				fReadImageFilePanel = new(std::nothrow) BFilePanel(B_SAVE_PANEL,
					new BMessenger(this), NULL, B_FILE_NODE, false, NULL, NULL,
					true);
			}

			if (fReadImageFilePanel != NULL) {
				BStringField* field = (BStringField*)row->GetField(2);
				fReadImageFilePanel->SetSaveText(field->String());
				fReadImageFilePanel->Show();
			}
			break;
		}
		case B_SAVE_REQUESTED:
		{
			BMessage* msg = new BMessage();

			entry_ref entryRef;
			message->FindRef("directory", &entryRef);
			BEntry entry(&entryRef);
			BPath path;
			entry.GetPath(&path);
			msg->AddString("targetfolder", path.Path());
			const char* name;
			message->FindString("name", &name);
			path.Append(name);

			PartitionListRow* row = dynamic_cast<PartitionListRow*>(
				fListView->CurrentSelection());

			msg->AddString("source", row->DevicePath());
			msg->AddString("target", path.Path());
			msg->AddInt32("partitionID", row->ID());
			msg->AddMessenger("messenger", BMessenger(this));

			thread_id save_thread = spawn_thread(SaveDiskImage, "SaveDiskImage",
				B_LOW_PRIORITY, (void*)msg);
			resume_thread(save_thread);

			break;
		}
		case MSG_COMPLETE:
		{
			const char* status = "";

			message->FindString("status", &status);

			fNotification->SetTitle(BString(B_TRANSLATE("Disk image")));
			fNotification->SetContent(BString(status));
			fNotification->Send();

			break;
		}

		case MSG_UPDATE_ZOOM_LIMITS:
			_UpdateWindowZoomLimits();
			break;

		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			int32 i = 0;

			while (message->FindRef("refs", i++, &ref) == B_OK) {
				BEntry entry(&ref, true);
				BPath path(&entry);
				BNode node(&entry);

				if (node.IsFile()) {
					if (entry.GetPath(&path) == B_OK)
						RegisterFileDiskDevice(path.Path());
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	// TODO: ask about any unsaved changes
	be_app->PostMessage(B_QUIT_REQUESTED);
	Hide();
	return false;
}


// #pragma mark -


status_t
MainWindow::StoreSettings(BMessage* archive) const
{
	if (archive->ReplaceRect("window frame", Frame()) < B_OK)
		archive->AddRect("window frame", Frame());

	BMessage columnSettings;
	fListView->SaveState(&columnSettings);
	if (archive->ReplaceMessage("column settings", &columnSettings) < B_OK)
		archive->AddMessage("column settings", &columnSettings);

	return B_OK;
}


status_t
MainWindow::RestoreSettings(BMessage* archive)
{
	BRect frame;
	if (archive->FindRect("window frame", &frame) == B_OK) {
		BScreen screen(this);
		if (frame.Intersects(screen.Frame())) {
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(), frame.Height());
		}
	}

	BMessage columnSettings;
	if (archive->FindMessage("column settings", &columnSettings) == B_OK)
		fListView->LoadState(&columnSettings);

	return B_OK;
}


void
MainWindow::ApplyDefaultSettings()
{
	if (!Lock())
		return;

	fListView->ResizeAllColumnsToPreferred();

	// Adjust window size for convenience
	BScreen screen(this);
	float windowWidth = Frame().Width();
	float windowHeight = Frame().Height();

	float enlargeWidthBy = fListView->PreferredSize().width
		- fListView->Bounds().Width();
	float enlargeHeightBy = fListView->PreferredSize().height
		- fListView->Bounds().Height();

	if (enlargeWidthBy > 0.0f)
		windowWidth += enlargeWidthBy;
	if (enlargeHeightBy > 0.0f)
		windowHeight += enlargeHeightBy;

	if (windowWidth > screen.Frame().Width() - 20.0f)
		windowWidth = screen.Frame().Width() - 20.0f;
	if (windowHeight > screen.Frame().Height() - 20.0f)
		windowHeight = screen.Frame().Height() - 20.0f;

	ResizeTo(windowWidth, windowHeight);
	CenterOnScreen();

	Unlock();
}


static void
FileErrorAlert(const char* text, const char* fileName, alert_type type)
{
	BString message;
	message.SetToFormat(text, fileName);
	BAlert* alert = new BAlert(B_TRANSLATE("DriveSetup error"), message.String(),
		B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_WIDEST, type);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}


status_t
MainWindow::RegisterFileDiskDevice(const char* fileName)
{
	BString message;

	struct stat st;
	if (lstat(fileName, &st) != 0) {
		status_t error = errno;

		FileErrorAlert(B_TRANSLATE("Failed to get information about %s"),
			fileName, B_STOP_ALERT);
		return error;
	}

	if (!S_ISREG(st.st_mode)) {
		FileErrorAlert(B_TRANSLATE("%s is not a regular file."), fileName,
			B_STOP_ALERT);
		return B_BAD_VALUE;
	}

	// register the file
	BDiskDeviceRoster roster;
	partition_id id = roster.RegisterFileDevice(fileName);
	if (id < 0) {
		FileErrorAlert(B_TRANSLATE("Failed to register file disk device: %s"),
			fileName, B_STOP_ALERT);
		return id;
	}

	fNotification->SetTitle(BString(B_TRANSLATE("Disk image")));
	fNotification->SetContent(BString(
		B_TRANSLATE("Disk image has been added.")));
	fNotification->Send();

	return B_OK;
}


status_t
MainWindow::UnRegisterFileDiskDevice(const char* fileNameOrID)
{
	BString message;

	// try to parse the parameter as ID
	char* numberEnd;
	partition_id id = strtol(fileNameOrID, &numberEnd, 0);
	BDiskDeviceRoster roster;
	if (id >= 0 && numberEnd != fileNameOrID && *numberEnd == '\0') {
		BDiskDevice device;
		if (roster.GetDeviceWithID(id, &device) == B_OK && device.IsFile()) {
			status_t error = roster.UnregisterFileDevice(id);
			if (error != B_OK) {
				FileErrorAlert(B_TRANSLATE(
					"Failed to unregister file disk device."), NULL,
					B_STOP_ALERT);
				return error;
			}

			fNotification->SetTitle(BString(B_TRANSLATE("Disk image")));
			fNotification->SetContent(BString(
				B_TRANSLATE("Disk image has been removed.")));
			fNotification->Send();

			return B_OK;
		} else {
			FileErrorAlert(B_TRANSLATE(
				"No file disk device found with the given ID, trying file %s"),
				fileNameOrID, B_WARNING_ALERT);
		}
	}

	// the parameter must be a file name -- stat() it
	struct stat st;
	if (lstat(fileNameOrID, &st) != 0) {
		status_t error = errno;
		FileErrorAlert(B_TRANSLATE("Failed to get information about %s"),
			fileNameOrID, B_STOP_ALERT);
		return error;
	}

	// remember the volume and node ID, so we can identify the file
	// NOTE: There's a race condition -- we would need to open the file and
	// keep it open to avoid it.
	dev_t volumeID = st.st_dev;
	ino_t nodeID = st.st_ino;

	// iterate through all file disk devices and try to find a match
	BDiskDevice device;
	while (roster.GetNextDevice(&device) == B_OK) {
		if (!device.IsFile())
			continue;

		// get file path and stat it, same for the device path
		BPath path;
		if ((device.GetFilePath(&path) == B_OK && lstat(path.Path(), &st) == 0
				&& volumeID == st.st_dev && nodeID == st.st_ino)
			|| (device.GetPath(&path) == B_OK && lstat(path.Path(), &st) == 0
				&& volumeID == st.st_dev && nodeID == st.st_ino)) {
			status_t error = roster.UnregisterFileDevice(device.ID());
			if (error != B_OK) {
				FileErrorAlert(B_TRANSLATE(
					"Failed to unregister file disk device %s"), fileNameOrID,
					B_STOP_ALERT);
				return error;
			}

			fNotification->SetTitle(BString(B_TRANSLATE("Disk image")));
			fNotification->SetContent(BString(
				B_TRANSLATE("Disk image has been removed.")));
			fNotification->Send();

			return B_OK;
		}
	}

	FileErrorAlert(B_TRANSLATE("%s does not refer to a file disk device."),
		fileNameOrID, B_WARNING_ALERT);
	return B_BAD_VALUE;
}


int32
MainWindow::SaveDiskImage(void* data)
{
	BMessage* msg = (BMessage*)data;
	const char* sourcepath;
	const char* targetpath;
	const char* targetfolder;
	int32 partitionID;
	BMessenger messenger;

	msg->FindString("source", &sourcepath);
	msg->FindString("target", &targetpath);
	msg->FindString("targetfolder", &targetfolder);
	msg->FindInt32("partitionID", &partitionID);
	msg->FindMessenger("messenger", &messenger);

	BFile source(sourcepath, B_READ_ONLY);
	BFile target(targetpath, B_READ_WRITE | B_CREATE_FILE);

	if (source.InitCheck() != B_OK) {
		FileErrorAlert(B_TRANSLATE("Cannot init source volume."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	if (target.InitCheck() != B_OK) {
		FileErrorAlert(B_TRANSLATE("Cannot init target file."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	BDirectory* targetdir = new BDirectory(targetfolder);
	node_ref node;
	targetdir->GetNodeRef(&node);
	BVolume volume(node.device);

	if (volume.InitCheck()) {
		FileErrorAlert(B_TRANSLATE("Cannot init target volume."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	if (volume.IsReadOnly()) {
		FileErrorAlert(B_TRANSLATE("Target volume is read only."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	BPartition *partition;
	BDiskDevice device;

	BDiskDeviceRoster().GetPartitionWithID(partitionID, &device, &partition);

	if (volume.FreeBytes() < partition->Size()) {
		FileErrorAlert(B_TRANSLATE("There is not enough free space on target "
			"device."), NULL, B_STOP_ALERT);
		return -1;
	}

	size_t bufsize = 1024*1024;
	off_t sourcesize, targetsize;
	ssize_t targetbytes = 0;

	source.GetSize(&sourcesize);

	char* buffer = (char*)malloc(bufsize);
	while (true) {
		ssize_t bytes = source.Read(buffer, bufsize);

		if (bytes > 0) {
			target.Write(buffer, (size_t)bytes);
			target.GetSize(&targetsize);

			targetbytes += bytes;
		} else
			break;
	}
	free(buffer);

	BMessage message(MSG_COMPLETE);
	message.AddString("status",
		B_TRANSLATE("Disk image successfully created."));
	messenger.SendMessage(&message);

	return 0;
}


int32
MainWindow::WriteDiskImage(void* data)
{
	BMessage* msg = (BMessage*)data;
	const char* sourcepath;
	const char* targetpath;
	BMessenger messenger;

	msg->FindString("source", &sourcepath);
	msg->FindString("target", &targetpath);
	msg->FindMessenger("messenger", &messenger);

	BFile source(sourcepath, B_READ_ONLY);
	BFile target(targetpath, B_READ_WRITE);

	if (target.InitCheck() != B_OK) {
		FileErrorAlert(B_TRANSLATE("Cannot init target partition."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	BPartition *partition;
	BDiskDevice device;

	BDiskDeviceRoster().GetPartitionForPath(targetpath, &device, &partition);

	if (partition->IsReadOnly()) {
		FileErrorAlert(B_TRANSLATE("Target partition is read only."), NULL,
			B_STOP_ALERT);
		return -1;
	}

	off_t size;
	source.GetSize(&size);

	if (partition->Size() < size) {
		FileErrorAlert(B_TRANSLATE("The target partition is smaller than the "
			"image file."), NULL, B_STOP_ALERT);
		return -1;
	}

	size_t bufsize = 1024 * 1024;
	off_t sourcesize;
	ssize_t targetbytes = 0;

	source.GetSize(&sourcesize);

	char* buffer = (char*)malloc(bufsize);
	while (true) {
		ssize_t bytes = source.Read(buffer, bufsize);

		if (bytes > 0) {
			target.Write(buffer, (size_t)bytes);

			targetbytes += bytes;
		} else
			break;
	}
	free(buffer);

	BMessage message(MSG_COMPLETE);
	message.AddString("status",
		B_TRANSLATE("Disk image successfully written to the target."));
	messenger.SendMessage(&message);

	return 0;
}


// #pragma mark -


void
MainWindow::_ScanDrives()
{
	fSpaceIDMap.Clear();
	int32 diskCount = 0;
	ListPopulatorVisitor driveVisitor(fListView, diskCount, fSpaceIDMap);
	fDiskDeviceRoster.VisitEachPartition(&driveVisitor);
	fDiskView->SetDiskCount(diskCount);

	// restore selection
	PartitionListRow* previousSelection
		= fListView->FindRow(fCurrentPartitionID);
	if (previousSelection) {
		fListView->SetFocusRow(previousSelection, true);
		_UpdateMenus(fCurrentDisk, fCurrentPartitionID,
			previousSelection->ParentID());
		fDiskView->ForceUpdate();
	} else {
		_UpdateMenus(NULL, -1, -1);
	}

	PostMessage(MSG_UPDATE_ZOOM_LIMITS);
}


// #pragma mark -


void
MainWindow::_AdaptToSelectedPartition()
{
	partition_id diskID = -1;
	partition_id partitionID = -1;
	partition_id parentID = -1;

	BRow* _selectedRow = fListView->CurrentSelection();
	if (_selectedRow) {
		// go up to top level row
		BRow* _topLevelRow = _selectedRow;
		BRow* parent = NULL;
		while (fListView->FindParent(_topLevelRow, &parent, NULL))
			_topLevelRow = parent;

		PartitionListRow* topLevelRow
			= dynamic_cast<PartitionListRow*>(_topLevelRow);
		PartitionListRow* selectedRow
			= dynamic_cast<PartitionListRow*>(_selectedRow);

		if (topLevelRow)
			diskID = topLevelRow->ID();
		if (selectedRow) {
			partitionID = selectedRow->ID();
			parentID = selectedRow->ParentID();
		}
	}

	_SetToDiskAndPartition(diskID, partitionID, parentID);
}


void
MainWindow::_SetToDiskAndPartition(partition_id disk, partition_id partition,
	partition_id parent)
{
//printf("MainWindow::_SetToDiskAndPartition(disk: %ld, partition: %ld, "
//	"parent: %ld)\n", disk, partition, parent);

	BDiskDevice* oldDisk = NULL;
	if (!fCurrentDisk || fCurrentDisk->ID() != disk) {
		oldDisk = fCurrentDisk;
		fCurrentDisk = NULL;
		if (disk >= 0) {
			BDiskDevice* newDisk = new BDiskDevice();
			status_t status = newDisk->SetTo(disk);
			if (status != B_OK) {
				printf("error switching disks: %s\n", strerror(status));
				delete newDisk;
			} else
				fCurrentDisk = newDisk;
		}
	}

	fCurrentPartitionID = partition;

	fDiskView->SetDisk(fCurrentDisk, fCurrentPartitionID);
	_UpdateMenus(fCurrentDisk, fCurrentPartitionID, parent);

	delete oldDisk;
}


void
MainWindow::_UpdateMenus(BDiskDevice* disk,
	partition_id selectedPartition, partition_id parentID)
{
	while (BMenuItem* item = fFormatMenu->RemoveItem((int32)0))
		delete item;
	while (BMenuItem* item = fDiskInitMenu->RemoveItem((int32)0))
		delete item;
	while (BMenuItem* item = fFormatContextMenuItem->RemoveItem((int32)0))
		delete item;

	fCreateMenuItem->SetEnabled(false);
	fUnmountMenuItem->SetEnabled(false);
	fDiskInitMenu->SetEnabled(false);
	fFormatMenu->SetEnabled(false);

	fCreateContextMenuItem->SetEnabled(false);
	fUnmountContextMenuItem->SetEnabled(false);
	fFormatContextMenuItem->SetEnabled(false);

	if (disk == NULL) {
		fWipeMenuItem->SetEnabled(false);
		fEjectMenuItem->SetEnabled(false);
		fSurfaceTestMenuItem->SetEnabled(false);
		fUnRegisterMenuItem->SetEnabled(false);
		fSaveMenuItem->SetEnabled(false);
		fWriteMenuItem->SetEnabled(false);
		fOpenDiskProbeMenuItem->SetEnabled(false);
		fOpenDiskProbeContextMenuItem->SetEnabled(false);
	} else {
//		fWipeMenuItem->SetEnabled(true);
		fWipeMenuItem->SetEnabled(false);
		fEjectMenuItem->SetEnabled(disk->IsRemovableMedia());
//		fSurfaceTestMenuItem->SetEnabled(true);
		fSurfaceTestMenuItem->SetEnabled(false);

		fUnRegisterMenuItem->SetEnabled(disk->IsFile());

		fSaveMenuItem->SetEnabled(true);
		fWriteMenuItem->SetEnabled(!disk->IsReadOnly());

		// Create menu and items
		BPartition* parentPartition = NULL;
		if (selectedPartition <= -2) {
			// a partitionable space item is selected
			parentPartition = disk->FindDescendant(parentID);
		}

		if (parentPartition && parentPartition->ContainsPartitioningSystem()) {
			fCreateMenuItem->SetEnabled(true);
			fCreateContextMenuItem->SetEnabled(true);
		}
		bool prepared = disk->PrepareModifications() == B_OK;

		fFormatMenu->SetEnabled(prepared);
		fDeleteMenuItem->SetEnabled(prepared);
		fChangeMenuItem->SetEnabled(prepared);

		fFormatContextMenuItem->SetEnabled(prepared);
		fDeleteContextMenuItem->SetEnabled(prepared);
		fChangeContextMenuItem->SetEnabled(prepared);

		BPartition* partition = disk->FindDescendant(selectedPartition);

		BDiskSystem diskSystem;
		fDiskDeviceRoster.RewindDiskSystems();
		while (fDiskDeviceRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
			if (!diskSystem.SupportsInitializing())
				continue;

			BMessage* message = new BMessage(MSG_INITIALIZE);
			message->AddInt32("parent id", parentID);
			message->AddString("disk system", diskSystem.PrettyName());

			BString label = diskSystem.PrettyName();
			label << B_UTF8_ELLIPSIS;
			BMenuItem* item = new BMenuItem(label.String(), message);

			// TODO: Very unintuitive that we have to use PrettyName (vs Name)
			item->SetEnabled(partition != NULL
				&& partition->CanInitialize(diskSystem.PrettyName()));

			if (disk->ID() == selectedPartition
				&& !diskSystem.IsFileSystem()) {
				// Disk is selected, and DiskSystem is a partition map
				fDiskInitMenu->AddItem(item);
			} else if (diskSystem.IsFileSystem()) {
				// Otherwise a filesystem
				fFormatMenu->AddItem(item);

				// Context menu
				BMessage* message = new BMessage(MSG_INITIALIZE);
				message->AddInt32("parent id", parentID);
				message->AddString("disk system", diskSystem.PrettyName());
				BMenuItem* popUpItem = new BMenuItem(label.String(), message);
				popUpItem->SetEnabled(partition != NULL
					&& partition->CanInitialize(diskSystem.PrettyName()));
				fFormatContextMenuItem->AddItem(popUpItem);
				fFormatContextMenuItem->SetTargetForItems(this);
			}
		}

		// Mount items
		if (partition != NULL) {
			bool writable = !partition->IsReadOnly()
				&& partition->Device()->HasMedia();
			bool notMountedAndWritable = !partition->IsMounted() && writable;

			fFormatMenu->SetEnabled(writable && fFormatMenu->CountItems() > 0);

			fDiskInitMenu->SetEnabled(notMountedAndWritable
				&& partition->IsDevice()
				&& fDiskInitMenu->CountItems() > 0);

			fChangeMenuItem->SetEnabled(writable
				&& partition->CanEditParameters());
			fChangeContextMenuItem->SetEnabled(writable
				&& partition->CanEditParameters());

			fDeleteMenuItem->SetEnabled(notMountedAndWritable
				&& !partition->IsDevice());
			fDeleteContextMenuItem->SetEnabled(notMountedAndWritable
				&& !partition->IsDevice());

			fMountMenuItem->SetEnabled(!partition->IsMounted());
			fMountContextMenuItem->SetEnabled(!partition->IsMounted());

			fFormatContextMenuItem->SetEnabled(notMountedAndWritable
				&& fFormatContextMenuItem->CountItems() > 0);

			bool unMountable = false;
			if (partition->IsMounted()) {
				// see if this partition is the boot volume
				BVolume volume;
				BVolume bootVolume;
				if (BVolumeRoster().GetBootVolume(&bootVolume) == B_OK
					&& partition->GetVolume(&volume) == B_OK) {
					unMountable = volume != bootVolume;
				} else
					unMountable = true;
			}
			fUnmountMenuItem->SetEnabled(unMountable);
			fUnmountContextMenuItem->SetEnabled(unMountable);
		} else {
			fDeleteMenuItem->SetEnabled(false);
			fChangeMenuItem->SetEnabled(false);
			fMountMenuItem->SetEnabled(false);
			fFormatMenu->SetEnabled(false);
			fDiskInitMenu->SetEnabled(false);

			fDeleteContextMenuItem->SetEnabled(false);
			fChangeContextMenuItem->SetEnabled(false);
			fMountContextMenuItem->SetEnabled(false);
			fFormatContextMenuItem->SetEnabled(false);
		}

		if (prepared)
			disk->CancelModifications();

		fOpenDiskProbeMenuItem->SetEnabled(true);
		fOpenDiskProbeContextMenuItem->SetEnabled(true);

		fMountAllMenuItem->SetEnabled(true);
	}
	if (selectedPartition < 0) {
		fDeleteMenuItem->SetEnabled(false);
		fChangeMenuItem->SetEnabled(false);
		fMountMenuItem->SetEnabled(false);

		fDeleteContextMenuItem->SetEnabled(false);
		fChangeContextMenuItem->SetEnabled(false);
		fMountContextMenuItem->SetEnabled(false);
	}
}


void
MainWindow::_DisplayPartitionError(BString _message,
	const BPartition* partition, status_t error) const
{
	char message[1024];

	if (partition && _message.FindFirst("%s") >= 0) {
		BString name;
		name << "\"" << partition->ContentName() << "\"";
		snprintf(message, sizeof(message), _message.String(), name.String());
	} else {
		_message.ReplaceAll("%s", "");
		strlcpy(message, _message.String(), sizeof(message));
	}

	if (error < B_OK) {
		BString helper = message;
		const char* errorString
			= B_TRANSLATE_COMMENT("Error: ", "in any error alert");
		snprintf(message, sizeof(message), "%s\n\n%s%s", helper.String(),
			errorString, strerror(error));
	}

	BAlert* alert = new BAlert("error", message, B_TRANSLATE("OK"), NULL, NULL,
		B_WIDTH_FROM_WIDEST, error < B_OK ? B_STOP_ALERT : B_INFO_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}


// #pragma mark -


void
MainWindow::_Mount(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError(B_TRANSLATE("You need to select a partition "
			"entry from the list."));
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError(B_TRANSLATE("Unable to find the selected "
			"partition by ID."));
		return;
	}

	if (!partition->IsMounted()) {
		status_t status = partition->Mount();
		if (status != B_OK) {
			_DisplayPartitionError(B_TRANSLATE("Could not mount partition %s."),
				partition, status);
		} else {
			// successful mount, adapt to the changes
			_ScanDrives();
		}
	} else {
		_DisplayPartitionError(
			B_TRANSLATE("The partition %s is already mounted."), partition);
	}
}


void
MainWindow::_Unmount(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError(B_TRANSLATE("You need to select a partition "
			"entry from the list."));
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError(B_TRANSLATE("Unable to find the selected "
			"partition by ID."));
		return;
	}

	if (partition->IsMounted()) {
		BPath path;
		partition->GetMountPoint(&path);
		status_t status = partition->Unmount();
		if (status != B_OK) {
			BString message = B_TRANSLATE("Could not unmount partition");
			message << " \"" << partition->ContentName() << "\":\n\t"
				<< strerror(status) << "\n\n"
				<< B_TRANSLATE("Should unmounting be forced?\n\n"
				"Note: If an application is currently writing to the volume, "
				"unmounting it now might result in loss of data.\n");

			BAlert* alert = new BAlert(B_TRANSLATE("Force unmount"), message,
				B_TRANSLATE("Cancel"), B_TRANSLATE("Force unmount"), NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);

			if (alert->Go() == 1)
				status = partition->Unmount(B_FORCE_UNMOUNT);
			else
				return;
		}

		if (status != B_OK) {
			_DisplayPartitionError(
				B_TRANSLATE("Could not unmount partition %s."),
				partition, status);
		} else {
			if (dev_for_path(path.Path()) == dev_for_path("/"))
				rmdir(path.Path());
			// successful unmount, adapt to the changes
			_ScanDrives();
		}
	} else {
		_DisplayPartitionError(
			B_TRANSLATE("The partition %s is already unmounted."),
			partition);
	}
}


void
MainWindow::_MountAll()
{
	MountAllVisitor visitor;
	fDiskDeviceRoster.VisitEachPartition(&visitor);
}


// #pragma mark -


void
MainWindow::_Initialize(BDiskDevice* disk, partition_id selectedPartition,
	const BString& diskSystemName)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError(B_TRANSLATE("You need to select a partition "
			"entry from the list."));
		return;
	}

	if (disk->IsReadOnly()) {
		_DisplayPartitionError(B_TRANSLATE("The selected disk is read-only."));
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError(B_TRANSLATE("Unable to find the selected "
			"partition by ID."));
		return;
	}

	if (partition->IsMounted()) {
		if (partition->Unmount() != B_OK) {
			// Probably it's the system partition
			_DisplayPartitionError(
				B_TRANSLATE("The partition cannot be unmounted."));

			return;
		}
	}

	BDiskSystem diskSystem;
	fDiskDeviceRoster.RewindDiskSystems();
	bool found = false;
	while (fDiskDeviceRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.SupportsInitializing()) {
			if (diskSystemName == diskSystem.PrettyName()) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		_DisplayPartitionError(B_TRANSLATE("Disk system %s not found!"));
		return;
	}

	BString message;

	if (diskSystem.IsFileSystem()) {
		BString intelExtendedPartition = "Intel Extended Partition";
		if (disk->ID() == selectedPartition) {
			message = B_TRANSLATE("Are you sure you "
				"want to format a raw disk? (Most people initialize the disk "
				"with a partitioning system first) You will be asked "
				"again before changes are written to the disk.");
		} else if (partition->ContentName()
			&& strlen(partition->ContentName()) > 0) {
			message = B_TRANSLATE("Are you sure you "
				"want to format the partition \"%s\"? You will be asked "
				"again before changes are written to the disk.");
			message.ReplaceFirst("%s", partition->ContentName());
		} else if (partition->Type() == intelExtendedPartition) {
			message = B_TRANSLATE("Are you sure you "
				"want to format the Intel Extended Partition? Any "
				"subpartitions it contains will be overwritten if you "
				"continue. You will be asked again before changes are "
				"written to the disk.");
		} else {
			message = B_TRANSLATE("Are you sure you "
				"want to format the partition? You will be asked again "
				"before changes are written to the disk.");
		}
	} else {
		message = B_TRANSLATE("Are you sure you "
			"want to initialize the selected disk? All data will be lost. "
			"You will be asked again before changes are written to the "
			"disk.\n");
	}
	BAlert* alert = new BAlert("first notice", message.String(),
		B_TRANSLATE("Continue"), B_TRANSLATE("Cancel"), NULL,
		B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 choice = alert->Go();

	if (choice == 1)
		return;

	ModificationPreparer modificationPreparer(disk);
	status_t status = modificationPreparer.ModificationStatus();
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("There was an error preparing the "
			"disk for modifications."), NULL, status);
		return;
	}

	BString name;
	BString parameters;
	InitParametersPanel* panel = new InitParametersPanel(this, diskSystemName,
		partition);
	if (panel->Go(name, parameters) != B_OK)
		return;

	bool supportsName = diskSystem.SupportsContentName();
	BString validatedName(name);
	status = partition->ValidateInitialize(diskSystem.PrettyName(),
		supportsName ? &validatedName : NULL, parameters.String());
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Validation of the given "
			"initialization parameters failed."), partition, status);
		return;
	}

	BString previousName = partition->ContentName();

	status = partition->Initialize(diskSystem.PrettyName(),
		supportsName ? validatedName.String() : NULL, parameters.String());
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Initialization of the partition "
			"%s failed. (Nothing has been written to disk.)"), partition,
			status);
		return;
	}

	// Also set the partition name in the partition table if supported
	if (partition->CanSetName()
		&& partition->ValidateSetName(&validatedName) == B_OK) {
		partition->SetName(validatedName.String());
	}

	// everything looks fine, we are ready to actually write the changes
	// to disk

	// Warn the user one more time...
	if (previousName.Length() > 0) {
		if (partition->IsDevice()) {
			message = B_TRANSLATE("Are you sure you "
				"want to write the changes back to disk now?\n\n"
				"All data on the disk \"%s\" will be irretrievably lost if you "
				"do so!");
			message.ReplaceFirst("%s", previousName.String());
		} else {
			message = B_TRANSLATE("Are you sure you "
				"want to write the changes back to disk now?\n\n"
				"All data on the partition \"%s\" will be irretrievably lost if you "
				"do so!");
			message.ReplaceFirst("%s", previousName.String());
		}
	} else {
		if (partition->IsDevice()) {
			message = B_TRANSLATE("Are you sure you "
				"want to write the changes back to disk now?\n\n"
				"All data on the selected disk will be irretrievably lost if "
				"you do so!");
		} else {
			message = B_TRANSLATE("Are you sure you "
				"want to write the changes back to disk now?\n\n"
				"All data on the selected partition will be irretrievably lost "
				"if you do so!");
		}
	}
	alert = new BAlert("final notice", message.String(),
		B_TRANSLATE("Write changes"), B_TRANSLATE("Cancel"), NULL,
		B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	choice = alert->Go();

	if (choice == 1)
		return;

	// commit
	status = modificationPreparer.CommitModifications();

	// The partition pointer is toast now! Use the partition ID to
	// retrieve it again.
	partition = disk->FindDescendant(selectedPartition);

	if (status == B_OK) {
		if (diskSystem.IsFileSystem()) {
			_DisplayPartitionError(B_TRANSLATE("The partition %s has been "
				"successfully formatted.\n"), partition);
		} else {
			_DisplayPartitionError(B_TRANSLATE("The disk has been "
				"successfully initialized.\n"), partition);
		}
	} else {
		if (diskSystem.IsFileSystem()) {
			_DisplayPartitionError(B_TRANSLATE("Failed to format the "
				"partition %s!\n"), partition, status);
		} else {
			_DisplayPartitionError(B_TRANSLATE("Failed to initialize the "
				"disk %s!\n"), partition, status);
		}
	}

	_ScanDrives();
}


void
MainWindow::_Create(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition > -2) {
		_DisplayPartitionError(B_TRANSLATE("The currently selected partition "
			"is not empty."));
		return;
	}

	if (disk->IsReadOnly()) {
		_DisplayPartitionError(B_TRANSLATE("The selected disk is read-only."));
		return;
	}

	PartitionListRow* currentSelection = dynamic_cast<PartitionListRow*>(
		fListView->CurrentSelection());
	if (!currentSelection) {
		_DisplayPartitionError(B_TRANSLATE("There was an error acquiring the "
			"partition row."));
		return;
	}

	BPartition* parent = disk->FindDescendant(currentSelection->ParentID());
	if (!parent) {
		_DisplayPartitionError(B_TRANSLATE("The currently selected partition "
			"does not have a parent partition."));
		return;
	}

	if (!parent->ContainsPartitioningSystem()) {
		_DisplayPartitionError(B_TRANSLATE("The selected partition does not "
			"contain a partitioning system."));
		return;
	}

	ModificationPreparer modificationPreparer(disk);
	status_t status = modificationPreparer.ModificationStatus();
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("There was an error preparing the "
			"disk for modifications."), NULL, status);
		return;
	}

	// get partitioning info
	BPartitioningInfo partitioningInfo;
	status_t error = parent->GetPartitioningInfo(&partitioningInfo);
	if (error != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Could not acquire partitioning "
			"information."));
		return;
	}

	int32 spacesCount = partitioningInfo.CountPartitionableSpaces();
	if (spacesCount == 0) {
		_DisplayPartitionError(B_TRANSLATE("There's no space on the partition "
			"where a child partition could be created."));
		return;
	}

	BString name, type, parameters;
	off_t offset = currentSelection->Offset();
	off_t size = currentSelection->Size();

	CreateParametersPanel* panel = new CreateParametersPanel(this, parent,
		offset, size);
	status = panel->Go(offset, size, name, type, parameters);
	if (status != B_OK) {
		if (status != B_CANCELED) {
			_DisplayPartitionError(B_TRANSLATE("The panel could not return "
				"successfully."), NULL, status);
		}
		return;
	}

	status = parent->ValidateCreateChild(&offset, &size, type.String(),
		&name, parameters.String());

	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Validation of the given creation "
			"parameters failed."), NULL, status);
		return;
	}

	// Warn the user one more time...
	BAlert* alert = new BAlert("final notice", B_TRANSLATE("Are you sure you "
		"want to write the changes back to disk now?\n\n"
		"All data on the partition will be irretrievably lost if you do "
		"so!"), B_TRANSLATE("Write changes"), B_TRANSLATE("Cancel"), NULL,
		B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 choice = alert->Go();

	if (choice == 1)
		return;

	status = parent->CreateChild(offset, size, type.String(), name.String(),
		parameters.String());

	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Creation of the partition has "
			"failed."), NULL, status);
		return;
	}

	// commit
	status = modificationPreparer.CommitModifications();

	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Failed to create the "
			"partition. No changes have been written to disk."), NULL, status);
		return;
	}

	// The disk layout has changed, update disk information
	bool updated;
	status = disk->Update(&updated);

	_ScanDrives();
	fDiskView->ForceUpdate();
}


void
MainWindow::_Delete(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError(B_TRANSLATE("You need to select a partition "
			"entry from the list."));
		return;
	}

	if (disk->IsReadOnly()) {
		_DisplayPartitionError(B_TRANSLATE("The selected disk is read-only."));
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError(B_TRANSLATE("Unable to find the selected "
			"partition by ID."));
		return;
	}

	BPartition* parent = partition->Parent();
	if (!parent) {
		_DisplayPartitionError(B_TRANSLATE("The currently selected partition "
			"does not have a parent partition."));
		return;
	}

	ModificationPreparer modificationPreparer(disk);
	status_t status = modificationPreparer.ModificationStatus();
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("There was an error preparing the "
			"disk for modifications."), NULL, status);
		return;
	}

	if (!parent->CanDeleteChild(partition->Index())) {
		_DisplayPartitionError(
			B_TRANSLATE("Cannot delete the selected partition."));
		return;
	}

	// Warn the user one more time...
	BAlert* alert = new BAlert("final notice", B_TRANSLATE("Are you sure you "
		"want to delete the selected partition?\n\n"
		"All data on the partition will be irretrievably lost if you "
		"do so!"), B_TRANSLATE("Delete partition"), B_TRANSLATE("Cancel"), NULL,
		B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 choice = alert->Go();

	if (choice == 1)
		return;

	status = parent->DeleteChild(partition->Index());
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Could not delete the selected "
			"partition."), NULL, status);
		return;
	}

	status = modificationPreparer.CommitModifications();

	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Failed to delete the partition. "
			"No changes have been written to disk."), NULL, status);
		return;
	}

	_ScanDrives();
	fDiskView->ForceUpdate();
}


void
MainWindow::_ChangeParameters(BDiskDevice* disk, partition_id selectedPartition)
{
	if (disk == NULL || selectedPartition < 0) {
		_DisplayPartitionError(B_TRANSLATE("You need to select a partition "
			"entry from the list."));
		return;
	}

	if (disk->IsReadOnly()) {
		_DisplayPartitionError(B_TRANSLATE("The selected disk is read-only."));
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (partition == NULL) {
		_DisplayPartitionError(B_TRANSLATE("Unable to find the selected "
			"partition by ID."));
		return;
	}

	ModificationPreparer modificationPreparer(disk);
	status_t status = modificationPreparer.ModificationStatus();
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("There was an error preparing the "
			"disk for modifications."), NULL, status);
		return;
	}

	ChangeParametersPanel* panel = new ChangeParametersPanel(this, partition);

	BString name, type, parameters;
	status = panel->Go(name, type, parameters);
	if (status != B_OK) {
		if (status != B_CANCELED) {
			_DisplayPartitionError(B_TRANSLATE("The panel experienced a "
				"problem!"), NULL, status);
		}
		return;
	}

	if (partition->CanSetType())
		status = partition->ValidateSetType(type.String());
	if (status == B_OK && partition->CanSetName())
		status = partition->ValidateSetName(&name);
	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Validation of the given parameters "
			"failed."));
		return;
	}

	// Warn the user one more time...
	BAlert* alert = new BAlert("final notice", B_TRANSLATE("Are you sure you "
		"want to change parameters of the selected partition?\n\n"
		"The partition may no longer be recognized by other operating systems "
		"anymore!"), B_TRANSLATE("Change parameters"), B_TRANSLATE("Cancel"),
		NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	alert->SetShortcut(1, B_ESCAPE);
	int32 choice = alert->Go();

	if (choice == 1)
		return;

	if (partition->CanSetType())
		status = partition->SetType(type.String());
	if (status == B_OK && partition->CanSetName())
		status = partition->SetName(name.String());
	if (status == B_OK && partition->CanEditParameters())
		status = partition->SetParameters(parameters.String());

	if (status != B_OK) {
		_DisplayPartitionError(
			B_TRANSLATE("Could not change the parameters of the selected "
				"partition."), NULL, status);
		return;
	}

	status = modificationPreparer.CommitModifications();

	if (status != B_OK) {
		_DisplayPartitionError(B_TRANSLATE("Failed to change the parameters "
			"of the partition. No changes have been written to disk."), NULL,
			status);
		return;
	}

	_ScanDrives();
	fDiskView->ForceUpdate();
}


float
MainWindow::_ColumnListViewHeight(BColumnListView* list, BRow* currentRow)
{
	float height = 0;
	int32 rows = list->CountRows(currentRow);
	for (int32 i = 0; i < rows; i++) {
		BRow* row = list->RowAt(i, currentRow);
		height += row->Height() + 1;
		if (row->IsExpanded() && list->CountRows(row) > 0)
			height += _ColumnListViewHeight(list, row);
	}
	return height;
}


void
MainWindow::_UpdateWindowZoomLimits()
{
	float maxHeight = 0;
	int32 numColumns = fListView->CountColumns();
	BRow* parentRow = fListView->RowAt(0, NULL);
	BColumn* column = NULL;

	maxHeight += _ColumnListViewHeight(fListView, NULL);

	float maxWidth = fListView->LatchWidth();
	for (int32 i = 0; i < numColumns; i++) {
		column = fListView->ColumnAt(i);
		maxWidth += column->Width();
	}

	maxHeight += B_H_SCROLL_BAR_HEIGHT;
	maxHeight += 1.5 * parentRow->Height();	// the label row
	maxHeight += fDiskView->Bounds().Height();
	maxHeight += fMenuBar->Bounds().Height();
	maxWidth += 1.5 * B_V_SCROLL_BAR_WIDTH;	// scroll bar & borders

	SetZoomLimits(maxWidth, maxHeight);
}

