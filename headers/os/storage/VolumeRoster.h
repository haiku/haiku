// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		VolumeRoster.h
//
//	Description:	BVolumeRoster class
// ----------------------------------------------------------------------


#ifndef __sk_volumeroster_h__
#define __sk_volumeroster_h__

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <SupportDefs.h>
#include <Volume.h>
#include <Application.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

const char	kBootVolumeName[B_DEV_NAME_LENGTH] = "/boot";

class BVolume;
class BMessenger;

class BVolumeRoster {
	public:
							BVolumeRoster(void);
		virtual				~BVolumeRoster(void);

		status_t			GetNextVolume(BVolume* vol);
		void				Rewind(void);

		status_t			GetBootVolume(BVolume* boot_vol);
		status_t			StartWatching(BMessenger msngr=be_app_messenger);

		void				StopWatching(void);

		BMessenger			Messenger(void) const;

	private:

	virtual	void			_SeveredVRoster1(void);
	virtual	void			_SeveredVRoster2(void);
	
		int32				fPos;
		BMessenger*			fTarget;

#if !_PR3_COMPATIBLE_
		uint32			_reserved[3];
#endif

};


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif	// __sk_volumeroster_h__
