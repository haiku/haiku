/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <fs_interface.h>


class Node;


class Volume {
public:
								Volume(fs_volume* fsVolume);
								~Volume();

			fs_volume*			FSVolume() const	{ return fFSVolume; }
			dev_t				ID() const			{ return fFSVolume->id; }

			fs_volume*			SourceFSVolume() const
													{ return fSourceFSVolume; }

			Node*				RootNode() const 	{ return fRootNode; }
			const char*			Name() const 		{ return fName; }

			status_t			Mount(const char* parameterString);
			void				Unmount();

			const fs_vnode_ops*	VnodeOps() const	{ return &fVnodeOps; }

private:
			struct VolumeListener;
	friend	struct VolumeListener;

private:
			status_t			_InitVnodeOpsFrom(fs_vnode* sourceNode);

private:
			fs_volume*			fFSVolume;
			fs_volume*			fSourceFSVolume;
			Node*				fRootNode;

			fs_vnode_ops		fVnodeOps;

			char				fName[B_PATH_NAME_LENGTH];
};


#endif	// VOLUME_H
