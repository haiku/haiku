/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm, bpmagic@columbus.rr.com
 */


#include "CDAudioDevice.h"
#include "scsi.h"

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct ConvertedToc {
	int32 min;
	int32 sec;
	int32 frame;
};


static int32
cddb_sum(int n)
{
	char buf[12];
	int32 ret = 0;
	
	sprintf(buf, "%u", n);
	for (const char *p = buf; *p != '\0'; p++)
		ret += (*p - '0');
	return ret;
}


//	#pragma mark -


CDAudioData::CDAudioData(const int32 &id, const int32 &count,
	const int32 &discLength)
	:
	fDiscId(id),
	fTrackCount(count),
	fLength(discLength)
{
}


CDAudioData::CDAudioData(const CDAudioData &from)
	:
	fDiscId(from.fDiscId),
	fTrackCount(from.fTrackCount),
	fLength(from.fLength)
{
}


CDAudioData &
CDAudioData::operator=(const CDAudioData &from)
{
	fDiscId = from.fDiscId;
	fTrackCount = from.fTrackCount;
	fLength = from.fLength;
	fFrameOffsets = from.fFrameOffsets;
	return *this;
}


//	#pragma mark -


CDAudioTime::CDAudioTime(const int32 min,const int32 &sec)
	:
	fMinutes(min),
	fSeconds(sec)
{
}


CDAudioTime::CDAudioTime(const CDAudioTime &from)
	:
	fMinutes(from.fMinutes),
	fSeconds(from.fSeconds)
{
}


CDAudioTime &
CDAudioTime::operator=(const CDAudioTime &from)
{
	fMinutes = from.fMinutes;
	fSeconds = from.fSeconds;
	return *this;
}


CDAudioTime
CDAudioTime::operator+(const CDAudioTime &from)
{
	CDAudioTime time;

	time.fMinutes = fMinutes + from.fMinutes;
	time.fSeconds = fSeconds + from.fSeconds;

	while (time.fSeconds > 59) {
		time.fMinutes++;
		time.fSeconds -= 60;
	}
	return time;
}


CDAudioTime
CDAudioTime::operator-(const CDAudioTime &from)
{
	CDAudioTime time;

	int32 tsec = ((fMinutes * 60) + fSeconds) - ((from.fMinutes * 60)
		+ from.fSeconds);
	if (tsec < 0) {
		time.fMinutes = 0;
		time.fSeconds = 0;
		return time;
	}

	time.fMinutes = tsec / 60;
	time.fSeconds = tsec % 60;

	return time;
}


//	#pragma mark -


CDAudioDevice::CDAudioDevice()
{
	_FindDrives("/dev/disk");
	if (CountDrives() > 0)
		SetDrive(0);
}


CDAudioDevice::~CDAudioDevice()
{
	for (int32 i = 0; i < fDriveList.CountItems(); i++)
		delete (BString*) fDriveList.ItemAt(i);
}



//!	This plays only one track - the track specified
bool
CDAudioDevice::Play(const int16 &track)
{
	if (GetState() == kNoCD) {
		// no CD available, bail out
		ioctl(fFileHandle, B_LOAD_MEDIA, 0, 0);
		return false;
	}

	scsi_play_track playtrack;

	playtrack.start_track = track;
	playtrack.start_index = 1;
	playtrack.end_track = track;
	playtrack.end_index = 1;

	status_t result = ioctl(fFileHandle, B_SCSI_PLAY_TRACK, &playtrack);
	if (result != B_OK) {
		printf("Couldn't play track: %s\n", strerror(errno));
		return false;
	}
	
	return true;
}


bool
CDAudioDevice::Pause()
{
	status_t result = ioctl(fFileHandle, B_SCSI_PAUSE_AUDIO);
	if (result != B_OK) {
		printf("Couldn't pause track: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::Resume()
{
	CDState state = GetState();
	if (state == kNoCD) {
		// no CD available, bail out
		ioctl(fFileHandle, B_LOAD_MEDIA, 0, 0);
		return false;
	} else {
		if (state == kStopped)
			return Play(0);
	}

	status_t result = ioctl(fFileHandle, B_SCSI_RESUME_AUDIO);
	if (result != B_OK) {
		printf("Couldn't resume track: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::Stop()
{
	status_t result = ioctl(fFileHandle, B_SCSI_STOP_AUDIO);
	if (result != B_OK) {
		printf("Couldn't stop CD: %s\n", strerror(errno));
		return false;
	}
	return true;
}


//!	Open or close the CD tray
bool
CDAudioDevice::Eject()
{
	status_t media_status = B_DEV_NO_MEDIA;

	// get the status first
	ioctl(fFileHandle, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));

	// if door open, load the media, else eject the CD
	status_t result = ioctl(fFileHandle,
		media_status == B_DEV_DOOR_OPEN ? B_LOAD_MEDIA : B_EJECT_DEVICE);

	if (result != B_OK) {
		printf("Couldn't eject CD: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::StartFastFwd()
{
	scsi_scan scan;
	scan.direction = 1;
	scan.speed = 1;
	status_t result = ioctl(fFileHandle, B_SCSI_SCAN, &scan);
	if (result != B_OK)	{
		printf("Couldn't fast forward: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::StopFastFwd()
{
	scsi_scan scan;
	scan.direction = 0;
	scan.speed = 1;
	status_t result = ioctl(fFileHandle, B_SCSI_SCAN, &scan);
	if (result != B_OK)	{
		printf("Couldn't stop fast forwarding: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::StartRewind()
{
	scsi_scan scan;
	scan.direction = -1;
	scan.speed = 1;
	status_t result = ioctl(fFileHandle, B_SCSI_SCAN, &scan);
	if (result != B_OK)	{
		printf("Couldn't rewind: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::StopRewind()
{
	scsi_scan scan;
	scan.direction = 0;
	scan.speed = 1;
	status_t result = ioctl(fFileHandle, B_SCSI_SCAN, &scan);
	if (result != B_OK)	{
		printf("Couldn't stop rewinding: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::SetVolume(uint8 value)
{
	scsi_volume vol;

	// change only port0's volume
	vol.flags = 2;
	vol.port0_volume = value;

	status_t result = ioctl(fFileHandle, B_SCSI_SET_VOLUME, &vol);
	if (result != B_OK)	{
		printf("Couldn't set volume: %s\n", strerror(errno));
		return false;
	}
	return true;
}


uint8
CDAudioDevice::GetVolume()
{
	scsi_volume vol;
	ioctl(fFileHandle, B_SCSI_GET_VOLUME, &vol);
	return vol.port0_volume;
}


//! Check the current CD play state
CDState
CDAudioDevice::GetState()
{
	scsi_position pos;
	status_t media_status = B_DEV_NO_MEDIA;

	ioctl(fFileHandle, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_OK)
		return kNoCD;

	status_t result = ioctl(fFileHandle, B_SCSI_GET_POSITION, &pos);
	if (result != B_OK)
		return kNoCD;
	else if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
		   ((pos.position[1] == 0x12) && (!pos.position[6])))
		return kStopped;
	else if (pos.position[1] == 0x11)
		return kPlaying;
	else
		return kPaused;
}


int16
CDAudioDevice::CountTracks()
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);

	if (result != B_OK)
		return -1;

	return toc.toc_data[3];
}


//! Get the 0-based index of the current track
int16
CDAudioDevice::GetTrack()
{
	scsi_position pos;

	status_t media_status = B_DEV_NO_MEDIA;

	ioctl(fFileHandle, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_OK)
		return -1;

	status_t result = ioctl(fFileHandle, B_SCSI_GET_POSITION, &pos);
	if (result != B_OK)
		return -1;

	if (!pos.position[1] || pos.position[1] >= 0x13
		|| (pos.position[1] == 0x12 && !pos.position[6]))
		return 0;
	else
		return pos.position[6];
}


uint8
CDAudioDevice::CountDrives()
{
	return fDriveList.CountItems();
}


bool
CDAudioDevice::SetDrive(const int32 &drive)
{
	BString *path = (BString*) fDriveList.ItemAt(drive);

	if (!path)
		return false;

	int device = open(path->String(), O_RDONLY);
	if (device >= 0) {
		fFileHandle = device;
		fDrivePath = path;
		fDriveIndex = drive;
		return true;
	}
	
	return false;
}


const char *
CDAudioDevice::GetDrivePath() const
{
	if (!fDrivePath)
		return NULL;

	return fDrivePath->String();
}


int32
CDAudioDevice::_FindDrives(const char *path)
{
	BDirectory dir(path);

	if (dir.InitCheck() != B_OK)
		return B_ERROR; 

	dir.Rewind();

	BEntry entry; 
	while (dir.GetNextEntry(&entry) >= 0) {
		BPath path;
		const char *name;
		entry_ref e;

		if (entry.GetPath(&path) != B_OK)
			continue;

		name = path.Path();
		if (entry.GetRef(&e) != B_OK)
			continue;

		if (entry.IsDirectory()) {
			// ignore floppy -- it is not silent
			if (strcmp(e.name, "floppy") == 0)
				continue;
			else if (strcmp(e.name, "ata") == 0)
				continue;

			// Note that if we check for the count here, we could
			// just search for one drive. However, we want to find *all* drives
			// that are available, so we keep searching even if we've found one
			_FindDrives(name);

		} else {
			int devfd;
			device_geometry g;

			// ignore partitions
			if (strcmp(e.name, "raw") != 0)
				continue;

			devfd = open(name, O_RDONLY);
			if (devfd < 0)
				continue;

			if (ioctl(devfd, B_GET_GEOMETRY, &g, sizeof(g)) >= 0) {
				if (g.device_type == B_CD)
					fDriveList.AddItem(new BString(name));
			}
			close(devfd);
		}
	}
	return fDriveList.CountItems();
}


bool
CDAudioDevice::GetTime(CDAudioTime &track, CDAudioTime &disc)
{
	scsi_position pos;

	// Sanity check
	status_t media_status = B_DEV_NO_MEDIA;
	ioctl(fFileHandle, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	if (media_status != B_OK)
		return false;

	status_t result = ioctl(fFileHandle, B_SCSI_GET_POSITION, &pos);

	if (result != B_OK)
		return false;

	if ((!pos.position[1]) || (pos.position[1] >= 0x13) ||
		((pos.position[1] == 0x12) && (!pos.position[6]))) {
		// This indicates that we have a CD, but we are stopped.
		return false;
	}

	disc.SetMinutes(pos.position[9]);
	disc.SetSeconds(pos.position[10]);
	track.SetMinutes(pos.position[13]);
	track.SetSeconds(pos.position[14]);
	return true;
}


bool
CDAudioDevice::GetTimeForTrack(const int16 &index, CDAudioTime &track)
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);

	if (result != B_OK)
		return false;

	int16 trackcount = toc.toc_data[3] - toc.toc_data[2] + 1;

	if (index < 1 || index > trackcount)
		return false;

	TrackDescriptor *desc = (TrackDescriptor*)&(toc.toc_data[4]);

	int32 tracktime = (desc[index].min * 60) + desc[index].sec;

	tracktime -= (desc[index - 1].min * 60) + desc[index - 1].sec;
	track.SetMinutes(tracktime / 60);
	track.SetSeconds(tracktime % 60);
	
	return true;
}


bool
CDAudioDevice::GetTimeForDisc(CDAudioTime &disc)
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);

	if (result != B_OK)
		return false;

	int16 trackcount = toc.toc_data[3] - toc.toc_data[2] + 1;
	TrackDescriptor *desc = (TrackDescriptor*)&(toc.toc_data[4]);

	disc.SetMinutes(desc[trackcount].min);
	disc.SetSeconds(desc[trackcount].sec);

	return true;
}


int32
CDAudioDevice::GetDiscID()
{
	// Read the disc
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);

	if (result != B_OK)
		return -1;


	int32 id, numTracks;
	BString frameOffsetsString;

	ConvertedToc tocData[100];

	// figure out the disc ID
	for (int index = 0; index < 100; index++) {
		tocData[index].min = toc.toc_data[9 + 8 * index];
		tocData[index].sec = toc.toc_data[10 + 8 * index];
		tocData[index].frame = toc.toc_data[11 + 8 * index];
	}
	numTracks = toc.toc_data[3] - toc.toc_data[2] + 1;

	int32 sum1 = 0;
	int32 sum2 = 0;
	for (int index = 0; index < numTracks; index++) {
		sum1 += cddb_sum((tocData[index].min * 60) + tocData[index].sec);
		
		// the following is probably running over too far
		sum2 +=	(tocData[index + 1].min * 60 + tocData[index + 1].sec) -
			(tocData[index].min * 60 + tocData[index].sec);
	}
	id = ((sum1 % 0xff) << 24) + (sum2 << 8) + numTracks;

	return id;
}


bool
CDAudioDevice::IsDataTrack(const int16 &track)
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);

	if (result != B_OK)
		return false;

	TrackDescriptor *trackindex = (TrackDescriptor*) &(toc.toc_data[4]);
	if (track > toc.toc_data[3])
		return false;

	// At least under R5, the SCSI CD drive has each legitimate audio track
	// have a value of 0x10. Data tracks have a value of 0x14;
	if (trackindex[track].adr_control & 4)
		return true;

	return false;
}
