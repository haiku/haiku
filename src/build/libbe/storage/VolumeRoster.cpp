// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		VolumeRoster.cpp
//
//	Description:	BVolumeRoster class
// ----------------------------------------------------------------------
/*!
	\file VolumeRoster.cpp
	BVolumeRoster implementation.
*/

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

/*!
	\class BVolumeRoster
	\brief A roster of all volumes available in the system
	
	Provides an interface for iterating through the volumes available in
	the system and watching volume mounting/unmounting.

	The class wraps the next_dev() function for iterating through the
	volume list and the watch_node()/stop_watching() for the watching
	features.

	\author Vincent Dominguez
	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	
	\version 0.0.0
*/

/*!	\var dev_t BVolumeRoster::fCookie
	\brief The iteration cookie for next_dev(). Initialized with 0.
*/

/*!	\var dev_t BVolumeRoster::fTarget
	\brief BMessenger referring to the target to which the watching
		   notification messages are sent.

	The object is allocated and owned by the roster. \c NULL, if not watching.
*/

// constructor
/*!	\brief Creates a new BVolumeRoster.

	The object is ready to be used.
*/
BVolumeRoster::BVolumeRoster()
	: fCookie(0),
	  fTarget(NULL)
{
}

// destructor
/*!	\brief Frees all resources associated with this object.

	If a watching was activated on (StartWatching()), it is deactived.
*/
BVolumeRoster::~BVolumeRoster()
{
	StopWatching();
}

// GetNextVolume
/*!	\brief Returns the next volume in the list of available volumes.
	\param volume A pointer to a pre-allocated BVolume to be initialized to
		   refer to the next volume in the list of available volumes.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The last volume in the list has already been returned.
*/
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

// Rewind
/*! \brief Rewinds the list of available volumes such that the next call to
		   GetNextVolume() will return the first element in the list.
*/
void
BVolumeRoster::Rewind()
{
	fCookie = 0;
}

// GetBootVolume
/*!	\brief Returns the boot volume.

	Currently, this function looks for the volume that is mounted at "/boot".
	The only way to fool the system into thinking that there is not a boot
	volume is to rename "/boot" -- but, please refrain from doing so...(:o(

	\param volume A pointer to a pre-allocated BVolume to be initialized to
		   refer to the boot volume.
	\return
	- \c B_OK: Everything went fine.
	- an error code otherwise
*/
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

// StartWatching
/*!	\brief Starts watching the list of volumes available in the system.

	Notifications are sent to the specified target whenever a volume is
	mounted or unmounted. The format of the notification messages is
	described under watch_node(). Actually BVolumeRoster just provides a
	more convenient interface for it.

	If StartWatching() has been called before with another target and no
	StopWatching() since, StopWatching() is called first, so that the former
	target won't receive any notifications anymore.

	When the object is destroyed all watching has an end as well.

	\param messenger The target to which the notification messages shall be
		   sent.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The supplied BMessenger is invalid.
	- \c B_NO_MEMORY: Insufficient memory to carry out this operation.
*/
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

// StopWatching
/*!	\brief Stops volume watching initiated with StartWatching() before.
*/
void
BVolumeRoster::StopWatching()
{
	if (fTarget) {
		stop_watching(*fTarget);
		delete fTarget;
		fTarget = NULL;
	}
}

// Messenger
/*!	\brief Returns a messenger to the target currently watching the volume
		   list.
	\return A messenger to the target currently watching the volume list, or
			an invalid messenger, if noone is currently watching.
*/
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
