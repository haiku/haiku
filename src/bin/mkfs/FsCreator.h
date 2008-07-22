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
							FsCreator(const char* path, const char* type,
								const char* volumeName, const char* fsOptions,
								bool quick, bool verbose);

			bool			Run();

private:
			BString			_ReadLine();

			const char*		fType;
			const char*		fPath;
			const char*		fVolumeName;
			const char*		fFsOptions;
			bool			fVerbose;
			bool			fQuick;
};

#endif	// _FSCREATOR_H_
