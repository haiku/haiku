//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDeviceVisitor.h>

// constructor
BDiskDeviceVisitor::BDiskDeviceVisitor()
{
}

// destructor
BDiskDeviceVisitor::~BDiskDeviceVisitor()
{
}

// Visit
bool
BDiskDeviceVisitor::Visit(BDiskDevice *device)
{
	return false;
}

// Visit
bool
BDiskDeviceVisitor::Visit(BSession *device)
{
	return false;
}

// Visit
bool
BDiskDeviceVisitor::Visit(BPartition *device)
{
	return false;
}

