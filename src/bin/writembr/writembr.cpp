/*
 * Copyright 2012 Aleksas Pantechovskis, <alexp.frl@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <string>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "MBR.h"


using namespace std;

const char* kUsageMessage = \
	"Usage: writembr [ device ] \n"
	"#\tRewrites the MBR for the specified device.\n"
	"#\tIf no device is specified, the boot device is used.\n"
	"#\t--help shows this usage message\n";


int
main(int argc, char** argv)
{
	if ((argc == 2 && strcmp(argv[1], "--help") == 0) || argc > 2) {
		cerr << kUsageMessage;
		return B_ERROR;
	}

	BPath device;

	if (argc == 2)
		// user specified device for rewriting
		device.SetTo(argv[1]);

	else if (argc == 1) {
		// no parameters specified, rewrite boot device
		BVolumeRoster volumeRoster;
		BVolume bootVolume;
		if (volumeRoster.GetBootVolume(&bootVolume) != B_OK) {
			cerr << "Can not find boot device" << endl;
			return B_ERROR;
		}

		BDiskDeviceRoster roster;
		BDiskDevice bootDevice;
		if(roster.FindPartitionByVolume(bootVolume, &bootDevice, NULL) != B_OK) {
			cerr << "Can not find boot device" << endl;
			return B_ERROR;
		}

		bootDevice.GetPath(&device);
	}


	if (strcmp(device.Leaf(), "raw") != 0) {
		cerr << device.Path() << " is not a raw device" << endl;
		return B_ERROR;
	}

	fstream fs;
	fs.open(device.Path(), fstream::in | fstream::out | fstream::binary);
	if (!fs.is_open()) {
		cerr << "Can't open " << device.Path() << endl;
		return B_ERROR;
	}

	unsigned char MBR[kMBRSize];
	fs.read((char*)MBR, kMBRSize);
	if (fs.fail() || fs.gcount() < kMBRSize ) {
		cerr << "Cannot read " << kMBRSize
			<< " bytes from " << device.Path() << endl;
		fs.close();
		return B_ERROR;
	}

	// update only the code area and the MBR signature
	memcpy(MBR, kMBR, 0x1be);
	MBR[0x1FE] = kMBR[0x1FE];
	MBR[0x1FF] = kMBR[0x1FF];

	cerr << "About to overwrite the MBR boot code on " << device.Path()
		<< "\nThis may disable any partition managers you have installed.\n"
		<< "Are you sure you want to continue?\nyes/[no]: ";

	string choice;
	getline(cin, choice, '\n');
	if (choice == "no" || choice == "" || choice != "yes") {
		cerr << "MBR was NOT written" << endl;
		fs.close();
		return B_ERROR;
	}
			
	cerr << "Rewriting MBR for " << device.Path() << endl;
		
	fs.seekg(0, ios::beg);
	fs.write((char*)MBR, kMBRSize);
	if (fs.fail()) {
		cerr << "Cannot write " << kMBRSize
			<< " bytes to " << device.Path() << endl;
		fs.close();
		return B_ERROR;
	}

	fs.close();

	cerr << "MBR was written OK" << endl;
	return B_OK;
}

