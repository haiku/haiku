/*! \file main.cpp
 *  \brief Code for the main class.
 *  
 *  This file contains the code for the main class.  This class sets up all
 *  of the initial conditions for the app.
 *
*/

#include "MainWindow.h"
#include "main.h"
#include <iostream>
using namespace std;
#define MEGABITE 1048576
/**
 * Main method.
 * 
 * Starts the whole thing.
 */
int main(int, char**){

	/**
	 * An instance of the application.
	 */
	new VM_pref();
	be_app->Run();
	delete be_app;
	return 0;
}

/*
 * Constructor.
 * 
 * Provides a contstructor for the application.
 */
VM_pref::VM_pref()
	:BApplication("application/x-vnd.Haiku-MEM$") {
	
	// read current swap settings
	FILE *settingsFile = fopen("/boot/home/config/settings/kernel/drivers/virtual_memory", "r");
	char dummy[80];
    
	int physMem;	//The amount of physical memory in the machine.
	int currSwap;	//The current size of the swap file.
	int setSwap;	//The set size of the swap file.
	double minSwap;	//The minimum size the swap file can be.
	int maxSwap;	//The maximum size the swap file can be.

	const char *swap_file= "/boot/var/swap";
	BEntry swap(swap_file);
	off_t swapsize;
	swap.GetSize(&swapsize);
	currSwap = swapsize / MEGABITE;
	
	system_info info;
	get_system_info(&info);
	physMem = (info.max_pages * 4096) / MEGABITE; 
	
	float memcalc = (physMem +(int)(physMem/3.0));
	cout << memcalc << endl;
	modf(memcalc/128.0, &minSwap);
	cout << minSwap << endl;
	minSwap*= 128;

	if (settingsFile != NULL) {
		fscanf(settingsFile, "%s %s\n", dummy, dummy);
		fscanf(settingsFile, "%s %d\n", dummy, &setSwap);
		setSwap = setSwap / MEGABITE;
	} else {
		setSwap = (int)minSwap;
	}
	
	fclose(settingsFile);
	
	BVolume bootVol;
	BVolumeRoster *vol_rost = new BVolumeRoster();
	vol_rost->GetBootVolume(&bootVol);
	/* maxSwap is defined by the amount of free space on your boot
	 * volume, plus the current swap file size, minus an arbitrary
	 * amount of space, just so you don't fill up your drive completly.
	 */
	maxSwap = (bootVol.FreeBytes() / MEGABITE) + currSwap - 16;
	
	bool changeMsg = false;
	if (currSwap != setSwap) {
		currSwap = setSwap;
		changeMsg = true;
	}

	fSettings = new VMSettings();
	window = new MainWindow(fSettings->WindowPosition(), physMem, currSwap, minSwap, maxSwap, fSettings);
	if (changeMsg) {
		window->toggleChangedMessage(true);
	}

}


VM_pref::~VM_pref()
{
	delete fSettings;
}


void
VM_pref::ReadyToRun()
{
	window->Show();
}

