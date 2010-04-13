/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VOLUME_ROSTER_H
#define _VOLUME_ROSTER_H


#include <Application.h>
#include <SupportDefs.h>
#include <Volume.h>


class BVolume;
class BMessenger;


class BVolumeRoster {
public:
								BVolumeRoster();
	virtual						~BVolumeRoster();

			status_t			GetNextVolume(BVolume* volume);
			void				Rewind();

			status_t			GetBootVolume(BVolume* volume);

			status_t			StartWatching(
									BMessenger messenger = be_app_messenger);
			void				StopWatching();
			BMessenger			Messenger() const;

private:
	virtual	void				_SeveredVRoster1();
	virtual	void				_SeveredVRoster2();

private:
			int32				fCookie;
			BMessenger*			fTarget;
			uint32				_reserved[3];
};


#endif	// _VOLUME_ROSTER_H
