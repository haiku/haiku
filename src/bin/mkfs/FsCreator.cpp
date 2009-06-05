/*
 * Copyright 2008-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marco Minutoli, mminutoli@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include "FsCreator.h"

#include <iostream>

#include <DiskSystem.h>


class UnregisterFileDevice {
public:
	UnregisterFileDevice()
		:
		fID(-1)
	{
	}

	~UnregisterFileDevice()
	{
		if (fID >= 0) {
			BDiskDeviceRoster roster;
			roster.UnregisterFileDevice(fID);
		}
	}

	void SetTo(partition_id id)
	{
		fID = id;
	}

	void Detach()
	{
		fID = -1;
	}

private:
	partition_id	fID;
};


extern "C" const char* __progname;
static const char* kProgramName = __progname;


FsCreator::FsCreator(const char* path, const char* type, const char* volumeName,
		const char* fsOptions, bool quick, bool verbose)
	:
	fType(type),
	fPath(path),
	fVolumeName(volumeName),
	fFsOptions(fsOptions),
	fVerbose(verbose),
	fQuick(quick)
{
}


bool
FsCreator::Run()
{
	UnregisterFileDevice unregisterFileDevice;

	BDiskDeviceRoster roster;
	BPartition* partition;
	BDiskDevice device;

	status_t status = roster.GetPartitionForPath(fPath, &device,
		&partition);
	if (status != B_OK) {
		if (!strncmp(fPath, "/dev", 4)) {
			std::cerr << kProgramName << ": Failed to get disk device for path "
				<< fPath << ": " << strerror(status) << std::endl;
			return false;
		}

		// try to register file device

		partition_id id = roster.RegisterFileDevice(fPath);
		if (id < B_OK) {
			std::cerr << kProgramName << ": Could not register file device for "
				"path " << fPath << ": " << strerror(status) << std::endl;
			return false;
		}

		unregisterFileDevice.SetTo(id);

		status = roster.GetPartitionWithID(id, &device, &partition);
		if (!strncmp(fPath, "/dev", 4)) {
			std::cerr << kProgramName << ": Cannot find registered file device "
				"for path " << fPath << ": " << strerror(status)
				<< std::endl;
			return false;
		}
	}

	// check that the device is writable
	if (partition->IsReadOnly()) {
		std::cerr << kProgramName << ": Cannot initialize read-only device.\n";
		return false;
	}

	// check if the device is mounted
	if (partition->IsMounted()) {
		std::cerr << kProgramName << ": Cannot initialize mounted device.\n";
		return false;
	}

	BDiskSystem diskSystem;
	if (roster.GetDiskSystem(&diskSystem, fType) != B_OK) {
		std::cerr << kProgramName << ": " << fType
			<< " is an invalid or unsupported file system type.\n";
		return false;
	}

	// prepare the device for modifications
	status = device.PrepareModifications();
	if (status != B_OK) {
		std::cerr << kProgramName << ": A problem occurred preparing the "
			"device for the modifications\n";
		return false;
	}
	if (fVerbose)
		std::cout << "Preparing for modifications...\n\n";

	// validate parameters
	BString name(fVolumeName);
	if (partition->ValidateInitialize(diskSystem.PrettyName(),
			&name, fFsOptions) != B_OK) {
		std::cerr << kProgramName << ": Parameters validation failed. "
			"Check what you wrote\n";
		std::cerr << status;
		return false;
	}
	if (fVerbose)
		std::cout << "Parameters Validation...\n\n";
	if (name != fVolumeName) {
		std::cout << "Volume name was adjusted to "
			<< name.String() << std::endl;
	}

	// Initialize the partition
	status = partition->Initialize(diskSystem.PrettyName(), name.String(),
		fFsOptions);
	if (status != B_OK) {
		std::cerr << kProgramName << ": Initialization failed: "
			<< strerror(status) << std::endl;
		return false;
	}

	if (!fQuick) {
		std::cout << "\nAbout to initialize " << fPath << " with "
			<< diskSystem.PrettyName()
			<< "\nAre you sure you want to do this now?\n"
			<< "\nALL YOUR DATA in " << fPath << " will be lost forever.\n";

		BString reply;
		do {
			std::cout << "Continue (yes|[no])? ";
			reply = _ReadLine();
			if (reply == "")
				reply = "no"; // silence is dissence
		} while (reply != "yes" && reply != "no");

		if (reply != "yes")
			return true;
	}

	BString contentName = partition->ContentName();
		// CommitModifications() will invalidate our partition object

	status = device.CommitModifications();
	if (status == B_OK) {
		if (fVerbose) {
			std::cout << "Volume \"" << contentName.String()
				<< "\" has been initialized successfully!" << std::endl;
		}
	} else {
		std::cout << kProgramName << ": Initialization of \""
			<< contentName.String() << "\" failed: " << strerror(status)
			<< std::endl;
		return false;
	}

	// TODO: should we keep the file device around, or unregister it
	// after we're done? This could be an option, too (for now, we'll
	// just keep them if everything went well).
	unregisterFileDevice.Detach();
	return true;
}


inline BString
FsCreator::_ReadLine()
{
	char line[255];

	std::cin.getline(line, sizeof(line), '\n');

	return line;
}
