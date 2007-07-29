/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */
#ifndef CDAUDIODEVICE_H
#define CDAUDIODEVICE_H

#include <List.h>
#include <String.h>
#include <SupportDefs.h>


// The SCSI table of contents consists of a 4-byte header followed by 100 track
// descriptors, which are each 8 bytes. We don't really need the first 5 bytes
// of the track descriptor, so we'll just ignore them. All we really want is the
// fLength of each track, which happen to be the last 3 bytes of the descriptor.
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
	kNoCD = 0,
	kStopped,
	kPaused,
	kPlaying,
	kSkipping,
	kError,
	kInit
};


class CDAudioTime {
	public:
		CDAudioTime(const int32 min = -1, const int32 &sec = -1);
		CDAudioTime(const CDAudioTime &from);
		CDAudioTime &operator=(const CDAudioTime &from);
		CDAudioTime operator+(const CDAudioTime &from);
		CDAudioTime operator-(const CDAudioTime &from);

		void SetMinutes(const int32& minutes)
		{
			fMinutes = minutes;
		}

		void SetSeconds(const int32& seconds)
		{
			fSeconds = seconds;
		}

		int32 GetMinutes() const
		{
			return fMinutes;
		}

		int32 GetSeconds() const
		{
			return fSeconds;
		}

	private:
		int32	fMinutes;
		int32	fSeconds;
};


class CDAudioData {
	public:
		CDAudioData(const int32 &id = -1, const int32 &count = -1, 
			const int32 &discLength = -1);
		CDAudioData(const CDAudioData &from);
		CDAudioData &operator=(const CDAudioData &from);

	private:
		int32	fDiscId;
		int32	fTrackCount;
		int32	fLength;
		BString fFrameOffsets;
};


class CDAudioDevice {
	public:
		CDAudioDevice();
		~CDAudioDevice();

		bool Play(const int16 &track);
		bool Pause();
		bool Resume();
		bool Stop();
		bool Eject();

		bool StartFastFwd();
		bool StopFastFwd();

		bool StartRewind();
		bool StopRewind();

		bool SetVolume(uint8 value);
		uint8 GetVolume();

		CDState GetState();

		int16 CountTracks();
		int16 GetTrack();

		uint8 CountDrives();
		bool SetDrive(const int32 &drive);
		const char* GetDrivePath() const;

		int32 GetDrive() const 
		{
			return fDriveIndex;
		}

		bool GetTime(CDAudioTime &track, CDAudioTime &disc);
		bool GetTimeForTrack(const int16 &index, CDAudioTime &track);
		bool GetTimeForDisc(CDAudioTime &disc);
		int32 GetDiscID();

		bool IsDataTrack(const int16 &track);

	private:
		int32 _FindDrives(const char *path);

		int			fFileHandle;
		BString*	fDrivePath;
		BList 		fDriveList;
		int32		fDriveIndex;
};

#endif	// CDAUDIODEVICE_H
