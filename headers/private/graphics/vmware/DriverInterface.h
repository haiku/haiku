/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef DRIVERINTERFACE_H
#define DRIVERINTERFACE_H

#include <GraphicsDefs.h>
#include <Accelerant.h>
#include <Drivers.h>
#include <PCI.h>
#include <OS.h>

#include "vm_device_version.h"
#include "svga_reg.h"

#define MAX_SAMPLE_DEVICE_NAME_LENGTH	32
#define CURSOR_ID						1


/*--------------------------------------------------------------------*/
/* Benaphores */

typedef struct {
	sem_id	sem;
	int32	ben;
} Benaphore;
#define INIT_BEN(x)		x.sem = create_sem(0, "VMware "#x); x.ben = 0;
#define ACQUIRE_BEN(x)	if((atomic_add(&(x.ben), 1)) >= 1) acquire_sem(x.sem);
#define RELEASE_BEN(x)	if((atomic_add(&(x.ben), -1)) > 1) release_sem(x.sem);
#define DELETE_BEN(x)	delete_sem(x.sem);


/*--------------------------------------------------------------------*/
/* Utils */

#define ROUND_TO_PAGE_SIZE(x) (((x)+(B_PAGE_SIZE)-1)&~((B_PAGE_SIZE)-1))

static inline int
BppForSpace(int space)
{
	switch (space) {
		case B_RGB32:
			return 32;
		case B_RGB24:
			return 24;
		case B_RGB16:
			return 16;
		case B_RGB15:
			return 15;
		case B_CMAP8:
			return 8;
	}
	return 0;
}


/*--------------------------------------------------------------------*/
/* Request codes for ioctl() */

enum {
	VMWARE_GET_PRIVATE_DATA = B_DEVICE_OP_CODES_END + 1,
	VMWARE_FIFO_START,
	VMWARE_FIFO_STOP,
	VMWARE_FIFO_SYNC,
	VMWARE_SET_MODE,
	VMWARE_SHOW_CURSOR,
	VMWARE_MOVE_CURSOR,
	VMWARE_GET_DEVICE_NAME,
	VMWARE_SET_PALETTE
};


/*--------------------------------------------------------------------*/
/* Structure shared between the kernel driver and the accelerant */

typedef struct {
	/* Device info and capabilities */
	uint16			vendorId;
	uint16			deviceId;
	uint8			revision;
	uint32			maxWidth;
	uint32			maxHeight;
	void			*fbDma;
	uint32			fbSize;
	void			*fifoDma;
	uint32			fifoSize;
	uint32			fifoMin;
	uint32			capabilities;
	uint32			fifoCapabilities;
	uint32			fifoFlags;

	/* For registers access */
	uint16			indexPort;
	uint16			valuePort;

	/* Mapped areas */
	area_id			fbArea;
	void			*fb;
	area_id			fifoArea;
	void			*fifo;

	/* This changes when we switch to another mode */
	uint32			fbOffset;
	uint32			bytesPerRow;

	/* Current display mode */
	display_mode	dm;

	Benaphore		engineLock;
	Benaphore		fifoLock;
	uint32			fifoNext;
	
	/* Cursor state */
	bool			cursorShow;
	uint16			cursorX;
	uint16			cursorY;
} SharedInfo;

#endif
