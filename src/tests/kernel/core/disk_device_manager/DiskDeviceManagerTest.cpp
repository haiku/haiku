//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include <KDiskDeviceManager.h>

// main
int
main()
{
	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager *manager = KDiskDeviceManager::Default();
	manager->InitialDeviceScan();
	return 0;
}

