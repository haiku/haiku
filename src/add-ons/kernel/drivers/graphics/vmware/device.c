/*
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Eric Petit <eric.petit@lapsus.org>
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */


#include "driver.h"

#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>
#include <graphic_driver.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define get_pci(o, s) (*gPciBus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*gPciBus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))


static void
PrintCapabilities(uint32 c)
{
	TRACE("capabilities:\n");
	if (c & SVGA_CAP_RECT_FILL)			TRACE("RECT_FILL\n");
	if (c & SVGA_CAP_RECT_COPY)			TRACE("RECT_COPY\n");
	if (c & SVGA_CAP_RECT_PAT_FILL)		TRACE("RECT_PAT_FILL\n");
	if (c & SVGA_CAP_LEGACY_OFFSCREEN)	TRACE("LEGACY_OFFSCREEN\n");
	if (c & SVGA_CAP_RASTER_OP)			TRACE("RASTER_OP\n");
	if (c & SVGA_CAP_CURSOR)			TRACE("CURSOR\n");
	if (c & SVGA_CAP_CURSOR_BYPASS)		TRACE("CURSOR_BYPASS\n");
	if (c & SVGA_CAP_CURSOR_BYPASS_2)	TRACE("CURSOR_BYPASS_2\n");
	if (c & SVGA_CAP_8BIT_EMULATION)	TRACE("8BIT_EMULATION\n");
	if (c & SVGA_CAP_ALPHA_CURSOR)		TRACE("ALPHA_CURSOR\n");
	if (c & SVGA_CAP_GLYPH)				TRACE("GLYPH\n");
	if (c & SVGA_CAP_GLYPH_CLIPPING)	TRACE("GLYPH_CLIPPING\n");
	if (c & SVGA_CAP_OFFSCREEN_1)		TRACE("OFFSCREEN_1\n");
	if (c & SVGA_CAP_ALPHA_BLEND)		TRACE("ALPHA_BLEND\n");
	if (c & SVGA_CAP_3D)				TRACE("3D\n");
	if (c & SVGA_CAP_EXTENDED_FIFO)		TRACE("EXTENDED_FIFO\n");
}


static status_t
CheckCapabilities()
{
	SharedInfo *si = gPd->si;
	uint32 id;

	/* Needed to read/write registers */
	si->indexPort = gPd->pcii.u.h0.base_registers[0] + SVGA_INDEX_PORT;
	si->valuePort = gPd->pcii.u.h0.base_registers[0] + SVGA_VALUE_PORT;
	TRACE("index port: %d, value port: %d\n",
		si->indexPort, si->valuePort);

	/* This should be SVGA II according to the PCI device_id,
	 * but just in case... */
	WriteReg(SVGA_REG_ID, SVGA_ID_2);
	if ((id = ReadReg(SVGA_REG_ID)) != SVGA_ID_2) {
		TRACE("SVGA_REG_ID is %ld, not %d\n", id, SVGA_REG_ID);
		return B_ERROR;
	}
	TRACE("SVGA_REG_ID OK\n");

	/* Grab some info */
	si->maxWidth = ReadReg(SVGA_REG_MAX_WIDTH);
	si->maxHeight = ReadReg(SVGA_REG_MAX_HEIGHT);
	TRACE("max resolution: %ldx%ld\n", si->maxWidth, si->maxHeight);
	si->fbDma = (void *)ReadReg(SVGA_REG_FB_START);
	si->fbSize = ReadReg(SVGA_REG_VRAM_SIZE);
	TRACE("frame buffer: %p, size %ld\n", si->fbDma, si->fbSize);
	si->fifoDma = (void *)ReadReg(SVGA_REG_MEM_START);
	si->fifoSize = ReadReg(SVGA_REG_MEM_SIZE) & ~3;
	TRACE("fifo: %p, size %ld\n", si->fifoDma, si->fifoSize);
	si->capabilities = ReadReg(SVGA_REG_CAPABILITIES);
	PrintCapabilities(si->capabilities);
	si->fifoMin = (si->capabilities & SVGA_CAP_EXTENDED_FIFO) ?
		ReadReg(SVGA_REG_MEM_REGS) : 4;

	return B_OK;
}


static status_t
MapDevice()
{
	SharedInfo *si = gPd->si;
	int writeCombined = 1;

	/* Map the frame buffer */
	si->fbArea = map_physical_memory("VMware frame buffer",
		(addr_t)si->fbDma, si->fbSize, B_ANY_KERNEL_BLOCK_ADDRESS|B_MTR_WC,
		B_READ_AREA|B_WRITE_AREA, (void **)&si->fb);
	if (si->fbArea < 0) {
		/* Try again without write combining */
		writeCombined = 0;
		si->fbArea = map_physical_memory("VMware frame buffer",
			(addr_t)si->fbDma, si->fbSize, B_ANY_KERNEL_BLOCK_ADDRESS,
			B_READ_AREA|B_WRITE_AREA, (void **)&si->fb);
	}
	if (si->fbArea < 0) {
		TRACE("failed to map frame buffer\n");
		return si->fbArea;
	}
	TRACE("frame buffer mapped: %p->%p, area %ld, size %ld, write "
		"combined: %d\n", si->fbDma, si->fb, si->fbArea,
		si->fbSize, writeCombined);

	/* Map the fifo */
	si->fifoArea = map_physical_memory("VMware fifo",
		(addr_t)si->fifoDma, si->fifoSize, B_ANY_KERNEL_BLOCK_ADDRESS,
		B_READ_AREA|B_WRITE_AREA, (void **)&si->fifo);
	if (si->fifoArea < 0) {
		TRACE("failed to map fifo\n");
		delete_area(si->fbArea);
		return si->fifoArea;
	}
	TRACE("fifo mapped: %p->%p, area %ld, size %ld\n", si->fifoDma,
		si->fifo, si->fifoArea, si->fifoSize);

	return B_OK;
}


static void
UnmapDevice()
{
	SharedInfo *si = gPd->si;
	pci_info *pcii = &gPd->pcii;
	uint32 tmpUlong;

	/* Disable memory mapped IO */
	tmpUlong = get_pci(PCI_command, 2);
	tmpUlong &= ~PCI_command_memory;
	set_pci(PCI_command, 2, tmpUlong);

	/* Delete the areas */
	if (si->fifoArea >= 0)
		delete_area(si->fifoArea);
	if (si->fbArea >= 0)
		delete_area(si->fbArea);
	si->fifoArea = si->fbArea = -1;
	si->fb = si->fifo = NULL;
}


static status_t
CreateShared()
{
	gPd->sharedArea = create_area("VMware shared", (void **)&gPd->si,
		B_ANY_KERNEL_ADDRESS, ROUND_TO_PAGE_SIZE(sizeof(SharedInfo)),
		B_FULL_LOCK, 0);
	if (gPd->sharedArea < B_OK) {
		TRACE("failed to create shared area\n");
		return gPd->sharedArea;
	}
	TRACE("shared area created\n");

	memset(gPd->si, 0, sizeof(SharedInfo));
	return B_OK;
}


static void
FreeShared()
{
	delete_area(gPd->sharedArea);
	gPd->sharedArea = -1;
	gPd->si = NULL;
}


static status_t
OpenHook(const char *name, uint32 flags, void **cookie)
{
	status_t ret = B_OK;
	pci_info *pcii = &gPd->pcii;
	uint32 tmpUlong;

	TRACE("OpenHook (%s, %ld)\n", name, flags);
	ACQUIRE_BEN(gPd->kernel);

	if (gPd->isOpen)
		goto markAsOpen;

	/* Enable memory mapped IO and VGA I/O */
	tmpUlong = get_pci(PCI_command, 2);
	tmpUlong |= PCI_command_memory;
	tmpUlong |= PCI_command_io;
	set_pci(PCI_command, 2, tmpUlong);

	if ((ret = CreateShared()) != B_OK)
		goto done;
	if ((ret = CheckCapabilities()) != B_OK)
		goto freeShared;
	if ((ret = MapDevice()) != B_OK)
		goto freeShared;

markAsOpen:
	gPd->isOpen++;
	*cookie = gPd;
	goto done;

freeShared:
	FreeShared();

done:
	RELEASE_BEN(gPd->kernel);
	TRACE("OpenHook: %ld\n", ret);
	return ret;
}


/*--------------------------------------------------------------------*/
/* ReadHook, WriteHook, CloseHook: do nothing */

static status_t
ReadHook(void *dev, off_t pos, void *buf, size_t *len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}
static status_t
WriteHook(void *dev, off_t pos, const void *buf, size_t *len)
{
	*len = 0;
	return B_NOT_ALLOWED;
}
static status_t
CloseHook(void *dev)
{
	return B_OK;
}


/*--------------------------------------------------------------------*/
/* FreeHook: closes down the device */

static status_t
FreeHook(void *dev)
{
	TRACE("FreeHook\n");
	ACQUIRE_BEN(gPd->kernel);

	if (gPd->isOpen < 2) {
		UnmapDevice();
		FreeShared();
	}
	gPd->isOpen--;

	RELEASE_BEN(gPd->kernel);
	TRACE("FreeHook ends\n");
	return B_OK;
}


static void
UpdateCursor(SharedInfo *si)
{
	WriteReg(SVGA_REG_CURSOR_ID, CURSOR_ID);
	WriteReg(SVGA_REG_CURSOR_X, si->cursorX);
	WriteReg(SVGA_REG_CURSOR_Y, si->cursorY);
	WriteReg(SVGA_REG_CURSOR_ON, si->cursorShow ? SVGA_CURSOR_ON_SHOW :
				SVGA_CURSOR_ON_HIDE);
}


/*--------------------------------------------------------------------*/
/* ControlHook: responds the the ioctl from the accelerant */

static status_t
ControlHook(void *dev, uint32 msg, void *buf, size_t len)
{
	SharedInfo *si = gPd->si;

	switch (msg) {
		case B_GET_ACCELERANT_SIGNATURE:
			strcpy((char *)buf, "vmware.accelerant");
			return B_OK;

		case VMWARE_GET_PRIVATE_DATA:
			*((area_id *)buf) = gPd->sharedArea;
			return B_OK;

		case VMWARE_FIFO_START:
			WriteReg(SVGA_REG_ENABLE, 1);
			WriteReg(SVGA_REG_CONFIG_DONE, 1);
			return B_OK;

		case VMWARE_FIFO_STOP:
			WriteReg(SVGA_REG_CONFIG_DONE, 0);
			WriteReg(SVGA_REG_ENABLE, 0);
			return B_OK;

		case VMWARE_FIFO_SYNC:
			WriteReg(SVGA_REG_SYNC, 1);
			while (ReadReg(SVGA_REG_BUSY));
			return B_OK;

		case VMWARE_SET_MODE:
		{
			display_mode *dm = buf;
			WriteReg(SVGA_REG_WIDTH, dm->virtual_width);
			WriteReg(SVGA_REG_HEIGHT, dm->virtual_height);
			WriteReg(SVGA_REG_BITS_PER_PIXEL, BppForSpace(dm->space));
			si->fbOffset = ReadReg(SVGA_REG_FB_OFFSET);
			si->bytesPerRow = ReadReg(SVGA_REG_BYTES_PER_LINE);
			ReadReg(SVGA_REG_DEPTH);
			ReadReg(SVGA_REG_PSEUDOCOLOR);
			ReadReg(SVGA_REG_RED_MASK);
			ReadReg(SVGA_REG_GREEN_MASK);
			ReadReg(SVGA_REG_BLUE_MASK);
			return B_OK;
		}

		case VMWARE_SET_PALETTE:
		{
			uint8 *color = (uint8 *)buf;
			uint32 i;
			if (ReadReg(SVGA_REG_PSEUDOCOLOR) != 1)
				return B_ERROR;

			for (i = 0; i < 256; i++) {
				WriteReg(SVGA_PALETTE_BASE + 3 * i, *color++);
				WriteReg(SVGA_PALETTE_BASE + 3 * i + 1, *color++);
				WriteReg(SVGA_PALETTE_BASE + 3 * i + 2, *color++);
			}
			return B_OK;
		}

		case VMWARE_MOVE_CURSOR:
		{
			uint16 *pos = buf;
			si->cursorX = pos[0];
			si->cursorY = pos[1];
			UpdateCursor(si);
			return B_OK;
		}

		case VMWARE_SHOW_CURSOR:
		{
			si->cursorShow = *((bool *)buf);
			UpdateCursor(si);
			return B_OK;
		}

		case VMWARE_GET_DEVICE_NAME:
			dprintf("device: VMWARE_GET_DEVICE_NAME %s\n", gPd->names[0]);
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			if (user_strlcpy((char *)buf, gPd->names[0],
					B_PATH_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;
#else
			strlcpy((char *)buf, gPd->names[0], B_PATH_NAME_LENGTH);
#endif
			return B_OK;

	}

	TRACE("ioctl: %ld, %p, %ld\n", msg, buf, len);
	return B_DEV_INVALID_IOCTL;
}


device_hooks gGraphicsDeviceHooks =
{
	OpenHook,
	CloseHook,
	FreeHook,
	ControlHook,
	ReadHook,
	WriteHook,
	NULL,
	NULL,
	NULL,
	NULL
};

