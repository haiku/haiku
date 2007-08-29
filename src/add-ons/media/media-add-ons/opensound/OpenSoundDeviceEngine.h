/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 */

#ifndef _OPENSOUNDDEVICEENGINE_H
#define _OPENSOUNDDEVICEENGINE_H

#include "OpenSoundDevice.h"

class OpenSoundDeviceEngine
{
	public:
		OpenSoundDeviceEngine(oss_audioinfo *info);
		virtual ~OpenSoundDeviceEngine(void);

		virtual status_t InitCheck(void) const;
		int			FD() const { return fFD; };
		const oss_audioinfo *Info() const { return &fAudioInfo; };

		status_t	Open(int mode/* OPEN_* */);
		status_t	Close();
		
		//BDadaIO...
virtual	ssize_t		Read(void *buffer, size_t size);
virtual	ssize_t		Write(const void *buffer, size_t size);

		
		/* chain of engines for the same physical in/out */
		OpenSoundDeviceEngine *NextPlay() const { return fNextPlay; };
		OpenSoundDeviceEngine *NextRec() const { return fNextRec; };
		
		int			OpenMode() const { return fOpenMode; };
		bool		InUse() const { return (fOpenMode != 0); };
		
		status_t	UpdateInfo();
		// shortcuts
		int			Caps() const { return fAudioInfo.caps; };
		int			Latency() const { return fAudioInfo.latency; };
		
		
		int			GetChannels(void);
		status_t	SetChannels(int chans);

		int			GetFormat(void);
		status_t	SetFormat(int fmt);

		int			GetSpeed(void);
		status_t	SetSpeed(int speed);
		
		void		GetMediaFormat(media_format &format);
		
		size_t		GetISpace(audio_buf_info *info=NULL);
		size_t		GetOSpace(audio_buf_info *info=NULL);

		int			GetODelay(void);

		status_t	StartRecording(void);
		
		int64		PlayedFramesCount(void);
		bigtime_t	PlayedRealTime(void);
		
		// suggest possibles
		status_t	WildcardFormatFor(int fmt, media_format &format, bool rec=false);
		// suggest best
		status_t	PreferredFormatFor(int fmt, media_format &format, bool rec=false);
		// try this format
		status_t	AcceptFormatFor(int fmt, media_format &format, bool rec=false);
		// apply this format
		status_t	SpecializeFormatFor(int fmt, media_format &format, bool rec=true);
		
	private:
		status_t 				fInitCheckStatus;
		oss_audioinfo			fAudioInfo;
friend class OpenSoundAddOn;
		OpenSoundDeviceEngine	*fNextPlay;
		OpenSoundDeviceEngine	*fNextRec;
		int						fOpenMode; // OPEN_*
		int						fFD;
		media_format			fMediaFormat;
		int64					fPlayedFramesCount;
		bigtime_t				fPlayedRealTime;
};

#endif
