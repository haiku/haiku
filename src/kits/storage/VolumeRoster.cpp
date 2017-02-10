// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  File Name:		VolumeRoster.cpp
//
//	Description:	BVolumeRoster class
// ----------------------------------------------------------------------


#include <errno.h>
#include <new>

#include <Bitmap.h>
#include <Directory.h>
#include <fs_info.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>


static const char kBootVolumePath[] = "/boot";

using namespace std;


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif


BVolumeRoster::BVolumeRoster()
	: fCookie(0),
	  fTarget(NULL)
{
}


// Deletes the volume roster and frees all associated resources.
BVolumeRoster::~BVolumeRoster()
{
	StopWatching();
}


// Fills out the passed in BVolume object with the next available volume.
status_t
BVolumeRoster::GetNextVolume(BVolume *volume)
{
	// check parameter
	status_t error = (volume ? B_OK : B_BAD_VALUE);
	// get next device
	dev_t device;
	if (error == B_OK) {
		device = next_dev(&fCookie);
		if (device < 0)
			error = device;
	}
	// init volume
	if (error == B_OK)
		error = volume->SetTo(device);
	return error;
}


// Rewinds the list of available volumes back to the first item.
void
BVolumeRoster::Rewind()
{
	fCookie = 0;
}


// Fills out the passed in BVolume object with the boot volume.
status_t
BVolumeRoster::GetBootVolume(BVolume *volume)
{
	// check parameter
	status_t error = (volume ? B_OK : B_BAD_VALUE);
	// get device
	dev_t device;
	if (error == B_OK) {
		device = dev_for_path(kBootVolumePath);
		if (device < 0)
			error = device;
	}
	// init volume
	if (error == B_OK)
		error = volume->SetTo(device);
	return error;
}


// Starts watching the available volumes for changes.
status_t
BVolumeRoster::StartWatching(BMessenger messenger)
{
	StopWatching();
	status_t error = (messenger.IsValid() ? B_OK : B_ERROR);
	// clone messenger
	if (error == B_OK) {
		fTarget = new(nothrow) BMessenger(messenger);
		if (!fTarget)
			error = B_NO_MEMORY;
	}
	// start watching
	if (error == B_OK)
		error = watch_node(NULL, B_WATCH_MOUNT, messenger);
	// cleanup on failure
	if (error != B_OK && fTarget) {
		delete fTarget;
		fTarget = NULL;
	}
	return error;
}


// Stops watching volumes initiated by StartWatching().
void
BVolumeRoster::StopWatching()
{
	if (fTarget) {
		stop_watching(*fTarget);
		delete fTarget;
		fTarget = NULL;
	}
}


// Returns the messenger currently watching the volume list.
BMessenger
BVolumeRoster::Messenger() const
{
	return (fTarget ? *fTarget : BMessenger());
}


// FBC
void BVolumeRoster::_SeveredVRoster1() {}
void BVolumeRoster::_SeveredVRoster2() {}


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif
