// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		VolumeRoster.cpp
//
//	Description:	BVolumeRoster class
// ----------------------------------------------------------------------

#include <VolumeRoster.h>

#include <Directory.h>
#include <Bitmap.h>
#include <Node.h>
#include <errno.h>
#include <fs_info.h>
#include <kernel_interface.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

// ----------------------------------------------------------------------
//	BVolumeRoster (public)
// ----------------------------------------------------------------------
//	Default constructor: creates a new BVolumeRoster object.
//	You don't have to "initialize" the object before using it (as you do
//	with most other Storage Kit classes). You can call GetNextVolume()
//	(or another method) immediately after constructing.

BVolumeRoster::BVolumeRoster(void)
{
#if !_PR3_COMPATIBLE_
	// Initialize reserved class variables to "safe" values:
	_reserved[0] = 0L;
	_reserved[1] = 0L;
	_reserved[2] = 0L;
		// Place this in a separate initialization method at
		//	a later time.
#endif
	
	fPos = 0;
	fTarget = NULL;
}


// ----------------------------------------------------------------------
//	~BVolumeRoster (public, virtual)
// ----------------------------------------------------------------------
//	Destructor: Destroys the BVolumeRoster object.
//	If this BVolumeRoster object was watching volumes,
//	the watch is called off.

BVolumeRoster::~BVolumeRoster(void)
{
}


// ----------------------------------------------------------------------
//	GetNextVolume (public)
// ----------------------------------------------------------------------
//	Retrieves the "next" volume from the volume list and uses it to
//	initialize the argument (which must be allocated). When the function 
//	returns B_BAD_VALUE, you've reached the end of the list. 

status_t
BVolumeRoster::GetNextVolume(BVolume* vol) 
{
	status_t		currentStatus = B_BAD_VALUE;
	dev_t			deviceID = next_dev(&fPos);
	
	if ((deviceID >= 0) && (vol != NULL))
	{
		currentStatus = vol->SetTo(deviceID);
	}
	
	return currentStatus;
}


// ----------------------------------------------------------------------
//	Rewind (public)
// ----------------------------------------------------------------------
//	Rewinds the volume list such that the next call to GetNextVolume()
//	will return the first element in the list.

void
BVolumeRoster::Rewind(void) 
{
	fPos = 0;
}


// ----------------------------------------------------------------------
//	GetBootVolume (public)
// ----------------------------------------------------------------------
//	Initializes boot_vol to refer to the "boot volume." This is the
//	volume that was used to boot the computer. boot_vol must be
//	allocated before you pass it in. If the boot volume can't be found,
//	the argument is uninitialized.
//
//	Currently, this function looks for the volume that is mounted at /boot.
//	The only way to fool the system into thinking that there is not a boot
//	volume is to rename /boot -- but, please refrain from doing so...(:o(

status_t
BVolumeRoster::GetBootVolume(BVolume* boot_vol) 
{
	int32		pos = 0; 
	dev_t		deviceID = 0;
	status_t	currentStatus = B_BAD_VALUE;
	
	do {
		deviceID = next_dev(&pos);
		
		if(deviceID >= 0) {
			fs_info			fsInfo;
			int				err = StorageKit::stat_dev(deviceID, &fsInfo);
			
			if (err == 0) {
				if(std::strcmp(fsInfo.device_name, kBootVolumeName) == 0) {
					fPos = pos;
					currentStatus = boot_vol->SetTo(deviceID);
					break;
				}
			}
		}
	} while(deviceID >= 0);
	
	return currentStatus;
}


// ----------------------------------------------------------------------
//	StartWatching (public)
// ----------------------------------------------------------------------
//	Registers a request for notifications of volume mounts and unmounts.
//	The notifications are sent (as BMessages) to the BHandler/BLooper
//	pair specified by the argument. There are separate messages for
//	mounting and unmounting; their formats are described below.
//
//	The caller retains possession of the BHandler/BLooper that the
//	BMessenger represents. The volume watching continues until this 
//	BVolumeRoster object is destroyed, or until you call StopWatching(). 

status_t
BVolumeRoster::StartWatching(BMessenger msngr) 
{
	return B_BAD_VALUE;
}


// ----------------------------------------------------------------------
//	StopWatching (public)
// ----------------------------------------------------------------------
//	Tells the volume-watcher to stop watching. Notifications of volume
//	mounts and unmounts are no longer sent to the BVolumeRoster's target. 

void
BVolumeRoster::StopWatching(void) 
{
}


// ----------------------------------------------------------------------
//	Messenger (public)
// ----------------------------------------------------------------------
//	Returns a copy of the BMessenger object that was set in the
//	previous StartWatching() call. 

BMessenger
BVolumeRoster::Messenger(void) const
{
	return *fTarget;
}


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

