/*! \file main.cpp
 *  \brief Code for the main class.
 *  
 *  This file contains the code for the main class.  This class sets up all
 *  of the initial conditions for the app.
 *
*/

#ifndef MAIN_WINDOW_H

	#include "MainWindow.h"
	
#endif
#ifndef MAIN_H

	#include "main.h"
	
#endif

/**
 * Main method.
 * 
 * Starts the whole thing.
 */
int main(int, char**){

	/**
	 * An instance of the application.
	 */
	DriveSetup dsApp;
	
	dsApp.Run();
	
	return(0);
	
}

/*
 * Constructor.
 * 
 * Provides a contstructor for the application.
 */
DriveSetup::DriveSetup()
			:BApplication("application/x-vnd.MSM-DriveSetupPrefPanel"){
	
	fSettings = new PosSettings();
    
	/*
	 * The main interface window.
	 */		
	MainWindow *Main;
	dev_info devs;
	partition_info partitions[4];
	
	Main = new MainWindow(fSettings->WindowPosition(), fSettings);
	
	//This section is to set up defaults that I can use for testing.
	//set up floppy drive
	partitions[0].type = 11;
	partitions[0].fs = "dos";
	partitions[0].name = "";
	partitions[0].mount_pt = "";
	partitions[0].size = 1.4;
	partitions[0].mounted = FALSE;
	devs.device = "/dev/disk/floppy";
	devs.map = 1;
	devs.numParts = 1;
	devs.parts = partitions;
	Main->AddDeviceInfo(devs);
	
	partitions[0].type = 11;
	partitions[0].fs = "dos";
	partitions[0].name = "winxp";
	partitions[0].mount_pt = "/winxp";
	partitions[0].size = 5600.0;
	partitions[0].mounted = TRUE;
	partitions[1].type = 12;
	partitions[1].fs = "dos";
	partitions[1].name = "windata";
	partitions[1].mount_pt = "/windata";
	partitions[1].size = 3600.0;
	partitions[1].mounted = TRUE;
	partitions[2].type = 235;
	partitions[2].fs = "Be File System";
	partitions[2].name = "BeOS";
	partitions[2].mount_pt = "/boot";
	partitions[2].size = 4900.0;
	partitions[2].mounted = TRUE;
	devs.device = "/dev/disk/ide/ata/0/master/0";
	devs.map = 1;
	devs.numParts = 3;
	devs.parts = partitions;
	Main->AddDeviceInfo(devs);
	
	partitions[0].type = 11;
	partitions[0].fs = "iso9660";
	partitions[0].name = "";
	partitions[0].mount_pt = "/OUTLAWSTAR_D1";
	partitions[0].size = 2200.0;
	partitions[0].mounted = TRUE;
	devs.device = "/dev/disk/ide/atapi/1/master/0";
	devs.map = 2;
	devs.numParts = 1;
	devs.parts = partitions;
	Main->AddDeviceInfo(devs);

	//End set up of defaults
	
	Main->Show();

}

DriveSetup::~DriveSetup()
{
		delete fSettings;
}

