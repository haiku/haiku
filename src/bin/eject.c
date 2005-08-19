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
#include <device/scsi.h>

static int usage(char *prog)
{
	printf("usage: eject [-q|-l|-s|-b|-u] /dev/disk/.../raw\n");
//	printf("usage: eject [-q|-l|-s|-b|-u] [scsi|ide|/dev/disk/.../raw]\n");
	printf("	eject the device, or:\n");
	printf("	-l: load it (close the tray)\n");
	printf("	-q: query for media status\n");
	printf("	-s: swap tray position (close/eject)\n");
	printf("	-b: block (lock) tray position (prevent close/eject)\n");
	printf("	-u: unblock (unlock) tray position (allow close/eject)\n");
//	printf("	scsi: act on all scsi devices\n");
//	printf("	ide:  act on all ide devices\n");
//	printf("	acts on all scsi and ide devices and floppy by default\n");
	return 0;
}

static int do_eject(char operation, char *device);

int main(int argc, char **argv)
{
	char *device = NULL;
	char operation = 'e';
	int i;
	int ret;
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-", 1) == 0) {
			if (strlen(argv[i]) > 1)
				operation = argv[i][1];
			else {
				usage("eject");
				return 1;
			}
		} else if (strncmp(argv[i], "--h", 2) == 0)
			return usage("eject");
		else {
			device = argv[i];
			ret = do_eject(operation, device);
			if (ret != 0)
				return ret;
		}
	}
	if (device == NULL)
		return do_eject(operation, "/dev/disk/floppy/raw");
}
static int do_eject(char operation, char *device)
{
	bool bval;
	int fd;
	status_t devstatus;
	fd = open(device, O_RDONLY);
	if (fd < 0) {
		perror(device);
		return 1;
	}
	switch (operation) {
	case 'h':
		return usage("eject");
	case 'e':
		if (ioctl(fd, B_EJECT_DEVICE, NULL, 0) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'l':
		if (ioctl(fd, B_LOAD_MEDIA, NULL, 0) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'b':
		bval = true;
		if (ioctl(fd, B_SCSI_PREVENT_ALLOW, &bval, sizeof(bval)) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'u':
		bval = false;
		if (ioctl(fd, B_SCSI_PREVENT_ALLOW, &bval, sizeof(bval)) < 0) {
			perror(device);
			return 1;
		}
		break;
	case 'q':
		if (ioctl(fd, B_GET_MEDIA_STATUS, &devstatus, sizeof(devstatus)) < 0) {
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
	case 's':
		if (ioctl(fd, B_GET_MEDIA_STATUS, &devstatus, sizeof(devstatus)) < 0) {
			perror(device);
			return 1;
		}
		switch (devstatus) {
		case B_NO_ERROR:
		case B_DEV_NO_MEDIA:
			if (ioctl(fd, B_EJECT_DEVICE, NULL, 0) < 0) {
				perror(device);
				return 1;
			}
			break;
		case B_DEV_DOOR_OPEN:
			if (ioctl(fd, B_LOAD_MEDIA, NULL, 0) < 0) {
				perror(device);
				return 1;
			}
			break;
		default:
			perror(device);
		}
		break;
	default:
		usage("eject");
		return 1;
	}
	close(fd);
	return 0;
}

