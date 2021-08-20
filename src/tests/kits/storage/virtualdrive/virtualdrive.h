/* This software is part of the Haiku distribution and is covered
** by the MIT License.
*/
#ifndef VIRTUAL_DRIVE_H
#define VIRTUAL_DRIVE_H

#include <Drivers.h>

// virtualdrive device directory and control device, "/dev" relative
#define VIRTUAL_DRIVE_DIRECTORY_REL			"misc/virtualdrive"
#define VIRTUAL_DRIVE_CONTROL_DEVICE_REL	VIRTUAL_DRIVE_DIRECTORY_REL \
											"/control"
// virtualdrive device directory and control device, absolute
#define VIRTUAL_DRIVE_DIRECTORY				"/dev/" \
											VIRTUAL_DRIVE_DIRECTORY_REL
#define VIRTUAL_DRIVE_CONTROL_DEVICE		"/dev/" \
											VIRTUAL_DRIVE_CONTROL_DEVICE_REL

#define VIRTUAL_DRIVE_IOCTL_BASE	(B_DEVICE_OP_CODES_END + 10001)

enum {
	VIRTUAL_DRIVE_REGISTER_FILE	= VIRTUAL_DRIVE_IOCTL_BASE,
		// on control device: virtual_drive_info*, fills in device_name
	VIRTUAL_DRIVE_UNREGISTER_FILE,
		// on data device: none
	VIRTUAL_DRIVE_GET_INFO,
		// on data device: virtual_drive_info*
};

#define VIRTUAL_DRIVE_MAGIC	'VdIn'

typedef struct virtual_drive_info {
	uint32			magic;
	size_t			drive_info_size;
	char			file_name[B_PATH_NAME_LENGTH];
	char			device_name[B_PATH_NAME_LENGTH];
	device_geometry	geometry;
	bool			use_geometry;
	bool			halted;		// only valid for VIRTUAL_DRIVE_GET_INFO
} virtual_drive_info;

#endif	// VIRTUAL_DRIVE_H
