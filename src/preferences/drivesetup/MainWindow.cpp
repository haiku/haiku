/*! \file MainWindow.cpp
 *  \brief Code for the MainWindow class.
 *  
 *  Displays the main window, the essence of the app.
 *
*/

#include "MainWindow.h"

#include "PosSettings.h"

#include <interface/MenuItem.h>
#include <interface/MenuBar.h>
#include <interface/Menu.h>

#include <interface/ColumnListView.h>
#include <interface/ColumnTypes.h>

#include <app/Application.h>

#include <storage/DiskDeviceVisitor.h>
#include <storage/DiskDevice.h>
#include <storage/Partition.h>
#include <storage/Path.h>

class DriveVisitor : public BDiskDeviceVisitor 
{
public:
	DriveVisitor(MainWindow* win);

	bool Visit(BDiskDevice *device);
	bool Visit(BPartition *partition, int32 level);
private:
	MainWindow* fDSWindow;
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

/**
 * Constructor.
 * @param frame The size to make the window.
 */	
MainWindow::MainWindow(BRect frame, PosSettings *Settings)
	: BWindow(frame, "DriveSetup", B_TITLED_WINDOW,  B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect bRect;
	BMenu *menu;
	BMenu *tmpMenu;
	BMenuItem *emptyItem;
	
	fSettings = Settings;
	
	bRect = Bounds();
	bRect.bottom = 18.0;
	
	BMenuBar* rootMenu = new BMenuBar(bRect, "root menu");
	//Setup Mount menu
	menu = new BMenu("Mount");
	menu->AddItem(new BMenuItem("Mount All Partitions", new BMessage(MOUNT_MOUNT_ALL_MSG), 'M'));
	menu->AddSeparatorItem();
	rootMenu->AddItem(menu);
	
	//Setup Unmount menu
	menu = new BMenu("Unmount");
	emptyItem = new BMenuItem("<empty>", NULL);
	emptyItem->SetEnabled(false);
	menu->AddItem(emptyItem);
	rootMenu->AddItem(menu);
	
	//Setup Setup menu
	menu = new BMenu("Setup");
		menu->AddItem(new BMenuItem("Format", new BMessage(SETUP_FORMAT_MSG), 'F'));
		tmpMenu = new BMenu("Partition");
		menu->AddItem(tmpMenu);
		tmpMenu = new BMenu("Initialize");
		//add menu for formats
		menu->AddItem(tmpMenu);
	rootMenu->AddItem(menu);
	
	//Setup Options menu
	menu = new BMenu("Options");
		menu->AddItem(new BMenuItem("Eject", new BMessage(OPTIONS_EJECT_MSG), 'E'));
		menu->AddItem(new BMenuItem("Surface Test", new BMessage(OPTIONS_SURFACE_TEST_MSG), 'T'));
	rootMenu->AddItem(menu);
	
	//Setup Rescan menu
	menu = new BMenu("Rescan");
		menu->AddItem(new BMenuItem("IDE", new BMessage(RESCAN_IDE_MSG)));
		menu->AddItem(new BMenuItem("SCSI", new BMessage(RESCAN_SCSI_MSG)));
	rootMenu->AddItem(menu);
		
	AddChild(rootMenu);

	BRect r(Bounds());
	r.top = rootMenu->Frame().bottom +1;
	fListView = new BColumnListView(r, "storagelist", B_FOLLOW_ALL, 0, B_PLAIN_BORDER, true);
	fListView->AddColumn(new BStringColumn("Device", 100, 500, 50, 0), 1);
	fListView->AddColumn(new BStringColumn("Map style", 100, 500, 50, 0), 2);
	fListView->AddColumn(new BStringColumn("Partition Type", 100, 500, 50, 0), 3);
	fListView->AddColumn(new BStringColumn("Filesystem", 100, 500, 50, 0), 4);
	fListView->AddColumn(new BStringColumn("Volume Name", 100, 500, 50, 0), 5);
	fListView->AddColumn(new BStringColumn("Mounted At", 100, 500, 50, 0), 6);
	fListView->AddColumn(new BStringColumn("Size", 100, 500, 50, 0), 7);
	AddChild(fListView);

	// Now visit all disks in the system and show their contents
	DriveVisitor driveVisitor(this);
	BPartition *partition = NULL;
	BDiskDevice device;
	fDDRoster.VisitEachPartition(&driveVisitor, &device, &partition);
}

/**
 * Handles messages.
 * @param message The message recieved by the window.
 */	
void MainWindow::MessageReceived(BMessage *message){

	switch(message->what){
	
		case MOUNT_MOUNT_ALL_MSG:
		
			printf("MOUNT_MOUNT_ALL_MSG\n");
			break;
			
		case MOUNT_MOUNT_SELECTED_MSG:
		
			printf("MOUNT_MOUNT_SELECTED_MSG\n");
			break;
			
		case UNMOUNT_UNMOUNT_SELECTED_MSG:
		
			printf("UNMOUNT_UNMOUNT_SELECTED_MSG\n");
			break;
		
		case SETUP_FORMAT_MSG:
		
			printf("SETUP_FORMAT_MSG\n");
			break;
			
		case SETUP_INITIALIZE_MSG:
		
			printf("SETUP_INITIALIZE_MSG\n");
			break;
			
		case OPTIONS_EJECT_MSG:
		
			printf("OPTIONS_EJECT_MSG\n");
			break;
			
		case OPTIONS_SURFACE_TEST_MSG:
		
			printf("OPTIONS_SURFACE_TEST_MSG\n");
			break;
			
		case RESCAN_IDE_MSG:
		
			printf("RESCAN_IDE_MSG\n");
			break;
			
		case RESCAN_SCSI_MSG:
		
			printf("RESCAN_SCSI_MSG\n");
			break;
			
		default:
		
			BWindow::MessageReceived(message);
			
	}//switch
	
}

/**
 * Quits and Saves settings.
 */	
bool 
MainWindow::QuitRequested()
{
	bool accepted = BWindow::QuitRequested();
	if (accepted) {
		fSettings->SetWindowPosition(Frame());
    	be_app->PostMessage(B_QUIT_REQUESTED);
	}
	
	return accepted;
}

static void
dump_partition_info(BPartition* partition)
{
	printf("\tOffset(): %Ld\n", partition->Offset());
	printf("\tSize(): %Ld\n", partition->Size());
	printf("\tContentSize(): %Ld\n", partition->ContentSize());
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
	printf("\tID(): %x\n\n", partition->ID());
}

void
MainWindow::AddDrive(BDiskDevice* device)
{
	printf("AddDrive(%p)\n", device);

	BPath path;
	if (device->GetPath(&path) == B_OK)  {
		BRow* row = new BRow();
		row->SetField(new BStringField(path.Path()), 1);
		
		printf("\tPath=%s\n", path.Path());

		if (device->ContainsPartitioningSystem()) {
			row->SetField(new BStringField(device->Type()), 3);
		}

		char size[1024];
		row->SetField(new BStringField(SizeAsString(device->Size(), size)), 7);
		
		fListView->AddRow(row);
	}

	dump_partition_info(device);
}

void
MainWindow::AddPartition(BPartition *partition, int32 level)
{
	printf("AddPartition(%p, %d)\n", partition, level);

	BPath path;
	if (partition->GetPath(&path) == B_OK)  {
		BRow* row = new BRow();
		row->SetField(new BStringField(path.Path()), 1);
		
		printf("\tPath=%s\n", path.Path());
		
		if (partition->ContainsFileSystem()) {
			row->SetField(new BStringField(partition->Type()), 3);	// Partition Type
			row->SetField(new BStringField(partition->ContentType()), 4); // Filesystem
			row->SetField(new BStringField(partition->ContentName()), 5);	// Volume Name
			
			if (partition->GetMountPoint(&path) == B_OK)
				row->SetField(new BStringField(path.Path()),  6);
		}
		
		char size[1024];
		row->SetField(new BStringField(SizeAsString(partition->Size(), size)), 7);
		
		fListView->AddRow(row);
	}

	dump_partition_info(partition);
}


DriveVisitor::DriveVisitor(MainWindow* win)
	: fDSWindow(win)
{
}

bool
DriveVisitor::Visit(BDiskDevice* device)
{
	if (fDSWindow != NULL)
		fDSWindow->AddDrive(device);

	return false; // Don't stop yet!
}

bool
DriveVisitor::Visit(BPartition* partition, int32 level)
{
	if (fDSWindow != NULL)
		fDSWindow->AddPartition(partition, level);

	return false; // Don't stop yet!
}
