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

namespace UDF {

class Volume {
public:
	Volume(nspace_id id);
	
	static status_t Identify(int device) { return Identify(device, 0); }
	static status_t Identify(int device, off_t base);
	
	status_t Mount(const char *deviceName, uint32 flags);
	status_t Unmount();
	
	nspace_id GetID() { return fID; }

	const char *Name() const;
	int Device() const { return fDevice; }
	
	bool IsReadOnly() const { return fReadOnly; }
	
	vnode_id ToVnodeID(off_t block) const { return (vnode_id)block; }
	
private:
	nspace_id fID;
	int fDevice;
	bool fReadOnly;	
};

};	// namespace UDF

#endif	// _UDF_VOLUME_H
