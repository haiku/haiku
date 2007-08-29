/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 */

#ifndef _OPENSOUNDDEVICEMIXER_H
#define _OPENSOUNDDEVICEMIXER_H

#include "OpenSoundDevice.h"

class OpenSoundDeviceMixer
{
	public:
		OpenSoundDeviceMixer(oss_mixerinfo *info);
		virtual ~OpenSoundDeviceMixer(void);

		virtual status_t InitCheck(void) const;
		int			FD() const { return fFD; };
		const oss_mixerinfo *Info() const { return &fMixerInfo; };
		
		int				CountExtInfos();
		status_t		GetExtInfo(int index, oss_mixext *info);
		status_t		GetMixerValue(oss_mixer_value *value);
		status_t		SetMixerValue(oss_mixer_value *value);
		status_t		GetEnumInfo(int index, oss_mixer_enuminfo *info);
		
		
		
		//
		int				CachedUpdateCounter(int index);
		void			SetCachedUpdateCounter(int index, int counter);
		
		/*
		status_t		Get(oss_ *info);
		status_t		Get(oss_ *info);
		*/
		
	private:
		status_t 				fInitCheckStatus;
		oss_mixerinfo			fMixerInfo;
		int						fFD;
};

#endif
