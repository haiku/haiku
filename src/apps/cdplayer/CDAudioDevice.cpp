/*
 * Copyright (c) 2006-2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#include "CDAudioDevice.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <String.h>
#include <string.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <errno.h>
#include "scsi.h"


cdaudio_data::cdaudio_data(const int32 &id, const int32 &count, 
							const int32 &disclength)
 :	disc_id(id),
 	track_count(count),
 	length(disclength)
{
}


cdaudio_data::cdaudio_data(const cdaudio_data &from)
 :	disc_id(from.disc_id),
 	track_count(from.track_count),
 	length(from.length)
{
}


cdaudio_data &
cdaudio_data::operator=(const cdaudio_data &from)
{
	disc_id = from.disc_id;
	track_count = from.track_count;
	length = from.length;
	frame_offsets = from.frame_offsets;
	return *this;
}


cdaudio_time::cdaudio_time(const int32 min,const int32 &sec)
 :	minutes(min),
 	seconds(sec)
{
}


cdaudio_time::cdaudio_time(const cdaudio_time &from)
 :	minutes(from.minutes),
 	seconds(from.seconds)
{
}


cdaudio_time &
cdaudio_time::operator=(const cdaudio_time &from)
{
	minutes = from.minutes;
	seconds = from.seconds;
	return *this;
}


cdaudio_time
cdaudio_time::operator+(const cdaudio_time &from)
{
	cdaudio_time time;
	
	time.minutes = minutes + from.minutes;
	time.seconds = seconds + from.seconds;
	
	while (time.seconds > 59) {
		time.minutes++;
		time.seconds-=60;
	}
	return time;
}


cdaudio_time
cdaudio_time::operator-(const cdaudio_time &from)
{
	cdaudio_time time;
	
	int32 tsec = ((minutes * 60) + seconds) - ((from.minutes * 60) + from.seconds);
	if (tsec < 0) {
		time.minutes = 0;
		time.seconds = 0;
		return time;
	}
	
	time.minutes = tsec / 60;
	time.seconds = tsec % 60;
	
	return time;
}


CDAudioDevice::CDAudioDevice(void)
{
	FindDrives("/dev/disk");
	if (CountDrives() > 0)
		SetDrive(0);
}


CDAudioDevice::~CDAudioDevice(void)
{
	for (int32 i = 0; i < fDriveList.CountItems(); i++)
		delete (BString*) fDriveList.ItemAt(i);
}


// This plays only one track - the track specified
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
CDAudioDevice::Pause(void)
{
	status_t result = ioctl(fFileHandle, B_SCSI_PAUSE_AUDIO);
	if (result != B_OK) {
		printf("Couldn't pause track: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::Resume(void)
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
CDAudioDevice::Stop(void)
{
	status_t result = ioctl(fFileHandle, B_SCSI_STOP_AUDIO);
	if (result != B_OK) {
		printf("Couldn't stop CD: %s\n", strerror(errno));
		return false;
	}
	return true;
}


// open or close the CD tray
bool
CDAudioDevice::Eject(void)
{
	status_t media_status = B_DEV_NO_MEDIA;
	
	// get the status first
	ioctl(fFileHandle, B_GET_MEDIA_STATUS, &media_status, sizeof(media_status));
	
	// if door open, load the media, else eject the CD
	status_t result = ioctl(fFileHandle,
							media_status == B_DEV_DOOR_OPEN ? 
											B_LOAD_MEDIA : B_EJECT_DEVICE);
	
	if (result != B_OK) {
		printf("Couldn't eject CD: %s\n", strerror(errno));
		return false;
	}
	return true;
}


bool
CDAudioDevice::StartFastFwd(void)
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
CDAudioDevice::StopFastFwd(void)
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
CDAudioDevice::StartRewind(void)
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
CDAudioDevice::StopRewind(void)
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
	vol.flags=2;
	vol.port0_volume=value;
	
	status_t result = ioctl(fFileHandle,B_SCSI_SET_VOLUME,&vol);
	if (result != B_OK)	{
		printf("Couldn't set volume: %s\n", strerror(errno));
		return false;
	}
	return true;
}


uint8
CDAudioDevice::GetVolume(void)
{
	scsi_volume vol;
	ioctl(fFileHandle, B_SCSI_GET_VOLUME, &vol);
	return vol.port0_volume;
}


// check the current CD play state
CDState
CDAudioDevice::GetState(void)
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
CDAudioDevice::CountTracks(void)
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);
	
	if (result != B_OK)
		return -1;
	
	return toc.toc_data[3];
}


// Get the 0-based index of the current track
int16
CDAudioDevice::GetTrack(void)
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
CDAudioDevice::CountDrives(void)
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
CDAudioDevice::GetDrivePath(void) const
{
	if (!fDrivePath)
		return NULL;
	
	return fDrivePath->String();
}


int32
CDAudioDevice::FindDrives(const char *path)
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
			else
			if (strcmp(e.name, "ata") == 0)
				continue;
			
			// Note that if we check for the count here, we could
			// just search for one drive. However, we want to find *all* drives
			// that are available, so we keep searching even if we've found one
			FindDrives(name);
			
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
CDAudioDevice::GetTime(cdaudio_time &track, cdaudio_time &disc)
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
	
	disc.minutes = pos.position[9];
	disc.seconds = pos.position[10];
	track.minutes = pos.position[13];
	track.seconds = pos.position[14];
	return true;
}


bool
CDAudioDevice::GetTimeForTrack(const int16 &index, cdaudio_time &track)
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
	
	tracktime -= (desc[index-1].min * 60) + desc[index-1].sec;
	track.minutes = tracktime / 60;
	track.seconds = tracktime % 60;
	
	return true;
}


bool
CDAudioDevice::GetTimeForDisc(cdaudio_time &disc)
{
	scsi_toc toc;
	status_t result = ioctl(fFileHandle, B_SCSI_GET_TOC, &toc);
	
	if (result != B_OK)
		return false;
	
	int16 trackcount = toc.toc_data[3] - toc.toc_data[2] + 1;
	TrackDescriptor *desc = (TrackDescriptor*)&(toc.toc_data[4]);
	
	disc.minutes = desc[trackcount].min;
	disc.seconds = desc[trackcount].sec;
	
	return true;
}


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


int32
CDAudioDevice::GetDiscID(void)
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
		tocData[index].min = toc.toc_data[9 + 8*index];
		tocData[index].sec = toc.toc_data[10 + 8*index];
		tocData[index].frame = toc.toc_data[11 + 8*index];
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

