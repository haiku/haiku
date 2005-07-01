#ifndef CDAUDIODEVICE_H
#define CDAUDIODEVICE_H

#include <SupportDefs.h>
#include <String.h>
#include <List.h>

enum CDState {
	kNoCD=0,
	kStopped,
	kPaused,
	kPlaying,
	kSkipping
};

typedef struct
{
	int32 minutes;
	int32 seconds;
} cdaudio_time;

typedef struct
{
	int32 disc_id;
	int32 track_count;
	int32 length;
	
	BString frame_offsets;
} cdaudio_data;

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
	
	bool			GetTime(cdaudio_time &track, cdaudio_time &disc);
	int32			GetDiscID(void);
	
private:
	int32			FindDrives(const char *path);
	
	int				fFileHandle;
	BString *		fDrivePath;
	BList 			fDriveList;
};

#endif
