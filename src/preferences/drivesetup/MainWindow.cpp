/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "MainWindow.h"
#include "PartitionList.h"

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


class DriveVisitor : public BDiskDeviceVisitor {
public:
	DriveVisitor(PartitionListView* list);

	virtual	bool Visit(BDiskDevice* device);
	virtual	bool Visit(BPartition* partition, int32 level);

private:
	PartitionListView* fPartitionList;
};

const char*
SizeAsString(off_t size, char *string)
{
	double kb = size / 1024.0;
	if (kb < 1.0) {
		sprintf(string, "%Ld B", size);
		return string;
	}
	float mb = kb / 1024.0;
	if (mb < 1.0) {
		sprintf(string, "%3.1f KB", kb);
		return string;
	}
	float gb = mb / 1024.0;
	if (gb < 1.0) {
		sprintf(string, "%3.1f MB", mb);
		return string;
	}
	float tb = gb / 1024.0;
	if (tb < 1.0) {
		sprintf(string, "%3.1f GB", gb);
		return string;
	}
	sprintf(string, "%.1f TB", tb);
	return string;
}


static void
dump_partition_info(BPartition* partition)
{
	char size[1024];
	printf("\tOffset(): %Ld\n", partition->Offset());
	printf("\tSize(): %s\n", SizeAsString(partition->Size(),size));
	printf("\tContentSize(): %s\n", SizeAsString(partition->ContentSize(), size));
	printf("\tBlockSize(): %ld\n", partition->BlockSize());
	printf("\tIndex(): %ld\n", partition->Index());
	printf("\tStatus(): %ld\n\n", partition->Status());
	printf("\tContainsFileSystem(): %s\n", partition->ContainsFileSystem() ? "true" : "false");
	printf("\tContainsPartitioningSystem(): %s\n\n", partition->ContainsPartitioningSystem() ? "true" : "false");
	printf("\tIsDevice(): %s\n", partition->IsDevice() ? "true" : "false");
	printf("\tIsReadOnly(): %s\n", partition->IsReadOnly() ? "true" : "false");
	printf("\tIsMounted(): %s\n", partition->IsMounted() ? "true" : "false");
	printf("\tIsBusy(): %s\n\n", partition->IsBusy() ? "true" : "false");
	printf("\tFlags(): %lx\n\n", partition->Flags());
	printf("\tName(): %s\n", partition->Name());
	printf("\tContentName(): %s\n", partition->ContentName());
	printf("\tType(): %s\n", partition->Type());
	printf("\tContentType(): %s\n", partition->ContentType());
	printf("\tID(): %lx\n\n", partition->ID());
}


enum {
	MSG_MOUNT_ALL				= 'mnta',
	MSG_MOUNT					= 'mnts',
	MSG_UNMOUNT					= 'unmt',
	MSG_FORMAT					= 'frmt',
	MSG_CREATE_PRIMARY			= 'crpr',
	MSG_CREATE_EXTENDED			= 'crex',
	MSG_CREATE_LOGICAL			= 'crlg',
	MSG_INITIALIZE				= 'init',
	MSG_EJECT					= 'ejct',
	MSG_SURFACE_TEST			= 'sfct',
	MSG_RESCAN					= 'rscn',
};


MainWindow::MainWindow(BRect frame)
	: BWindow(frame, "DriveSetup", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BMenuBar* rootMenu = new BMenuBar(Bounds(), "root menu");

	// Disk menu
	BMenu* menu = new BMenu("Disk");
		menu->AddItem(new BMenuItem("Format", new BMessage(MSG_FORMAT), 'F'));
		menu->AddItem(new BMenuItem("Eject", new BMessage(MSG_EJECT), 'E'));
		menu->AddItem(new BMenuItem("Surface Test",
			new BMessage(MSG_SURFACE_TEST), 'T'));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Rescan", new BMessage(MSG_RESCAN)));
	rootMenu->AddItem(menu);

	// Parition menu
	menu = new BMenu("Partition");
		BMenu* createMenu = new BMenu("Create");
		menu->AddItem(createMenu);
		fInitMenu = new BMenu("Initialize");
		menu->AddItem(fInitMenu);
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Mount", new BMessage(MSG_MOUNT), 'M'));
		menu->AddItem(new BMenuItem("Unmount", new BMessage(MSG_UNMOUNT), 'U'));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem("Mount All",
			new BMessage(MSG_MOUNT_ALL), 'M', B_SHIFT_KEY));
	rootMenu->AddItem(menu);

	AddChild(rootMenu);

	createMenu->AddItem(new BMenuItem("Primary",
		new BMessage(MSG_CREATE_PRIMARY)));
	createMenu->AddItem(new BMenuItem("Extended",
		new BMessage(MSG_CREATE_EXTENDED)));
	createMenu->AddItem(new BMenuItem("Logical",
		new BMessage(MSG_CREATE_LOGICAL)));

	BRect r(Bounds());
	r.top = rootMenu->Frame().bottom + 1;
	fListView = new PartitionListView(r);
	AddChild(fListView);

	// Populate the Initialiaze menu with the available file systems
	_ScanFileSystems();

	// Visit all disks in the system and show their contents
	_ScanDrives();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MOUNT_ALL:
			printf("MSG_MOUNT_ALL\n");
			break;
		case MSG_MOUNT:
			printf("MSG_MOUNT\n");
			break;
		case MSG_UNMOUNT:
			printf("MSG_UNMOUNT\n");
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

		case MSG_INITIALIZE:
			printf("MSG_INITIALIZE\n");
			break;

		case MSG_EJECT:
			printf("MSG_EJECT\n");
			break;
		case MSG_SURFACE_TEST:
			printf("MSG_SURFACE_TEST\n");
			break;
		case MSG_RESCAN:
			_ScanDrives();
			break;
			
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
	// TODO: store column list settings
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
	// TODO: restore column list settings
	return B_OK;
}


// #pragma mark -


void
MainWindow::_ScanDrives()
{
	DriveVisitor driveVisitor(fListView);
	BPartition* partition = NULL;
	BDiskDevice device;
	fDDRoster.VisitEachPartition(&driveVisitor, &device, &partition);
}


void
MainWindow::_ScanFileSystems()
{
	while (BMenuItem* item = fInitMenu->RemoveItem(0L))
		delete item;

	BDiskSystem diskSystem;
	fDDRoster.RewindDiskSystems();
	while(fDDRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.IsFileSystem() && diskSystem.SupportsInitializing()) {
			BMessage* message = new BMessage(MSG_INITIALIZE);
			message->AddString("format", diskSystem.Name());
			BString label = diskSystem.PrettyName();
			label << B_UTF8_ELLIPSIS;
			fInitMenu->AddItem(new BMenuItem(label.String(), message));
		}
	}
}


// #pragma mark - DriveVisitor


DriveVisitor::DriveVisitor(PartitionListView* list)
	: fPartitionList(list)
{
	// start with an empty list
	int32 rows = fPartitionList->CountRows();
	for (int32 i = rows - 1; i >= 0; i--) {
		BRow* row = fPartitionList->RowAt(i);
		fPartitionList->RemoveRow(row);
		delete row;
	}
}


bool
DriveVisitor::Visit(BDiskDevice* device)
{
	fPartitionList->AddPartition(device);

	printf("Visit(%p)\n", device);
	dump_partition_info(device);

	return false; // Don't stop yet!
}


bool
DriveVisitor::Visit(BPartition* partition, int32 level)
{
	fPartitionList->AddPartition(partition);

	printf("Visit(%p, %ld)\n", partition, level);
	dump_partition_info(partition);

	return false; // Don't stop yet!
}
