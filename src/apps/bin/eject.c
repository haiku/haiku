// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered
//  by the OpenBeOS license.
//
//
//  File:        eject.c
//  Author:      François Revol (mmu_man@users.sf.net)
//  Description: ejects physical media from a drive.
//               This version also loads a media and can query for the status.
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <Drivers.h>

int usage(char *prog)
{
	printf("usage: eject [-q|-l] /dev/disk/.../raw\n");
	printf("	eject the device, or:\n");
	printf("	-l: load it (close the tray)\n");
	printf("	-q: query for media status\n");
	return 0;
}

int main(int argc, char **argv)
{
	char *device = "/dev/disk/floppy/raw";
	char operation = 'e';
	int fd, i;
	status_t devstatus;
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-", 1) == 0) {
			if (strlen(argv[i]) > 1)
				operation = argv[i][1];
			else {
				usage(argv[0]);
				return 1;
			}
		} else if (strncmp(argv[i], "--h", 2) == 0)
			return usage(argv[0]);
		else {
			device = argv[i];
			break;
		}
	}
	fd = open(device, O_RDONLY);
	if (fd < 0) {
		perror(device);
		return 1;
	}
	switch (operation) {
	case 'h':
		return usage(argv[0]);
	case 'e':
		if (ioctl(fd, B_EJECT_DEVICE) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'l':
		if (ioctl(fd, B_LOAD_MEDIA) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'q':
		if (ioctl(fd, B_GET_MEDIA_STATUS, &devstatus) < 0) {
			perror(device);
			return 1;
		}
		switch (devstatus) {
		case B_NO_ERROR:
			puts("Media present");
			break;
		default:
			puts(strerror(devstatus));
		}
		break;
	default:
		usage(argv[0]);
		return 1;
	}
	close(fd);
	return 0;
}

