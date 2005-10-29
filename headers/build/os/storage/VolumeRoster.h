// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		VolumeRoster.h
//
//	Description:	BVolumeRoster class
// ----------------------------------------------------------------------
/*!
	\file VolumeRoster.h
	BVolumeRoster interface declarations.
*/
#ifndef _VOLUME_ROSTER_H
#define _VOLUME_ROSTER_H

#include <Application.h>
#include <SupportDefs.h>
#include <Volume.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BVolume;
class BMessenger;

class BVolumeRoster {
public:
	BVolumeRoster();
	virtual ~BVolumeRoster();

	status_t GetNextVolume(BVolume *volume);
	void Rewind();

	status_t GetBootVolume(BVolume *volume);

	status_t StartWatching(BMessenger messenger = be_app_messenger);
	void StopWatching();
	BMessenger Messenger() const;

private:
	virtual	void _SeveredVRoster1();
	virtual	void _SeveredVRoster2();
	
	int32		fCookie;
	BMessenger	*fTarget;
	uint32		_reserved[3];
};

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif	// _VOLUME_ROSTER_H
