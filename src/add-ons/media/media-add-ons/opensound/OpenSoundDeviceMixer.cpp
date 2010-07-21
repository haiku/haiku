/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 */

#include "OpenSoundDeviceMixer.h"
#include "debug.h"
#include "driver_io.h"
#include <MediaDefs.h>
#include <Debug.h>
#include <errno.h>
#include <string.h>

OpenSoundDeviceMixer::~OpenSoundDeviceMixer()
{
	CALLED();
	if (fFD != 0) {
		close(fFD);
	}
}

OpenSoundDeviceMixer::OpenSoundDeviceMixer(oss_mixerinfo *info)
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;
	memcpy(&fMixerInfo, info, sizeof(oss_mixerinfo));
	fFD = open(info->devnode, O_RDWR);
	if (fFD < 0) {
		fInitCheckStatus = errno;
		return;
	}
	fInitCheckStatus = B_OK;
}


status_t OpenSoundDeviceMixer::InitCheck(void) const
{
	CALLED();
	return fInitCheckStatus;
}

int OpenSoundDeviceMixer::CountExtInfos()
{
	int n;
	CALLED();
	
	// faster way!
	return Info()->nrext;
	
	// -1 doesn't work here (!?)
	n = Info()->dev;
	if (ioctl(fFD, SNDCTL_MIX_NREXT, &n, sizeof(int)) < 0) {
		PRINT(("OpenSoundDeviceMixer::CountExtInfos: SNDCTL_MIX_NREXT: %s\n", strerror(errno)));
		return 0;
	}
	
	return n;
}



status_t OpenSoundDeviceMixer::GetExtInfo(int index, oss_mixext *info)
{
	CALLED();
	
	if (!info)
		return EINVAL;
	
	// XXX: we should probably cache them as they might change on the fly
	
	info->dev = Info()->dev; // this mixer
	info->ctrl = index;
	if (ioctl(fFD, SNDCTL_MIX_EXTINFO, info, sizeof(*info)) < 0) {
		PRINT(("OpenSoundDeviceMixer::%s: SNDCTL_MIX_EXTINFO(%d): %s\n", __FUNCTION__, index, strerror(errno)));
		return errno;
	}
	
	return B_OK;
}


status_t OpenSoundDeviceMixer::GetMixerValue(oss_mixer_value *value)
{
	CALLED();
	
	if (!value)
		return EINVAL;
	
	value->dev = Info()->dev; // this mixer
	// should be set before calling
	//value->ctrl = index;
	//value->timestamp = 
	if (ioctl(fFD, SNDCTL_MIX_READ, value, sizeof(*value)) < 0) {
		PRINT(("OpenSoundDeviceMixer::%s: SNDCTL_MIX_READ(%d): %s\n", __FUNCTION__, value->ctrl, strerror(errno)));
		return errno;
	}
	
	return B_OK;
}


status_t OpenSoundDeviceMixer::SetMixerValue(oss_mixer_value *value)
{
	CALLED();
	
	if (!value)
		return EINVAL;
	
	value->dev = Info()->dev; // this mixer
	// should be set before calling
	//value->ctrl = index;
	//value->timestamp = 
	if (ioctl(fFD, SNDCTL_MIX_WRITE, value, sizeof(*value)) < 0) {
		PRINT(("OpenSoundDeviceMixer::%s: SNDCTL_MIX_WRITE(%d): %s\n", __FUNCTION__, value->ctrl, strerror(errno)));
		return errno;
	}
	
	return B_OK;
}


status_t OpenSoundDeviceMixer::GetEnumInfo(int index, oss_mixer_enuminfo *info)
{
	CALLED();
	
	if (!info)
		return EINVAL;
	
	info->dev = Info()->dev; // this mixer
	info->ctrl = index;
	if (ioctl(fFD, SNDCTL_MIX_ENUMINFO, info, sizeof(*info)) < 0) {
		PRINT(("OpenSoundDeviceMixer::%s: SNDCTL_MIX_ENUMINFO(%d): %s\n", __FUNCTION__, index, strerror(errno)));
		return errno;
	}
	
	return B_OK;
}


//update_counter


