//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <Partition.h>

// constructor
BPartition::BPartition()
{
}

// destructor
BPartition::~BPartition()
{
}

// Session
BSession *
BPartition::Session() const
{
	return NULL;	// not implemented
}

// Device
BDiskDevice *
BPartition::Device() const
{
	return NULL;	// not implemented
}

// Offset
off_t
BPartition::Offset() const
{
	return 0;	// not implemented
}

// Size
off_t
BPartition::Size() const
{
	return 0;	// not implemented
}

// BlockSize
int32
BPartition::BlockSize() const
{
	return 0;	// not implemented
}

// Index
int32
BPartition::Index() const
{
	return -1;	// not implemented
}

// Flags
uint32
BPartition::Flags() const
{
	return 0;	// not implemented
}

// IsHidden
bool
BPartition::IsHidden() const
{
	return false;	// not implemented
}

// IsVirtual
bool
BPartition::IsVirtual() const
{
	return false;	// not implemented
}

// IsEmpty
bool
BPartition::IsEmpty() const
{
	return false;	// not implemented
}

// Name
const char *
BPartition::Name() const
{
	return NULL;	// not implemented
}

// Type
const char *
BPartition::Type() const
{
	return NULL;	// not implemented
}

// FileSystemShortName
const char *
BPartition::FileSystemShortName() const
{
	return NULL;	// not implemented
}

// FileSystemLongName
const char *
BPartition::FileSystemLongName() const
{
	return NULL;	// not implemented
}

// VolumeName
const char *
BPartition::VolumeName() const
{
	return NULL;	// not implemented
}

// FileSystemFlags
uint32
BPartition::FileSystemFlags() const
{
	return NULL;	// not implemented
}

// IsMounted
bool
BPartition::IsMounted() const
{
	return NULL;	// not implemented
}

// UniqueID
int32
BPartition::UniqueID() const
{
	return 0;	// not implemented
}

// GetVolume
status_t
BPartition::GetVolume(BVolume *volume) const
{
	return B_ERROR;	// not implemented
}

// GetIcon
status_t
BPartition::GetIcon(BBitmap *icon, icon_size which) const
{
	return B_ERROR;	// not implemented
}

// Mount
status_t
BPartition::Mount(uint32 mountflags, const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Unmount
status_t
BPartition::Unmount()
{
	return B_ERROR;	// not implemented
}

// GetInitializationParameters
status_t
BPartition::GetInitializationParameters(const char *fileSystem,
										BPoint dialogCenter,
										BString *parameters,
										bool *cancelled);
{
	return B_ERROR;	// not implemented
}

// Initialize
status_t
BPartition::Initialize(const char *fileSystem, const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Initialize
status_t
BPartition::Initialize(const char *fileSystem, BPoint dialogCenter,
					   bool *cancelled);
{
	return B_ERROR;	// not implemented
}

// GetFileSystemList
status_t
BPartition::GetFileSystemList(BObjectList<BString*> *list)
{
	return B_ERROR;	// not implemented
}

// constructor
BPartition::BPartition(const BPartition &)
{
}

// =
BPartition &
BPartition::operator=(const BPartition &)
{
	return *this;
}

