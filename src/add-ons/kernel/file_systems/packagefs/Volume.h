/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_interface.h>


class Directory;


class Volume {
public:
								Volume(fs_volume* fsVolume);
								~Volume();

			status_t			Mount();
			void				Unmount();

			Directory*			RootDirectory() const { return fRootDirectory; }

private:
			fs_volume*			fFSVolume;
			Directory*			fRootDirectory;
};


#endif	// VOLUME_H
