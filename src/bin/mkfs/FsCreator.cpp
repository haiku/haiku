/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marco Minutoli, mminutoli@gmail.com
 */

#include "FsCreator.h"

#include <iostream>

#include <DiskSystem.h>


FsCreator::FsCreator(const char* devPath, BString& type,
	BString& volumeName, const char* fsOpt, bool verbose)
	:
	fType(type),
	fDevicePath(devPath),
	fVolumeName(volumeName),
	fFsOptions(fsOpt),
	fVerbose(verbose)
{
	BDiskDeviceRoster roster;
	status_t ret = roster.GetDeviceForPath(devPath, &fDevice);
	if (ret != B_OK) {
		std::cerr << "Error: Failed to get disk device for path "
				  << devPath << ": " << strerror(ret);
	}
}


bool
FsCreator::Run()
{
	// check that the device is writable
	if (fDevice.IsReadOnly()) {
		std::cerr << "Error: Can't Inizialize the device. "
			"It is read only.\n";
		return false;
	}

	// check if the device is mounted
	if (fDevice.IsMounted()) {
		std::cerr << "Error: The device has to be unmounted before.\n";
		return false;
	}

	BDiskSystem diskSystem;
	BDiskDeviceRoster dDRoster;
	bool found = false;
	while (dDRoster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (diskSystem.IsFileSystem() && diskSystem.SupportsInitializing()) {
			if (diskSystem.ShortName() == fType) {
				found = true;
				break;
			}
		}
	}

	if (!found) {
		std::cerr << "Error: " << fType.String()
				  << " is an invalid or unsupported file system type.\n";
		return false;
	}

	// prepare the device for modifications
	status_t ret = fDevice.PrepareModifications();
	if (ret != B_OK) {
		std::cerr << "Error: A problem occurred preparing the device for the"
			"modifications\n";
		return false;
	}
	if (fVerbose)
		std::cout << "Preparing for modifications...\n\n";

	// validate parameters
	BString name(fVolumeName);
	if (fDevice.ValidateInitialize(diskSystem.PrettyName(),
			&fVolumeName, fFsOptions) != B_OK) {
		std::cerr << "Error: Parameters validation failed. "
			"Check what you wrote\n";
		std::cerr << ret;
		return false;
	}
	if (fVerbose)
		std::cout << "Parameters Validation...\n\n";
	if (name != fVolumeName)
		std::cout << "Volume name was adjusted to "
				  << fVolumeName.String() << std::endl;

	// Initialize the partition
	ret = fDevice.Initialize(diskSystem.PrettyName(),
		fVolumeName.String(), fFsOptions);
	if (ret != B_OK) {
		std::cerr << "Initialization failed: " <<  strerror(ret) << std::endl;
		return false;
	}

	std::cout << "\nAre you sure you want to do this now?\n"
			  << "\nALL YOUR DATA in " << fDevicePath.String()
			  << " will be lost forever.\n";

	BString reply;
	do {
		std::cout << "Continue? [yes|no]: ";
		reply = _ReadLine();
		if (reply == "")
			reply = "no"; // silence is dissence
	} while (reply != "yes" && reply != "no");

	if (reply == "yes") {
		ret = fDevice.CommitModifications();
		if (ret == B_OK) {
			if (fVerbose) {
				std::cout << "Volume " << fDevice.ContentName()
						  << " has been initialized successfully!\n";
			}
		} else {
			std::cout << "Error: Initialization of " << fDevice.ContentName()
					  << " failed: " << strerror(ret) << std::endl;
			return false;
		}
	}
	return true;
}


inline BString
FsCreator::_ReadLine()
{
	char line[255];

	cin.getline(line, sizeof(line), '\n');

	return line;
}
