//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_VISITOR_H
#define _DISK_DEVICE_VISITOR_H

#include <SupportDefs.h>

class BDiskDevice;
class BPartition;

// BDiskDeviceVisitor
class BDiskDeviceVisitor {
public:
	BDiskDeviceVisitor();
	virtual ~BDiskDeviceVisitor();

	// return true to abort iteration
	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BPartition *partition, int32 level);
};

#endif	// _DISK_DEVICE_VISITOR_H
