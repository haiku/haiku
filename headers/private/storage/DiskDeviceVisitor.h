//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_VISITOR_H
#define _DISK_DEVICE_VISITOR_H

class BDiskDevice;
class BPartition;
class BSession;

// BDiskDeviceVisitor
class BDiskDeviceVisitor {
	BDiskDeviceVisitor();
	virtual ~BDiskDeviceVisitor();

	// return true to abort iteration
	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BSession *device);
	virtual bool Visit(BPartition *device);
};

#endif _DISK_DEVICE_VISITOR_H
