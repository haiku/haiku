/*! \file main.cpp
 *  \brief Code for the main class.
 *  
 *  This file contains the code for the main class.  This class sets up all
 *  of the initial conditions for the app.
 *
*/

#include "MainWindow.h"
#include "main.h"

/**
 * Main method.
 * 
 * Starts the whole thing.
 */
int main(int, char**){

	/**
	 * An instance of the application.
	 */
	VM_pref vmApp;
	
	vmApp.Run();
	
	return(0);
	
}

/*
 * Constructor.
 * 
 * Provides a contstructor for the application.
 */
VM_pref::VM_pref()
			:BApplication("application/x-vnd.MSM-VirtualMemoryPrefPanel"){
	
	FILE *settingsFile =
    	fopen("/boot/home/config/settings/kernel/drivers/virtual_memory", "r");
   	FILE *ptr;
    char dummy[80];
    BVolume bootVol;
    BVolumeRoster *vol_rost = new BVolumeRoster();
    fSettings = new VMSettings();
    
	/*
	 * The main interface window.
	 */		
	MainWindow *Main;
	
	/*
	 * The amount of physical memory in the machine.
	 */
	int physMem;
	
	/*
	 * The current size of the swap file.
	 */
	int currSwap;
	
	/*
	 * The set size of the swap file.
	 */
	int setSwap;
	
	/*
	 * The minimum size the swap file can be.
	 */
	int minSwap;
	
	/*
	 * The maximum size the swap file can be.
	 */
	int maxSwap;
	
	/*
	 * System info
	 */
	system_info info;
	
	/*
	 * Swap file
	 */
	const char *swap_file;
	
	bool changeMsg = false;
	
	swap_file = "/boot/var/swap";
	BEntry swap(swap_file);
	off_t swapsize;
	swap.GetSize(&swapsize);
	currSwap = swapsize / 1048576;
	
	get_system_info(&info);
	physMem = (info.max_pages * 4096) / 1048576; 
	
	minSwap = physMem + (int)(physMem / 3.0);
	
	if(settingsFile != NULL){
	
		fscanf(settingsFile, "%s %s\n", dummy, dummy);
		fscanf(settingsFile, "%s %d\n", dummy, &setSwap);
		setSwap = setSwap / 1048576;
	
	}//if
	else{
	
		setSwap = minSwap;
		
	}//else
	fclose(settingsFile);
	
	vol_rost->GetBootVolume(&bootVol);
	/* maxSwap is defined by the amount of free space on your boot
	 * volume, plus the current swap file size, minus an arbitrary
	 * amount of space, just so you don't fill up your drive completly.
	 */
	maxSwap = (bootVol.FreeBytes() / 1048576) + currSwap - 16;
	
	if(currSwap != setSwap){
	
		currSwap = setSwap;
		changeMsg = true;
				
	}//if
	
	Main = new MainWindow(fSettings->WindowPosition(), physMem, currSwap, minSwap, maxSwap, fSettings);
		
	if(changeMsg){
	
		Main->toggleChangedMessage(true);
		
	}//if
	
	Main->Show();

}


VM_pref::~VM_pref()
{//VM_pref::~VM_pref
		delete fSettings;
}//VM_pref::~VM_pref

