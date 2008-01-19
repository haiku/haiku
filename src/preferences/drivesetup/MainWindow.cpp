/*
 * Copyright 2002-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "MainWindow.h"
#include "DiskView.h"
#include "InitParamsPanel.h"
#include "PartitionList.h"
#include "Support.h"

#include <Alert.h>
#include <Application.h>

#include <DiskDeviceVisitor.h>
#include <DiskSystem.h>
#include <DiskDevice.h>
#include <Partition.h>
#include <Path.h>

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <Menu.h>
#include <Screen.h>


class ListPopulatorVisitor : public BDiskDeviceVisitor {
public:
	ListPopulatorVisitor(PartitionListView* list, int32& diskCount)
		: fPartitionList(list)
		, fDiskCount(diskCount)
	{
		fDiskCount = 0;
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
		fPartitionList->AddPartition(device);
		return false; // Don't stop yet!
	}

	virtual bool Visit(BPartition* partition, int32 level)
	{
		fPartitionList->AddPartition(partition);
		return false; // Don't stop yet!
	}

private:
	PartitionListView*	fPartitionList;
	int32&				fDiskCount;
};


class MountAllVisitor : public BDiskDeviceVisitor {
public:
	MountAllVisitor()
	{
	}

	virtual bool Visit(BDiskDevice* device)
	{
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


enum {
	MSG_MOUNT_ALL				= 'mnta',
	MSG_MOUNT					= 'mnts',
	MSG_UNMOUNT					= 'unmt',
	MSG_FORMAT					= 'frmt',
	MSG_CREATE_PRIMARY			= 'crpr',
	MSG_CREATE_EXTENDED			= 'crex',
	MSG_CREATE_LOGICAL			= 'crlg',
	MSG_INITIALIZE				= 'init',
	MSG_DELETE					= 'delt',
	MSG_EJECT					= 'ejct',
	MSG_SURFACE_TEST			= 'sfct',
	MSG_RESCAN					= 'rscn',

	MSG_PARTITION_ROW_SELECTED	= 'prsl',
};


MainWindow::MainWindow(BRect frame)
	: BWindow(frame, "DriveSetup", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
	, fCurrentDisk(NULL)
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "root menu");

	// create all the menu items
	fFormatMI = new BMenuItem("Format (not implemented)", new BMessage(MSG_FORMAT));
	fEjectMI = new BMenuItem("Eject", new BMessage(MSG_EJECT), 'E');
	fSurfaceTestMI = new BMenuItem("Surface Test (not implemented)",
		new BMessage(MSG_SURFACE_TEST));
	fRescanMI = new BMenuItem("Rescan", new BMessage(MSG_RESCAN));

	fCreatePrimaryMI = new BMenuItem("Primary",
		new BMessage(MSG_CREATE_PRIMARY));
	fCreateExtendedMI = new BMenuItem("Extended",
		new BMessage(MSG_CREATE_EXTENDED));
	fCreateLogicalMI = new BMenuItem("Logical",
		new BMessage(MSG_CREATE_LOGICAL));

	fDeleteMI = new BMenuItem("Delete (not implemented)",
		new BMessage(MSG_DELETE));
fDeleteMI->SetEnabled(false);

	fMountMI = new BMenuItem("Mount", new BMessage(MSG_MOUNT), 'M');
	fUnmountMI = new BMenuItem("Unmount", new BMessage(MSG_UNMOUNT), 'U');
	fMountAllMI = new BMenuItem("Mount All",
		new BMessage(MSG_MOUNT_ALL), 'M', B_SHIFT_KEY);

	// Disk menu
	fDiskMenu = new BMenu("Disk");
		fDiskMenu->AddItem(fFormatMI);
		fDiskMenu->AddItem(fEjectMI);
		fDiskMenu->AddItem(fSurfaceTestMI);

		fDiskMenu->AddSeparatorItem();

		fDiskMenu->AddItem(fRescanMI);
	menuBar->AddItem(fDiskMenu);

	// Parition menu
	fPartitionMenu = new BMenu("Partition");
		fCreateMenu = new BMenu("Create (not implemented)");
			fCreateMenu->AddItem(fCreatePrimaryMI);
			fCreateMenu->AddItem(fCreateExtendedMI);
			fCreateMenu->AddItem(fCreateLogicalMI);
		fPartitionMenu->AddItem(fCreateMenu);

		fInitMenu = new BMenu("Initialize");
		fPartitionMenu->AddItem(fInitMenu);

		fPartitionMenu->AddItem(fDeleteMI);

		fPartitionMenu->AddSeparatorItem();

		fPartitionMenu->AddItem(fMountMI);
		fPartitionMenu->AddItem(fUnmountMI);

		fPartitionMenu->AddSeparatorItem();

		fPartitionMenu->AddItem(fMountAllMI);
	menuBar->AddItem(fPartitionMenu);

	AddChild(menuBar);


	// add DiskView
	BRect r(Bounds());
	r.top = menuBar->Frame().bottom + 1;
	r.bottom = floorf(r.top + r.Height() * 0.33);
	fDiskView = new DiskView(r, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(fDiskView);

	// add PartitionListView
	r.top = r.bottom + 2;
	r.bottom = Bounds().bottom;
	r.InsetBy(-1, -1);
	fListView = new PartitionListView(r, B_FOLLOW_ALL);
	AddChild(fListView);

	// configure PartitionListView
	fListView->SetSelectionMessage(new BMessage(MSG_PARTITION_ROW_SELECTED));
	fListView->SetTarget(this);

	// populate the Initialiaze menu with the available file systems
	_ScanFileSystems();

	// visit all disks in the system and show their contents
	_ScanDrives();

	_EnabledDisableMenuItems(NULL, -1);
}


MainWindow::~MainWindow()
{
	delete fCurrentDisk;
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

		case MSG_CREATE_PRIMARY:
			printf("MSG_CREATE_PRIMARY\n");
			break;
		case MSG_CREATE_EXTENDED:
			printf("MSG_CREATE_EXTENDED\n");
			break;
		case MSG_CREATE_LOGICAL:
			printf("MSG_CREATE_LOGICAL\n");
			break;

		case MSG_INITIALIZE: {
			BString diskSystemName;
			if (message->FindString("format", &diskSystemName) != B_OK)
				break;
			_Initialize(fCurrentDisk, fCurrentPartitionID, diskSystemName);
			break;
		}

		case MSG_DELETE:
			printf("MSG_DELETE\n");
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
		case MSG_SURFACE_TEST:
			printf("MSG_SURFACE_TEST\n");
			break;
		case MSG_RESCAN:
			_ScanDrives();
			break;

		case MSG_PARTITION_ROW_SELECTED:
			// selection of partitions via list view
			_AdaptToSelectedPartition();
			break;
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


// #pragma mark -


void
MainWindow::_ScanDrives()
{
	int32 diskCount = 0;
	ListPopulatorVisitor driveVisitor(fListView, diskCount);
	fDDRoster.VisitEachPartition(&driveVisitor);
	fDiskView->SetDiskCount(diskCount);
}


void
MainWindow::_ScanFileSystems()
{
	while (BMenuItem* item = fInitMenu->RemoveItem(0L))
		delete item;

	BDiskSystem diskSystem;
	fDDRoster.RewindDiskSystems();
	while (fDDRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.IsFileSystem() && diskSystem.SupportsInitializing()) {
			BMessage* message = new BMessage(MSG_INITIALIZE);
			message->AddString("format", diskSystem.PrettyName());
			BString label = diskSystem.PrettyName();
			label << B_UTF8_ELLIPSIS;
			fInitMenu->AddItem(new BMenuItem(label.String(), message));
		}
	}
}


// #pragma mark -


void
MainWindow::_AdaptToSelectedPartition()
{
	partition_id disk = -1;
	partition_id partition = -1;

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
			disk = topLevelRow->ID();
		if (selectedRow)
			partition = selectedRow->ID();
	}

	_SetToDiskAndPartition(disk, partition);
}


void
MainWindow::_SetToDiskAndPartition(partition_id disk, partition_id partition)
{
	if (!fCurrentDisk || fCurrentDisk->ID() != disk) {
		delete fCurrentDisk;
		fCurrentDisk = NULL;
		if (disk >= 0) {
			BDiskDevice* newDisk = new BDiskDevice();
			status_t ret = newDisk->SetTo(disk);
			if (ret < B_OK) {
				printf("error switching disks: %s\n", strerror(ret));
				delete newDisk;
			} else
				fCurrentDisk = newDisk;
		}
	}

	fCurrentPartitionID = partition;

	fDiskView->SetDisk(fCurrentDisk, fCurrentPartitionID);
	_EnabledDisableMenuItems(fCurrentDisk, fCurrentPartitionID);
}


void
MainWindow::_EnabledDisableMenuItems(BDiskDevice* disk,
	partition_id selectedPartition)
{
	if (!disk) {
		fFormatMI->SetEnabled(false);
		fEjectMI->SetEnabled(false);
		fSurfaceTestMI->SetEnabled(false);

		fPartitionMenu->SetEnabled(false);
	} else {
//		fFormatMI->SetEnabled(true);
fFormatMI->SetEnabled(false);
		fEjectMI->SetEnabled(disk->IsRemovableMedia());
//		fSurfaceTestMI->SetEnabled(true);
fSurfaceTestMI->SetEnabled(false);

		fPartitionMenu->SetEnabled(true);
		// fCreateMenu->SetEnabled(/*empty space selected*/);
fCreateMenu->SetEnabled(false);

		BPartition* partition = disk->FindDescendant(selectedPartition);
		if (partition) {
			fInitMenu->SetEnabled(!partition->IsMounted());
//			fDeleteMI->SetEnabled(!partition->IsMounted());
fDeleteMI->SetEnabled(false);
			fMountMI->SetEnabled(!partition->IsMounted());
			fUnmountMI->SetEnabled(partition->IsMounted());
		} else {
			fInitMenu->SetEnabled(false);
			fDeleteMI->SetEnabled(false);
			fMountMI->SetEnabled(false);
			fUnmountMI->SetEnabled(false);
		}
		fMountAllMI->SetEnabled(true);
	}
}


void
MainWindow::_DisplayPartitionError(BString _message,
	const BPartition* partition, status_t error) const
{
	char message[1024];
	
	if (partition && _message.FindFirst("%s") >= 0) {
		BString name;
		name << " \"" << partition->ContentName() << "\"";
		sprintf(message, _message.String(), name.String());
	} else {
		_message.ReplaceAll("%s", "");
		sprintf(message, _message.String());
	}

	if (error < B_OK) {
		BString helper = message;
		sprintf(message, "%s\n\nError: %s", helper.String(), strerror(error));
	}

	BAlert* alert = new BAlert("error", message, "Ok", NULL, NULL,
		B_WIDTH_FROM_WIDEST, error < B_OK ? B_STOP_ALERT : B_INFO_ALERT);
	alert->Go(NULL);
}


// #pragma mark -


void
MainWindow::_Mount(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError("You need to select a partition "
			"entry from the list.");
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError("Unable to find the selected partition by id.");
		return;
	}

	if (!partition->IsMounted()) {
		status_t ret = partition->Mount();
		if (ret < B_OK) {
			_DisplayPartitionError("Could not mount partition %s.",
				partition, ret);
		} else {
			// successful mount, adapt to the changes
			_ScanDrives();
		}
	} else {
		_DisplayPartitionError("The partition %s is already mounted.",
			partition);
	}
}


void
MainWindow::_Unmount(BDiskDevice* disk, partition_id selectedPartition)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError("You need to select a partition "
			"entry from the list.");
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError("Unable to find the selected partition by id.");
		return;
	}

	if (partition->IsMounted()) {
		status_t ret = partition->Unmount();
		if (ret < B_OK) {
			_DisplayPartitionError("Could not unmount partition %s.",
				partition, ret);
		} else {
			// successful unmount, adapt to the changes
			_ScanDrives();
		}
	} else {
		_DisplayPartitionError("The partition %s is already unmounted.",
			partition);
	}
}


void
MainWindow::_MountAll()
{
	MountAllVisitor visitor;
	fDDRoster.VisitEachPartition(&visitor);
}


// #pragma mark -


void
MainWindow::_Initialize(BDiskDevice* disk, partition_id selectedPartition,
	const BString& diskSystemName)
{
	if (!disk || selectedPartition < 0) {
		_DisplayPartitionError("You need to select a partition "
			"entry from the list.");
		return;
	}

	if (disk->IsReadOnly()) {
		_DisplayPartitionError("The selected disk is read-only.");
		return;
	}

	BPartition* partition = disk->FindDescendant(selectedPartition);
	if (!partition) {
		_DisplayPartitionError("Unable to find the selected partition by id.");
		return;
	}

	if (partition->IsMounted()) {
		_DisplayPartitionError("The partition %s is currently mounted.");
		// TODO: option to unmount and continue on success to unmount
		return;
	}

	BString message("Are you sure you want to initialize the partition ");
	message << "\"" << partition->ContentName();
	message << "\"? After entering the initialization parameters, ";
	message << "you can abort this operation right before writing ";
	message << "changes back to the disk.";
	BAlert* alert = new BAlert("first notice", message.String(),
		"Continue", "Cancel", NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	int32 choice = alert->Go();

	if (choice == 1)
		return;

	BDiskSystem diskSystem;
	fDDRoster.RewindDiskSystems();
	bool found = false;
	while (fDDRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.IsFileSystem() && diskSystem.SupportsInitializing()) {
			if (diskSystemName == diskSystem.PrettyName()) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		BString message("Disk system \"");
		message << diskSystemName << "\" not found!";
		_DisplayPartitionError(message);
		return;
	}


	// allow BFS only, since our parameter string
	// construction only handles BFS at the moment
	if (diskSystemName != "Be File System") {
		_DisplayPartitionError("Don't know how to construct parameters "
			"for this file system.");
		return;
	}

	status_t ret = disk->PrepareModifications();
	if (ret != B_OK) {
		_DisplayPartitionError("There was an error preparing the "
			"disk for modifications.", NULL, ret);
		return;
	}

	// TODO: use partition initialization editor
	// (partition->GetInitializationParameterEditor())

	BString name = "Haiku";
	BString parameters;
	InitParamsPanel* panel = new InitParamsPanel(this);
	panel->Go(name, parameters);

	bool supportsName = diskSystem.SupportsContentName();
	BString validatedName(name);
	ret = partition->ValidateInitialize(diskSystem.PrettyName(),
		supportsName ? &validatedName : NULL, parameters.String());
	if (ret != B_OK) {
		_DisplayPartitionError("Validation of the given initialization "
			"parameters failed.", partition, ret);
		return;
	}

	BString previousName = partition->ContentName();

	ret = partition->Initialize(diskSystem.PrettyName(),
		supportsName ? name.String() : NULL, parameters.String());
	if (ret != B_OK) {
		_DisplayPartitionError("Initialization of the partition %s "
			"failed. (Nothing has been written to disk.)", partition, ret);
		return;
	}

	// everything looks fine, we are ready to actually write the changes
	// to disk

	message = "Are you sure you want to write the changes back to "
		"disk now?\n\n";
	message << "All data on the partition \"" << previousName;
	message << "\" will be irrevertably lost if you do so!";
	alert = new BAlert("final notice", message.String(),
		"Write Changes", "Cancel", NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	choice = alert->Go();

	if (choice == 1)
		return;

	// commit
	ret = disk->CommitModifications();
	if (ret == B_OK) {
		_DisplayPartitionError("The partition %s has been successfully "
			"initialized.\n", partition);
	} else {
		_DisplayPartitionError("Failed to initialize the partition "
			"%s!\n", partition, ret);
	}
}

