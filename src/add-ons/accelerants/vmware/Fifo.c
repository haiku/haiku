/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */


#include "GlobalData.h"


/*--------------------------------------------------------------------*/
/* UpdateThread: we explicitely need to tell VMware what and when
 * something should be refreshed from the frame buffer. Since the
 * app_server doesn't tell us what it's doing, we simply force a full
 * screen update every 1/50 second */

static int32
UpdateThread(void *data)
{
	bigtime_t begin, end, wait;

	while (!gUpdateThreadDie) {
		begin = system_time();
		FifoUpdateFullscreen();
		end = system_time();

		/* 50 Hz refresh */
		wait = 20000 - (end - begin);
		if (wait > 0)
			snooze(wait);
	}

	return B_OK;
}


static void
FifoPrintCapabilities(int c)
{
	TRACE("fifo capabilities:\n");
	if (c & SVGA_FIFO_CAP_FENCE)		TRACE("FENCE\n");
	if (c & SVGA_FIFO_CAP_ACCELFRONT)	TRACE("ACCELFRONT\n");
	if (c & SVGA_FIFO_CAP_PITCHLOCK)	TRACE("PITCHLOCK\n");
}


void
FifoInit(void)
{
	uint32 *fifo = gSi->fifo;

	/* Stop update thread, if any */
	if (gUpdateThread > B_OK) {
		status_t exitValue;
		gUpdateThreadDie = 1;
		wait_for_thread(gUpdateThread, &exitValue);
	}

	gSi->fifoCapabilities = 0;
	gSi->fifoFlags = 0;
	if (gSi->capabilities & SVGA_CAP_EXTENDED_FIFO) {
		gSi->fifoCapabilities = fifo[SVGA_FIFO_CAPABILITIES];
		gSi->fifoFlags = fifo[SVGA_FIFO_FLAGS];
		FifoPrintCapabilities(gSi->fifoCapabilities);
	}

	fifo[SVGA_FIFO_MIN]		 = gSi->fifoMin * 4;
	fifo[SVGA_FIFO_MAX]		 = gSi->fifoSize;
	fifo[SVGA_FIFO_NEXT_CMD] = fifo[SVGA_FIFO_MIN];
	fifo[SVGA_FIFO_STOP]	 = fifo[SVGA_FIFO_MIN];

	gSi->fifoNext = fifo[SVGA_FIFO_NEXT_CMD];

	/* Launch a new update thread */
	gUpdateThreadDie = 0;
	gUpdateThread = spawn_thread(UpdateThread, "VMware",
		B_REAL_TIME_DISPLAY_PRIORITY, NULL);
	resume_thread(gUpdateThread);

	TRACE("init fifo: %ld -> %ld\n",
		fifo[SVGA_FIFO_MIN], fifo[SVGA_FIFO_MAX]);
}


void
FifoSync(void)
{
	ioctl(gFd, VMWARE_FIFO_SYNC, NULL, 0);
}


void
FifoBeginWrite(void)
{
	ACQUIRE_BEN(gSi->fifoLock);
}


void
FifoWrite(uint32 value)
{
	uint32 *fifo = gSi->fifo;
	uint32 fifoCapacity = fifo[SVGA_FIFO_MAX] - fifo[SVGA_FIFO_MIN];

	/* If the fifo is full, sync it */
	if (fifo[SVGA_FIFO_STOP] == fifo[SVGA_FIFO_NEXT_CMD] + 4 ||
		fifo[SVGA_FIFO_STOP] + fifoCapacity == fifo[SVGA_FIFO_NEXT_CMD] + 4)
		FifoSync();

	fifo[gSi->fifoNext / 4] = value;
	gSi->fifoNext = fifo[SVGA_FIFO_MIN] +
		(gSi->fifoNext + 4 - fifo[SVGA_FIFO_MIN]) % fifoCapacity;
}


void
FifoEndWrite(void)
{
	uint32 *fifo = gSi->fifo;

	fifo[SVGA_FIFO_NEXT_CMD] = gSi->fifoNext;
	RELEASE_BEN(gSi->fifoLock);
}


void
FifoUpdateFullscreen(void)
{
	FifoBeginWrite();
	FifoWrite(SVGA_CMD_UPDATE);
	FifoWrite(0);
	FifoWrite(0);
	FifoWrite(gSi->dm.virtual_width);
	FifoWrite(gSi->dm.virtual_height);
	FifoEndWrite();
}

