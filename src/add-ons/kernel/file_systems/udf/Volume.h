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

#include "cpp.h"
#include "UdfDebug.h"

#include "DiskStructures.h"

namespace UDF {

class Volume {
public:
	static status_t Identify(int device, off_t base = 0);

	Volume(nspace_id id);
		
	status_t Mount(const char *deviceName, off_t volumeStart, off_t volumeLength, uint32 flags, 
	               uint32 blockSize = 2048);
	status_t Unmount();
	
	const char *Name() const;
	int Device() const { return fDevice; }
	nspace_id ID() const { return fID; }
	
	off_t StartAddress() const { return fStartAddress; }
	off_t Length() const { return fLength; }
	
	uint32 BlockSize() const { return fBlockSize; }
	off_t AddressForRelativeBlock(off_t block) { return StartAddress() + block * BlockSize(); }
	off_t RelativeAddress(off_t address) { return StartAddress() + address; }
	
	
	bool IsReadOnly() const { return fReadOnly; }
	
	vnode_id ToVnodeID(off_t block) const { return (vnode_id)block; }	
private:
	status_t _InitStatus() const { return fInitStatus; }
	// Private _InitStatus() status_t values
	enum {
		B_UNINITIALIZED = B_ERRORS_END+1,	//!< Completely uninitialized
		B_DEVICE_INITIALIZED,				//!< Initialized enough to access underlying device safely
		
		B_INITIALIZED = B_OK,
	};
	
	// Called by Mount(), either directly or indirectly
	status_t _Identify();
	status_t _WalkVolumeRecognitionSequence();
	status_t _WalkVolumeDescriptorSequence(extent_address extent);
		
private:
	nspace_id fID;
	int fDevice;
	bool fReadOnly;

	off_t fStartAddress;	//!< Start address of volume on given device
	off_t fLength;			//!< Length of volume (in blocks)
	uint32 fBlockSize;

	status_t fInitStatus;
};

};	// namespace UDF

#endif	// _UDF_VOLUME_H
