// KPartitionListener.cpp

#include <KPartitionListener.h>
#include <util/kernel_cpp.h>


// constructor
KPartitionListener::KPartitionListener()
{
}

// destructor
KPartitionListener::~KPartitionListener()
{
}

// OffsetChanged
void
KPartitionListener::OffsetChanged(KPartition *partition, off_t offset)
{
}

// SizeChanged
void
KPartitionListener::SizeChanged(KPartition *partition, off_t size)
{
}

// ContentSizeChanged
void
KPartitionListener::ContentSizeChanged(KPartition *partition, off_t size)
{
}

// BlockSizeChanged
void
KPartitionListener::BlockSizeChanged(KPartition *partition, uint32 blockSize)
{
}

// IndexChanged
void
KPartitionListener::IndexChanged(KPartition *partition, int32 index)
{
}

// StatusChanged
void
KPartitionListener::StatusChanged(KPartition *partition, uint32 status)
{
}

// FlagsChanged
void
KPartitionListener::FlagsChanged(KPartition *partition, uint32 flags)
{
}

// NameChanged
void
KPartitionListener::NameChanged(KPartition *partition, const char *name)
{
}

// ContentNameChanged
void
KPartitionListener::ContentNameChanged(KPartition *partition, const char *name)
{
}

// TypeChanged
void
KPartitionListener::TypeChanged(KPartition *partition, const char *type)
{
}

// IDChanged
void
KPartitionListener::IDChanged(KPartition *partition, partition_id id)
{
}

// VolumeIDChanged
void
KPartitionListener::VolumeIDChanged(KPartition *partition, dev_t volumeID)
{
}

// MountCookieChanged
void
KPartitionListener::MountCookieChanged(KPartition *partition, void *cookie)
{
}

// ParametersChanged
void
KPartitionListener::ParametersChanged(KPartition *partition,
									  const char *parameters)
{
}

// ContentParametersChanged
void
KPartitionListener::ContentParametersChanged(KPartition *partition,
											 const char *parameters)
{
}

// ChildAdded
void
KPartitionListener::ChildAdded(KPartition *partition, KPartition *child,
							   int32 index)
{
}

// ChildRemoved
void
KPartitionListener::ChildRemoved(KPartition *partition, KPartition *child,
								 int32 index)
{
}

// DiskSystemChanged
void
KPartitionListener::DiskSystemChanged(KPartition *partition,
									  KDiskSystem *diskSystem)
{
}

// CookieChanged
void
KPartitionListener::CookieChanged(KPartition *partition, void *cookie)
{
}

// ContentCookieChanged
void
KPartitionListener::ContentCookieChanged(KPartition *partition, void *cookie)
{
}

