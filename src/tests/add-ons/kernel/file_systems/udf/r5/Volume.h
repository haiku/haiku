//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
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
#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

#ifdef COMPILE_FOR_R5
extern "C" {
#endif
	#include "fsproto.h"
#ifdef COMPILE_FOR_R5
}
#endif

#include "kernel_cpp.h"
#include "UdfDebug.h"

#include "UdfString.h"
#include "UdfStructures.h"
#include "Partition.h"

namespace Udf {

class Icb;

class Volume {
public:
	// Construction/destruction
	Volume(nspace_id id);
	~Volume();	
		
	// Mounting/unmounting
	status_t Mount(const char *deviceName, off_t offset, off_t length,
                   uint32 blockSize, uint32 flags);
	status_t Unmount();
	
	// Address mapping
	status_t MapBlock(long_address address, off_t *mappedBlock);
	status_t MapExtent(long_address logicalExtent, extent_address &physicalExtent);

	// Miscellaneous info
	const char *Name() const;
	int Device() const { return fDevice; }
	nspace_id Id() const { return fId; }
	off_t Offset() const { return fOffset; }
	off_t Length() const { return fLength; }
	uint32 BlockSize() const { return fBlockSize; }
	uint32 BlockShift() const { return fBlockShift; }
	bool Mounted() const { return fMounted; }
	Icb* RootIcb() { return fRootIcb; }
		
private:
	Volume();						// unimplemented
	Volume(const Volume &ref);		// unimplemented
	Volume& operator=(const Volume &ref);	// unimplemented

	void _Unset();

	status_t _SetPartition(uint number, Partition *partition);
	Partition* _GetPartition(uint number);

private:
	nspace_id fId;
	int fDevice;
	bool fMounted;
	off_t fOffset;
	off_t fLength;
	uint32 fBlockSize;
	uint32 fBlockShift;
	Partition *fPartitions[UDF_MAX_PARTITION_MAPS];
	Icb *fRootIcb;	// Destroyed by vfs via callback to release_node()
	String fName;
};

};	// namespace Udf

#endif	// _UDF_VOLUME_H
