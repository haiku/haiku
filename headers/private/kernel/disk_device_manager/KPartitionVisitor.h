// KPartitionVisitor.h

#ifndef _K_DISK_DEVICE_PARTITION_VISITOR_H
#define _K_DISK_DEVICE_PARTITION_VISITOR_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KPartition;

class KPartitionVisitor {
public:
	KPartitionVisitor();
	virtual ~KPartitionVisitor();

	virtual bool VisitPre(KPartition *partition);
	virtual bool VisitPost(KPartition *partition);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KPartitionVisitor;

#endif	// _K_DISK_DEVICE_PARTITION_VISITOR_H
