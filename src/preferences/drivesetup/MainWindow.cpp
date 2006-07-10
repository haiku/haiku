/*! \file MainWindow.cpp
 *  \brief Code for the MainWindow class.
 *  
 *  Displays the main window, the essence of the app.
 *
*/

#include "MainWindow.h"
#include "PartitionList.h"
#include "PosSettings.h"

#include <interface/MenuItem.h>
#include <interface/MenuBar.h>
#include <interface/Menu.h>

#include <interface/ColumnListView.h>
#include <interface/ColumnTypes.h>

#include <app/Application.h>

#include <storage/DiskDeviceVisitor.h>
#include <storage/DiskSystem.h>
#include <storage/DiskDevice.h>
#include <storage/Partition.h>
#include <storage/Path.h>

class DriveVisitor : public BDiskDeviceVisitor 
{
public:
	DriveVisitor(PartitionListView* list);

	bool Visit(BDiskDevice *device);
	bool Visit(BPartition *partition, int32 level);
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

#if DEBUG
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
#endif

/**
 * Constructor.
 * @param frame The size to make the window.
 */	
MainWindow::MainWindow(BRect frame, PosSettings *Settings)
	: BWindow(frame, "DriveSetup", B_DOCUMENT_WINDOW,  B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BMenu *partitionMenu, *initMenu;
	BMenu *menu;

	fSettings = Settings;
	
	BMenuBar* rootMenu = new BMenuBar(Bounds(), "root menu");
	//Setup Mount menu
	menu = new BMenu("Mount");
	menu->AddItem(new BMenuItem("Mount All Partitions", new BMessage(MOUNT_MOUNT_ALL_MSG), 'M'));
	menu->AddSeparatorItem();
	rootMenu->AddItem(menu);
	
	//Setup Unmount menu
	menu = new BMenu("Unmount");
	rootMenu->AddItem(menu);
	
	//Setup Setup menu
	menu = new BMenu("Setup");
		menu->AddItem(new BMenuItem("Format", new BMessage(SETUP_FORMAT_MSG), 'F'));
		partitionMenu = new BMenu("Partition");
		menu->AddItem(partitionMenu);
		initMenu = new BMenu("Initialize");
		//add menu for formats
		menu->AddItem(initMenu);
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
	fListView = new PartitionListView(r);
	AddChild(fListView);

	// Now update filesystem/partition menus with values
	BDiskSystem diskSystem;
	fDDRoster.RewindDiskSystems();
	while(fDDRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.IsPartitioningSystem()) {
			BMessage* msg = new BMessage(SETUP_PARTITION_SELECTED_MSG);
			msg->AddString("bdisksystem:name", diskSystem.Name());
			partitionMenu->AddItem(new BMenuItem(diskSystem.PrettyName(), msg));
		} else if (diskSystem.IsFileSystem()) {
			BMessage* msg = new BMessage(SETUP_INITIALIZE_MSG);
			msg->AddString("bdisksystem:name", diskSystem.Name());
			initMenu->AddItem(new BMenuItem(diskSystem.PrettyName(), msg));
		}
	}

	// Now visit all disks in the system and show their contents
	DriveVisitor driveVisitor(fListView);
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

DriveVisitor::DriveVisitor(PartitionListView* list)
	: fPartitionList(list)
{
}

bool
DriveVisitor::Visit(BDiskDevice* device)
{
	if (fPartitionList != NULL)
		fPartitionList->AddPartition(device);

DEBUG_ONLY(
	printf("Visit(%p)\n", device);
	dump_partition_info(device);
);

	return false; // Don't stop yet!
}

bool
DriveVisitor::Visit(BPartition* partition, int32 level)
{
	if (fPartitionList != NULL)
		fPartitionList->AddPartition(partition);

DEBUG_ONLY(
	printf("Visit(%p, %d)\n", partition, level);
	dump_partition_info(partition);
);

	return false; // Don't stop yet!
}
