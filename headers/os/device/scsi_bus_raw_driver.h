/*
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_BUS_RAW_DRIVER_H
#define _SCSI_BUS_RAW_DRIVER_H


/*!	Part of Open SCSI bus manager

	Devfs entry for raw bus access.

	This interface will go away. It's used by scsi_probe as
	long as we have no proper pnpfs where all the info can
	be retrieved from.
*/


#include <Drivers.h>


// we start with +300 to not collide with any other SCSI opcode defined
// in scsiprobe_driver.h or scsi.h;
// all ioctl calls return the subsystem status (see SCSI.h)
enum {
	B_SCSI_BUS_RAW_RESET = B_DEVICE_OP_CODES_END + 300,
	B_SCSI_BUS_RAW_PATH_INQUIRY
};

#endif	/* _SCSI_BUS_RAW_DRIVER_H */
