/* ++++++++++
	play.cpp
	Copyright (C) 1995 Be Incorporated.  All Rights Reserved.
	
	modified on November 03, 2003 by Jerome Duval
+++++ */
/*
 * was released as sample code; source:
 * ftp://planetmirror.com/raid/13/beos/samples/media_kit/obsolete/play.cpp
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <scsi.h>
#include <unistd.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

	
/* ----------
	play requested track 
----- */

void
play(int id, scsi_play_track *track)
{
	track->start_index = 1;
	track->end_track = 99;
	track->end_index = 1;
	if (!ioctl(id, B_SCSI_PLAY_TRACK, track))
		printf("Playing audio...\n");
	else
		printf("Play audio failed\n");
}


/* ----------
	show valid cd-rom id's
	(from /boot/optional/sample-code/interface_kit/CDButton/CDEngine.cpp)
----- */

static void
try_dir(const char *directory, long *count, bool show)
{ 
	bool add = false;
	BDirectory dir; 
	dir.SetTo(directory); 
	if(dir.InitCheck() != B_NO_ERROR) { 
		return; 
	} 
	dir.Rewind(); 
	BEntry entry; 
	while(dir.GetNextEntry(&entry) >= 0) { 
		BPath path; 
		const char *name; 
		entry_ref e; 
		
		if(entry.GetPath(&path) != B_NO_ERROR) 
			continue; 
		name = path.Path(); 
		
		if(entry.GetRef(&e) != B_NO_ERROR) 
			continue; 

		if(entry.IsDirectory()) { 
			if(strcmp(e.name, "floppy") == 0) 
				continue; // ignore floppy (it is not silent) 
			try_dir(name, count, show);
		} 
		else { 
			int devfd; 
			device_geometry g; 

			if(strcmp(e.name, "raw") != 0) 
				continue; // ignore partitions 

			devfd = open(name, O_RDONLY); 
			if(devfd < 0) 
				continue; 

			if(ioctl(devfd, B_GET_GEOMETRY, &g, sizeof(g)) >= 0) {
				if(g.device_type == B_CD)
				{ 
					add = true;
				}
			}
			close(devfd);
		}
		if (add) {
			*count += 1;
			if (show)
				printf("   %s\n", name);
		}
	}
}

/* ----------
	show valid cd-rom id's
----- */

int count_ids(bool show)
{
	long		count = 0;

	try_dir("/dev/disk", &count, show);
	return (count);
}


/* ----------
	main 
----- */

int
main (int argc, char **argv)
{
	int					id;
	int					i;
	int					length;
	int					start;
	long				command;
	long				req_track = 0;
	long				vol;
	long				index;
	ulong				frames;
	short				tmp;
	uchar				*buf;
	scsi_toc			toc;
	scsi_play_track		track;
	scsi_volume			volume;
	scsi_position		position;
	scsi_read_cd		read_cd;
	scsi_scan			scan;
	FILE				*f;

	length = count_ids(FALSE);
	if (!length) {
		printf("No CD-ROM drive present.\n");
		return 0;
	}
	errno = 0;

	if (argc > 2)
		command = strtol (argv[2], 0, 0);
	else
		command = 0;

	if ((argc < 2) || (errno) ||
		(command < 0) || (command > 8)) {
		printf("Usage:  play device [command [param]]\n\n");
		printf(" Valid devices:\n");
		count_ids(TRUE);
		printf("\n");
		printf(" Valid commands:\n");
		printf("   0 [n] - play from track n [1]\n");
		printf("   1     - pause\n");
		printf("   2     - resume\n");
		printf("   3     - stop\n");
		printf("   4     - eject\n");
		printf("   5 n   - set volume to n (0 <= n <= 255)\n");
		printf("   6     - current position\n");
		printf("   7 n s - save track n to file s\n");
		printf("   8 [c] - scan in direction c (f = forward, b = backward)\n\n");
		return 0;
	}

	if ((id = open(argv[1], 0)) >= 0) {
		switch (command) {
			case 0:
				if (!ioctl(id, B_SCSI_GET_TOC, &toc)) {
					track.start_track = 0;
					for (i = toc.toc_data[2]; i <= toc.toc_data[3]; i++) {
						length = (toc.toc_data[(((i + 1) - toc.toc_data[2]) * 8) + 4 + 5] * 60) + 
						 (toc.toc_data[(((i + 1) - toc.toc_data[2]) * 8) + 4 + 6]);
						length -= ((toc.toc_data[((i - toc.toc_data[2]) * 8) + 4 + 5] * 60) + 
						  (toc.toc_data[((i - toc.toc_data[2]) * 8) + 4 + 6]));
						printf(" Track %.2d: %.2d:%.2d - ", i, length / 60, length % 60);
						if (toc.toc_data[((i - toc.toc_data[2]) * 8) + 4 + 1] & 4)
							printf("DATA\n");
						else {
							printf("AUDIO\n");
							if (!track.start_track)
								track.start_track = i;
						}
					}
					if (track.start_track) {
						if (argc > 3) {
							req_track = strtol (argv[3], 0, 0);
							if ((req_track < toc.toc_data[2]) || (req_track > toc.toc_data[3]))
								printf("Requested track is out of range\n");
							else
							if (toc.toc_data[((req_track - toc.toc_data[2]) * 8) + 4 + 1] & 4)
								printf("Requested track is not an audio track\n");
							else {
								track.start_track = req_track;
								play(id, &track);
							}
						}
						else
							play(id, &track);
					}
					else
						printf("No audio tracks on CD\n");
				}
				else
					printf("Could not read table of contents on CD\n");
				break;

			case 1:
				printf("Pausing audio\n");
				ioctl(id, B_SCSI_PAUSE_AUDIO);
				break;

			case 2:
				printf("Resuming audio\n");
				ioctl(id, B_SCSI_RESUME_AUDIO);
				break;

			case 3:
				printf("Stopping audio\n");
				ioctl(id, B_SCSI_STOP_AUDIO);
				break;

			case 4:
				printf("Ejecting CD\n");
				ioctl(id, B_SCSI_EJECT);
				break;
				
			case 5:
				if (argc < 4)
					printf("Specify the volume\n");
				else {
					vol = strtol (argv[3], 0, 0);
					if ((vol < 0) || (vol > 255))
						printf("Volume is out of range (0 - 255)\n");
					else {
						printf("Setting volume to %ld\n", vol);
						volume.port0_volume = vol;
						volume.port1_volume = vol;
						volume.flags = B_SCSI_PORT0_VOLUME | B_SCSI_PORT1_VOLUME;
						ioctl(id, B_SCSI_SET_VOLUME, &volume);
					}
				}
				break;

			case 6:
				if (ioctl(id, B_SCSI_GET_POSITION, &position) == B_ERROR)
					printf("Could not get current position\n");
				else {
					switch(position.position[1]) {
						case 0x00:
							printf("Position not supported by device\n");
							break;

						case 0x11:
							printf("Playing track %d (%.2d:%.2d:%.2d)\n",
								position.position[6],
								position.position[9],
								position.position[10],
								position.position[11]);
							break;

						case 0x12:
							printf("Paused at track %d (%.2d:%.2d:%.2d)\n",
								position.position[6],
								position.position[9],
								position.position[10],
								position.position[11]);
							break;

						case 0x13:
							printf("Play has been completed\n");
							break;

						case 0x14:
							printf("Play stopped due to error\n");
							break;

						case 0x15:
							printf("No status to return\n");
							break;

						default:
							printf("Unexpected result: %.2x\n",
								position.position[1]);
					}
				}
				break;

			case 7:
				if (argc < 4) {
					printf("Specify the track to save\n");
					break;
				}

				if (argc < 5) {
					printf("Specify a file to save to\n");
					break;
				}

				f = fopen(argv[4], "w");
				if (f == NULL) {
					printf("Un-able to create %s\n", argv[4]);
					break;
				}

				req_track = strtol (argv[3], 0, 0);
				if (!ioctl(id, B_SCSI_GET_TOC, &toc)) {
					if (req_track > toc.toc_data[3]) {
						printf("Track %ld is out of range [%d-%d]\n", req_track,
								toc.toc_data[2], toc.toc_data[3]);
						break;
					}
					index = 0;
					while (toc.toc_data[4 + (index * 8) + 2] != req_track) {
						index++;
					}
					start = (toc.toc_data[4 + (index * 8) + 5] * 60 * 75) +
							(toc.toc_data[4 + (index * 8) + 6] * 75) +
							 toc.toc_data[4 + (index * 8) + 7];
					index++;
					length = ((toc.toc_data[4 + (index * 8) + 5] * 60 * 75) +
							  (toc.toc_data[4 + (index * 8) + 6] * 75) +
							   toc.toc_data[4 + (index * 8) + 7]) - start;

					frames = min_c(1 * 75, (int) length);
					read_cd.buffer = (char *)malloc(frames * 2352);

					printf("Saving track %ld to %s...\n", req_track, argv[4]);
					while (length) {
						index = start;
						read_cd.start_m = index / (60 * 75);
						index %= (60 * 75);
						read_cd.start_s = index / 75;
						index %= 75;
						read_cd.start_f = index;

						index = min_c((int)frames, length);
						read_cd.buffer_length = index * 2352;
						length -= index;
						start += index;

						read_cd.length_m = index / (60 * 75);
						index %= (60 * 75);
						read_cd.length_s = index / 75;
						index %= 75;
						read_cd.length_f = index;

						for (i = 0; i < 5; i++)
							if (ioctl(id, B_SCSI_READ_CD, &read_cd) == B_NO_ERROR)
								break;
						if (i == 5) {
							printf("Error reading CD-DA\n");
							break;
						}
						buf = (uchar *)read_cd.buffer;
						for (i = 0; i < read_cd.buffer_length; i += 2) {
							tmp = (short)((buf[1] << 8) | buf[0]);
							*(short *)(&buf[0]) = tmp;
							buf += 2;
						}
						fwrite(read_cd.buffer, read_cd.buffer_length, 1, f);
					}
					free(read_cd.buffer);
					fclose(f);
				}
				else
					printf("Failed to read table of contents\n");
				break;

			case 8:
				if (argc == 4) {
					if (!strcmp(argv[3], "f") || !strcmp(argv[3], "F"))
						scan.direction = 1;
					else if (!strcmp(argv[3], "b") || !strcmp(argv[3], "B"))
						scan.direction = -1;
					else {
						printf("Use 'f' to scan forward, 'b' to scan backwards\n");
						break;
					}
				}
				else
					scan.direction = 0;
				scan.speed = 0;
				if (ioctl(id, B_SCSI_SCAN, &scan) == B_ERROR)
					printf("Error trying to scan\n");
				else
					printf("Scanning...\n");
				break;
		}
		close(id);
	}
	else
		printf("CD player is empty or invalid scsi-id\n");
	return 0;
}
