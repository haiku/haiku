//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Mad props to Axel Dörfler and his BFS implementation, from which
//  this UDF implementation draws much influence (and a little code :-P).
//---------------------------------------------------------------------
#ifndef _UDF_VOLUME_H
#define _UDF_VOLUME_H

/*! \file Volume.h
*/

#include <KernelExport.h>
extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include <fsproto.h>
}

#include "kernel_cpp.h"
#include "UdfDebug.h"

#include "CS0String.h"
#include "DiskStructures.h"
#include "PartitionMap.h"

namespace Udf {

class Icb;

class Volume {
public:
	static status_t Identify(int device, off_t offset, off_t length, uint32 blockSize, char *volumeName);

	Volume(nspace_id id);		
		
	status_t Mount(const char *deviceName, off_t volumeStart, off_t volumeLength, uint32 flags, 
	               uint32 blockSize = 2048);
	status_t Unmount();
	
	const char *Name() const;
	int Device() const { return fDevice; }
	nspace_id Id() const { return fId; }
	
	off_t Offset() const { return fOffset; }
	off_t Length() const { return fLength; }
	
	uint32 BlockSize() const { return fBlockSize; }
	uint32 BlockShift() const { return fBlockShift; }
	
	off_t AddressForRelativeBlock(off_t block) { return (Offset() + block) * BlockSize(); }
	off_t RelativeAddress(off_t address) { return Offset() * BlockSize() + address; }
		
	bool IsReadOnly() const { return fReadOnly; }
	
	vnode_id ToVnodeId(off_t block) const { return (vnode_id)block; }	

	template <class AddressType>
		ssize_t Read(AddressType address, ssize_t length, void *data);
		
#if (!DRIVE_SETUP_ADDON)
	Icb* RootIcb() { return fRootIcb; }
#endif

	status_t MapAddress(udf_long_address address, off_t *mappedAddress);
	off_t MapAddress(udf_extent_address address);
	status_t MapBlock(udf_long_address address, off_t *mappedBlock);
	off_t MapAddress(udf_short_address address);
private:
	Volume();						// unimplemented
	Volume(const Volume &ref);		// unimplemented
	Volume& operator=(const Volume &ref);	// unimplemented

	status_t _InitStatus() const { return fInitStatus; }
	// Private _InitStatus() status_t values
	enum {
		B_UNINITIALIZED = B_ERRORS_END+1,	//!< Completely uninitialized
		B_DEVICE_INITIALIZED,				//!< Initialized enough to access underlying device safely
		B_IDENTIFIED,						//!< Verified to be a UDF volume on disc
		B_LOGICAL_VOLUME_INITIALIZED,		//!< Initialized enough to map addresses
		
		B_INITIALIZED = B_OK,
	};
	
	
	// Called by Mount(), either directly or indirectly
	status_t _Init(int device, off_t offset, off_t length, int blockSize);
	status_t _Identify();
	status_t _Mount();
	status_t _WalkVolumeRecognitionSequence();
	status_t _WalkAnchorVolumeDescriptorSequences();
	status_t _WalkVolumeDescriptorSequence(udf_extent_address extent);
	status_t _InitFileSetDescriptor();
			
private:
	nspace_id fId;
	int fDevice;
	bool fReadOnly;

	off_t fOffset;	//!< Starting block of the volume on the given device
	off_t fLength;	//!< Block length of volume on the given device
	uint32 fBlockSize;
	uint32 fBlockShift;

	status_t fInitStatus;
	
	udf_logical_descriptor fLogicalVD;
	PartitionMap fPartitionMap;
#if (!DRIVE_SETUP_ADDON)
	Icb *fRootIcb;	// Destroyed by vfs via callback to udf_release_node()
#endif
	CS0String fName;
};

//----------------------------------------------------------------------
// Template functions
//----------------------------------------------------------------------


template <class AddressType>
status_t
Volume::Read(AddressType address, ssize_t length, void *data)
{
	DEBUG_INIT(CF_PRIVATE | CF_HIGH_VOLUME, "Volume");
	off_t mappedAddress;
	status_t err = data ? B_OK : B_BAD_VALUE;
	if (!err)
		err = MapAddress(address, &mappedAddress);
	if (!err) {
		ssize_t bytesRead = read_pos(fDevice, mappedAddress, data, BlockSize());
		if (bytesRead != (ssize_t)BlockSize()) {
			err = B_IO_ERROR;
			PRINT(("read_pos(pos:%Ld, len:%ld) failed with: 0x%lx\n", mappedAddress,
			       length, bytesRead));
		}
	}
	RETURN(err);
}


};	// namespace Udf

#endif	// _UDF_VOLUME_H
