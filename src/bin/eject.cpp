/*
 * Copyright (c) 2003-2011, Haiku Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		François Revol <mmu_man@users.sf.net>
 *		Philippe Houdoin
 *
 * Description: ejects physical media from a drive.
 *              This version also loads a media and can query for the status.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <device/scsi.h>
#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <Drivers.h>
#include <fs_info.h>
#include <ObjectList.h>
#include <Path.h>
#include <String.h>


class RemovableDevice {
public:
	RemovableDevice(BDiskDevice* device) {
		fName = device->Name();
		device->GetPath(&fPath);
	};

	inline const char* Name() 	{ return fName.String(); }
	inline const char* Path()	{ return fPath.Path(); }

private:
	BString	fName;
	BPath 	fPath;
};


class RemovableDeviceVisitor : public BDiskDeviceVisitor {
public:
	virtual bool Visit(BDiskDevice* device)	{
		if (device->IsRemovableMedia())
			fRemovableDevices.AddItem(new RemovableDevice(device));
		return false; // Don't stop yet!
	}

	virtual bool Visit(BPartition* partition, int32 level) { return false; }

	inline BObjectList<RemovableDevice>&	RemovableDevices() { return fRemovableDevices; }

private:
	BObjectList<RemovableDevice>	fRemovableDevices;
};


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
		if (strncmp(argv[i], "--h", 3) == 0) {
			return usage("eject");
		} else if (strncmp(argv[i], "-", 1) == 0) {
			if (strlen(argv[i]) > 1)
				operation = argv[i][1];
			else {
				usage("eject");
				return 1;
			}
		} else {
			device = argv[i];
			ret = do_eject(operation, device);
			if (ret != 0)
				return ret;
		}
	}
	if (device == NULL) {
		BDiskDeviceRoster diskDeviceRoster;
		RemovableDeviceVisitor visitor;
		diskDeviceRoster.VisitEachDevice(&visitor);

		int32 count = visitor.RemovableDevices().CountItems();
		if (count < 1) {
			printf("No removable device found!\n");
			return 1;
		}

		if (count > 1) {
			printf("Multiple removable devices available:\n");
			for (i = 0; i < count; i++) {
				RemovableDevice* item = visitor.RemovableDevices().ItemAt(i);
				printf("  %s\t\"%s\"\n", item->Path(), item->Name());
			}
			return 1;
		}

		// Default to single removable device found
		device = (char*)visitor.RemovableDevices().FirstItem()->Path();
		return do_eject(operation, device);
	}

	return 0;
}


static int do_eject(char operation, char *device)
{
	bool bval;
	int fd;
	status_t devstatus;
	fs_info info;

	// if the path is not on devfs, it's probably on
	// the mountpoint of the device we want to act on.
	// (should rather stat() for blk(char) device though).
	if (fs_stat_dev(dev_for_path(device), &info) >= B_OK) {
		if (strcmp(info.fsh_name, "devfs"))
			device = info.device_name;
	}

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

