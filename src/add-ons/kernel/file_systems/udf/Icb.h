//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_ICB_H
#define _UDF_ICB_H

/*! \file Icb.h
*/

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include <fsproto.h>
}

#include "cpp.h"
#include "UdfDebug.h"

#include "DiskStructures.h"
#include "MemoryChunk.h"

namespace Udf {

class Volume;

class Icb {
public:
	Icb(Volume *volume, udf_long_address address);
	status_t InitCheck();
	vnode_id Id() { return fId; }
private:
	Icb();	// unimplemented
private:
	Volume *fVolume;
	MemoryChunk fIcbData;
	status_t fInitStatus;
	vnode_id fId;
};

};	// namespace Udf

#endif	// _UDF_ICB_H
