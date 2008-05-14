/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marco Minutoli, mminutoli@gmail.com
 */
#ifndef _FSCREATOR_H_
#define _FSCREATOR_H_

#include <String.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>

class FsCreator {
public:
	FsCreator(const char* devPath, BString& type, BString& volumeName,
		const char* fsOpt, bool verbose);

	bool Run();
private:
	inline BString _ReadLine();

	BString fType;
	BString fDevicePath;
	BString& fVolumeName;
	const char* fFsOptions;
	BDiskDevice fDevice;
	BPartition* fPartition;
	const bool fVerbose;
};

#endif // _FSCREATOR_H_
