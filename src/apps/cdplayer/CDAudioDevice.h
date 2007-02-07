/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef CDAUDIODEVICE_H
#define CDAUDIODEVICE_H

#include <SupportDefs.h>
#include <String.h>
#include <List.h>

// The SCSI table of contents consists of a 4-byte header followed by 100 track
// descriptors, which are each 8 bytes. We don't really need the first 5 bytes
// of the track descriptor, so we'll just ignore them. All we really want is the
// length of each track, which happen to be the last 3 bytes of the descriptor.
typedef struct {
	uint8	reserved;
	uint8	adr_control;	// bytes 0-3 are control, 4-7 are ADR
	uint8	track_number;
	uint8	reserved2;
	
	uint8	reserved3;
	uint8	min;
	uint8	sec;
	uint8	frame;
} TrackDescriptor;

enum CDState {
	kNoCD=0,
	kStopped,
	kPaused,
	kPlaying,
	kSkipping,
	kError,
	kInit
};

class cdaudio_time
{
public:
			cdaudio_time(const int32 min=-1,const int32 &sec=-1);
			cdaudio_time(const cdaudio_time &from);
			cdaudio_time &operator=(const cdaudio_time &from);
			cdaudio_time operator+(const cdaudio_time &from);
			cdaudio_time operator-(const cdaudio_time &from);
	
	int32	minutes;
	int32	seconds;
};

class cdaudio_data
{
public:
			cdaudio_data(const int32 &id = -1, const int32 &count = -1, 
						const int32 &disclength = -1);
			cdaudio_data(const cdaudio_data &from);
			cdaudio_data &operator=(const cdaudio_data &from);
	
	int32	disc_id;
	int32	track_count;
	int32	length;
	
	BString frame_offsets;
};

class CDAudioDevice
{
public:
					CDAudioDevice(void);
					~CDAudioDevice(void);
	
	bool			Play(const int16 &track);
	bool			Pause(void);
	bool			Resume(void);
	bool			Stop(void);
	bool			Eject(void);
	
	bool			StartFastFwd(void);
	bool			StopFastFwd(void);
	
	bool			StartRewind(void);
	bool			StopRewind(void);
	
	bool			SetVolume(uint8 value);
	uint8			GetVolume(void);
	
	CDState			GetState(void);
	
	int16			CountTracks(void);
	int16			GetTrack(void);
	
	uint8			CountDrives(void);
	bool			SetDrive(const int32 &drive);
	const char *	GetDrivePath(void) const;
	int32			GetDrive(void) const { return fDriveIndex; }
	
	bool			GetTime(cdaudio_time &track, cdaudio_time &disc);
	bool			GetTimeForTrack(const int16 &index, cdaudio_time &track);
	bool			GetTimeForDisc(cdaudio_time &disc);
	int32			GetDiscID(void);
	
	bool			IsDataTrack(const int16 &track);
	
private:
	int32			FindDrives(const char *path);
	
	int				fFileHandle;
	BString *		fDrivePath;
	BList 			fDriveList;
	int32			fDriveIndex;
};

#endif
