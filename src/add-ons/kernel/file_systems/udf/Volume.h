/*
 * Copyright 2012, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UDF_VOLUME_H
#define _UDF_VOLUME_H

/*! \file Volume.h */

#include <fs_info.h>
#include <fs_interface.h>
#include <KernelExport.h>

#include <util/kernel_cpp.h>

#include <dirent.h>

#include "UdfString.h"
#include "UdfStructures.h"
#include "Partition.h"

class Icb;

class Volume {
public:
						Volume(fs_volume *volume);
						~Volume();	

	status_t			Mount(const char *device, off_t offset, off_t length,
                		   uint32 blockSize, uint32 flags);
	status_t 			Unmount();

	bool				IsReadOnly() { return true; }

	// Address mapping
	status_t			MapBlock(long_address address, off_t *mappedBlock);
	status_t			MapExtent(long_address logicalExtent,
							extent_address &physicalExtent);

	// Miscellaneous info
	const char			*Name() const;
	int					Device() const { return fDevice; }
	dev_t				ID() const { return fFSVolume ? fFSVolume->id : -1; }
	fs_volume			*FSVolume() const { return fFSVolume; }
	off_t				Offset() const { return fOffset; }
	off_t				Length() const { return fLength; }
	void				*BlockCache() {return fBlockCache; }
	uint32				BlockSize() const { return fBlockSize; }
	uint32				BlockShift() const { return fBlockShift; }
	bool				Mounted() const { return fMounted; }
	Icb*				RootIcb() { return fRootIcb; }
	primary_volume_descriptor *PrimaryVolumeDescriptor() 
							{ return &fPrimaryVolumeDescriptor; }

private:
	void				_Unset();

	status_t			_SetPartition(uint number, Partition *partition);
	Partition*			_GetPartition(uint number);

private:
	void				*fBlockCache;
	uint32				fBlockShift;
	uint32				fBlockSize;
	int					fDevice;
	fs_volume			*fFSVolume;
	off_t				fLength;
	bool				fMounted;
	UdfString			fName;
	off_t				fOffset;
	Partition			*fPartitions[UDF_MAX_PARTITION_MAPS];
	Icb					*fRootIcb;	// Destroyed by vfs via callback to put_node()
	primary_volume_descriptor fPrimaryVolumeDescriptor;
};

#endif	// _UDF_VOLUME_H
