/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_info.h>
#include <String.h>

#include <packagefs.h>


class BDirectory;

class Root;


class Volume {
public:
								Volume();
								~Volume();

			status_t			Init(BDirectory& directory,
									dev_t& _rootDeviceID, ino_t& _rootNodeID);

			const BString&		Path() const
									{ return fPath; }
			PackageFSMountType	MountType() const
									{ return fMountType; }
			dev_t				DeviceID() const
									{ return fDeviceID; }
			ino_t				RootDirectoryID() const
									{ return fRootDirectoryID; }

			Root*				GetRoot() const
									{ return fRoot; }
			void				SetRoot(Root* root)
									{ fRoot = root; }

private:
			BString				fPath;
			PackageFSMountType	fMountType;
			dev_t				fDeviceID;
			ino_t				fRootDirectoryID;
			Root*				fRoot;
};



#endif	// VOLUME_H
