// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		Directory.cpp
//
//	Description:	BVolume class
// ----------------------------------------------------------------------

#include <Volume.h>
//#include "Volume.h"

#include <Directory.h>
#include <Bitmap.h>
#include <Node.h>
#include <errno.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// ----------------------------------------------------------------------
//	BVolume (public)
// ----------------------------------------------------------------------
//	Default constructor: does nothing and sets InitCheck() to B_NO_INIT.

BVolume::BVolume(void)
{
	Unset();
}


// ----------------------------------------------------------------------
//	BVolume (public)
// ----------------------------------------------------------------------
//	Device constructor: sets the BVolume to point to the volume
//	represented by the argument. See the SetTo() function for
//	status codes.

BVolume::BVolume(
	dev_t			dev)
{
	SetTo(dev);
}


// ----------------------------------------------------------------------
//	BVolume (public)
// ----------------------------------------------------------------------
//	Copy constructor: sets the object to point to the same device as
//	does the argument.

BVolume::BVolume(
	const BVolume&	vol)
{
	fDev = vol.Device();
	fCStatus = vol.InitCheck();
}


// ----------------------------------------------------------------------
//	~BVolume (public, virtual)
// ----------------------------------------------------------------------
//	Destructor: Destroys the BVolume object.

BVolume::~BVolume(void)
{
}


// ----------------------------------------------------------------------
//	InitCheck (public)
// ----------------------------------------------------------------------
//	Returns the status of the last initialization (from either the
//	constructor or SetTo()). 

status_t
BVolume::InitCheck(void) const 
{	
	return fCStatus;
}


// ----------------------------------------------------------------------
//	SetTo (public)
// ----------------------------------------------------------------------
//	Initializes the BVolume object to represent the volume (device)
//	identified by the argument. 

status_t
BVolume::SetTo(
	dev_t			dev) 
{
	fDev = dev;
	
	// Call the kernel function that gets device information
	//	in order to determine the device status:
	fs_info			fsInfo;
	int				err = fs_stat_dev(dev, &fsInfo);
	
	if (err != 0) {
		fCStatus = errno;
	}
	else {
		fCStatus = B_OK;
	}
	
	return fCStatus;
}


// ----------------------------------------------------------------------
//	Unset (public)
// ----------------------------------------------------------------------
//	Uninitializes the BVolume. 

void
BVolume::Unset(void) 
{
	fDev = 0L;
	fCStatus = B_NO_INIT;
}


// ----------------------------------------------------------------------
//	Device (public)
// ----------------------------------------------------------------------
//	Returns the object's dev_t number. 

dev_t
BVolume::Device(void) const 
{
	return fDev;
}


// ----------------------------------------------------------------------
//	GetRootDirectory (public)
// ----------------------------------------------------------------------
//	Initializes dir (which must be allocated) to refer to the volume's
//	"root directory." The root directory stands at the "root" of the
//	volume's file hierarchy.
//
//	NOTE: This isn't necessarily the root of the entire file
//	hierarchy, but only the root of the volume hierarchy.
//
//	This function does not change fDev nor fCStatus.

status_t
BVolume::GetRootDirectory(
	BDirectory*		dir) const
{
	status_t		currentStatus = fCStatus;
	
	if ((dir != NULL) && (currentStatus == B_OK)){

		// Obtain the device information for the current device
		//	and initialize the passed-in BDirectory object with
		//	the device and root node values.
		
		fs_info			fsInfo;
		int				err = fs_stat_dev(fDev, &fsInfo);
		
		if (err != 0) {
			currentStatus = errno;
		}
		else {
			node_ref		nodeRef;
			
			nodeRef.device = fsInfo.dev;
				// NOTE: This should be the same as fDev.
			nodeRef.node = fsInfo.root;
			
			currentStatus = dir->SetTo(&nodeRef);
		}

	}
	
	return currentStatus;
}


// ----------------------------------------------------------------------
//	Capacity (public)
// ----------------------------------------------------------------------
//	Returns the volume's total storage capacity (in bytes).

off_t
BVolume::Capacity(void) const 
{
	off_t		totalBytes = 0;
	
	if (fCStatus == B_OK){

		// Obtain the device information for the current device
		//	and calculate the total storage capacity.
		
		fs_info			fsInfo;
		int				err = fs_stat_dev(fDev, &fsInfo);
		
		if (err == 0) {
			totalBytes = fsInfo.block_size * fsInfo.total_blocks;
		}

	}

	return totalBytes;
}


// ----------------------------------------------------------------------
//	FreeBytes (public)
// ----------------------------------------------------------------------
//	Returns the amount of storage that's currently unused on the
//	volume (in bytes).

off_t
BVolume::FreeBytes(void) const 
{
	off_t		remainingBytes = 0L;
	
	if (fCStatus == B_OK){

		// Obtain the device information for the current device
		//	and calculate the free storage available.
		
		fs_info			fsInfo;
		int				err = fs_stat_dev(fDev, &fsInfo);
		
		if (err == 0) {
			remainingBytes = fsInfo.block_size * fsInfo.free_blocks;
		}

	}

	return remainingBytes;
}


// ----------------------------------------------------------------------
//	GetName (public)
// ----------------------------------------------------------------------
//	Copies the name of the volume into the supplied buffer.

status_t
BVolume::GetName(
	char*			name) const 
{
	status_t		currentStatus = fCStatus;
	
	if ((name != NULL) && (currentStatus == B_OK)) {

		// Obtain the device information for the current device
		//	and copies the device name into the buffer.
		
		fs_info			fsInfo;
		int				err = fs_stat_dev(fDev, &fsInfo);
		
		if (err != 0) {
			currentStatus = errno;
		}
		else {
			strcpy(name, fsInfo.volume_name);
			currentStatus = B_OK;
		}

	}
	
	return currentStatus;
}


// ----------------------------------------------------------------------
//	SetName (public)
// ----------------------------------------------------------------------
//	Sets the name of the volume to the supplied string.
//	Setting the name is typically (and most politely) the user's
//	responsibility (a task that's performed, most easily, through the
//	Tracker). If you really want to set the name of the volume
//	programmatically, you do so by renaming the volume's root directory.

status_t
BVolume::SetName(
	const char*		name) 
{
	status_t		currentStatus = B_ERROR;
	
	// *** Call a kernel or, more indirectly, a POSIX function
	//	that sets a volume name ***
	
	return currentStatus;
}


// ----------------------------------------------------------------------
//	GetIcon (public)
// ----------------------------------------------------------------------
//	Returns the volume's icon in icon. which specifies the icon to
//	retrieve, either B_MINI_ICON (16x16) or B_LARGE_ICON (32x32).

status_t
BVolume::GetIcon(
	BBitmap*		icon,
	icon_size		which) const
{
	status_t		currentStatus = fCStatus;
	
	if ((icon != NULL) && (currentStatus == B_OK)
		&& ((which == B_MINI_ICON) || (which == B_LARGE_ICON))) {
		char		deviceName[B_DEV_NAME_LENGTH];
		
		currentStatus = GetName(deviceName);
		
		if (currentStatus == B_OK)
		{
			currentStatus = get_device_icon(deviceName, icon, which);
		}
	}
	
	return (currentStatus);
}


// ----------------------------------------------------------------------
//	IsRemovable (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it is removable.

bool
BVolume::IsRemovable(void) const
{
	bool			volumeIsRemovable = false;

	// Obtain the device information for the current device
	//	and determines whether or not the device is removable.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeIsRemovable = (fsInfo.flags & B_FS_IS_REMOVABLE);
	}
	
	return (volumeIsRemovable);
}


// ----------------------------------------------------------------------
//	IsReadOnly (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it is read-only.

bool
BVolume::IsReadOnly(void) const
{
	bool		volumeIsReadOnly = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the device is read-only.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeIsReadOnly = (fsInfo.flags & B_FS_IS_READONLY);
	}
	
	return (volumeIsReadOnly);
}


// ----------------------------------------------------------------------
//	IsPersistent (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it is persistent.

bool
BVolume::IsPersistent(void) const
{
	bool		volumeIsPersistent = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the storage medium
	//	is persistent.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeIsPersistent = (fsInfo.flags & B_FS_IS_PERSISTENT);
	}
	
	return (volumeIsPersistent);
}


// ----------------------------------------------------------------------
//	IsShared (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it is shared.

bool
BVolume::IsShared(void) const
{
	bool		volumeIsShared = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the volume is shared
	//	over a network.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeIsShared = (fsInfo.flags & B_FS_IS_SHARED);
	}
	
	return (volumeIsShared);
}


// ----------------------------------------------------------------------
//	KnowsMime (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it uses MIME types.

bool
BVolume::KnowsMime(void) const
{
	bool		volumeKnowsMime = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the volume supports
	//	MIME types.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeKnowsMime = (fsInfo.flags & B_FS_HAS_MIME);
	}
	
	return (volumeKnowsMime);
}


// ----------------------------------------------------------------------
//	KnowsAttr (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not its files
//	accept attributes.

bool
BVolume::KnowsAttr(void) const
{
	bool		volumeKnowsAttr = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the files on the
	//	volume accept attributes.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeKnowsAttr = (fsInfo.flags & B_FS_HAS_ATTR);
	}
	
	return (volumeKnowsAttr);
}


// ----------------------------------------------------------------------
//	KnowsQuery (public)
// ----------------------------------------------------------------------
//	Tests the volume and returns whether or not it can respond
//	to queries.

bool
BVolume::KnowsQuery(void) const
{
	bool		volumeKnowsQuery = false;
	
	// Obtain the device information for the current device
	//	and determines whether or not the volume can
	//	respond to queries.
	
	fs_info			fsInfo;
	int				err = fs_stat_dev(fDev, &fsInfo);
	
	if (err == 0) {
		volumeKnowsQuery = (fsInfo.flags & B_FS_HAS_QUERY);
	}
	
	return (volumeKnowsQuery);
}


// ----------------------------------------------------------------------
//	operator == (public)
// ----------------------------------------------------------------------
//	Two BVolume objects are said to be equal if they refer to the
//	same volume, or if they're both uninitialized.
//	Returns whether or not the volumes are equal.

bool
BVolume::operator==(
	const BVolume&		vol) const
{
	// First determine whether both objects are uninitialized,
	//	since comparing the fDev members of uninitialized BVolume
	//	instances will return an invalid result.
	
	bool		areEqual = ((this->fCStatus == B_NO_INIT)
								&& (vol.InitCheck() == B_NO_INIT));

	if (!areEqual) {
		// The BVolume instance are initialized, test the
		//	fDev member values:
		areEqual = (this->fDev == vol.Device());
	}
	
	return (areEqual);
}


// ----------------------------------------------------------------------
//	operator != (public)
// ----------------------------------------------------------------------
//	Two BVolume objects are said to be equal if they refer to the
//	same volume, or if they're both uninitialized.
//	Returns whether or not the volumes are not equal.

bool
BVolume::operator!=(
	const BVolume&		vol) const
{
	bool		areNotEqual = !(*this == vol);
	
	return (areNotEqual);
}


// ----------------------------------------------------------------------
//	operator = (public)
// ----------------------------------------------------------------------
//	In the expression:
//
//		BVolume a = b;
//
//	BVolume a is initialized to refer to the same volume as b.
//	To gauge the success of the assignment, you should call InitCheck()
//	immediately afterwards.
//
//	Assigning a BVolume to itself is safe. 
//	Assigning from an uninitialized BVolume is "successful":
//	The assigned-to BVolume will also be uninitialized (B_NO_INIT).

BVolume&
BVolume::operator=(
	const BVolume&		vol)
{
	this->fDev = vol.Device();
	this->fCStatus = vol.InitCheck();

	return (*this);
}


// FBC 
void BVolume::_TurnUpTheVolume1() {} 
void BVolume::_TurnUpTheVolume2() {} 
void BVolume::_TurnUpTheVolume3() {} 
void BVolume::_TurnUpTheVolume4() {} 
void BVolume::_TurnUpTheVolume5() {} 
void BVolume::_TurnUpTheVolume6() {} 
void BVolume::_TurnUpTheVolume7() {} 
void BVolume::_TurnUpTheVolume8() {}


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

