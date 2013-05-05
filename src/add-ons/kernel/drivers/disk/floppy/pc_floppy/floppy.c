/*
 * Copyright 2003-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */

/*
 * inspired by the NewOS floppy driver
 * http://www.newos.org/cgi-bin/fileDownLoad.cgi?FSPC=//depot/newos/kernel/addons/dev/arch/i386/floppy/floppy.c&REV=6
 * related docs:
 * - datasheets
 * http://floppyutil.sourceforge.net/floppy.html
 * http://www.openppc.org/datasheets/NatSemi_PC87308VUL_full.pdf
 * http://www.smsc.com/main/datasheets/37c665gt.pdf
 * http://www.smsc.com/main/datasheets/47b27x.pdf
 * - sources
 * http://www.freebsd.org/cgi/cvsweb.cgi/src/sys/isa/fd.c
 * http://fxr.watson.org/fxr/source/sys/fdcio.h?v=RELENG50
 * http://fxr.watson.org/fxr/source/isa/fd.c?v=RELENG50
 * http://lxr.linux.no/source/drivers/block/floppy.c
 */

/*
 * the stupid FDC engine is able to use several drives at a time, but *only* for some commands,
 * others need exclusive access, as they need the drive to be selected.
 * we just serialize the whole thing to simplify.
 */


#include <Drivers.h>
#include <ISA.h>
#include <PCI.h>
#include <config_manager.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "floppy.h"

/* compile time configuration */

/* enables support for more than one drive */
#define NEW_DEV_LAYOUT
/* don't symlink raw -> 0/raw */
#define NO_SYMLINK_OLD
/* publish the first drive as floppy/raw */
#define FIRST_ONE_NO_NUMBER



/* default values... for 3"5 HD, but we support DD too now */
#define SECTORS_PER_TRACK 18
#define NUM_HEADS 2
#define SECTORS_PER_CYLINDER (SECTORS_PER_TRACK * NUM_HEADS)
#define SECTOR_SIZE 512
#define TRACK_SIZE (SECTOR_SIZE * SECTORS_PER_CYLINDER)
#define NUM_TRACKS 80
#define DISK_SIZE (TRACK_SIZE * NUM_TRACKS)


extern const char floppy_icon[];
extern const char floppy_mini_icon[];

#define OLD_DEV_FORMAT "disk/floppy/raw"

#ifndef NEW_DEV_LAYOUT
#  define MAX_FLOPPIES 1
#  define DEV_FORMAT OLD_DEV_FORMAT
#else
#  define MAX_FLOPPIES 4
#  define DEV_FORMAT "disk/floppy/%d/raw"
#endif

static char floppy_dev_name[MAX_FLOPPIES][B_OS_NAME_LENGTH];
static const char *dev_names[MAX_FLOPPIES+1];

extern device_hooks floppy_hooks;
int32 api_version = B_CUR_DRIVER_API_VERSION;

isa_module_info *isa;
config_manager_for_driver_module_info *config_man;

struct floppy floppies[MAX_FLOPPIES];

/* by order of check */
/* from fd.c from fBSD */
const floppy_geometry supported_geometries[] = {

/*	{ gap,	nsecs,	data_rate,	fmtgap,	sd2off,	{secsz,	s/trk,	ntraks,	nheads,	type,	rmable,	ro,	wonce},	flags,		name }, */
	{ 0x1b,	2880,	FDC_500KBPS,	0x6c,	0,	{ 512,	18,	80,	2,	B_DISK,	true,	true,	false}, FL_MFM,		"1.44M" },
/*	{ 0x20,	5760,	FDC_1MBPS,		0x4c,	1,	{ 512,	36,	80,	2,	B_DISK,	true,	true,	false}, FL_MFM|FL_PERP,	"2.88M" }, */
	{ 0x20,	1440,	FDC_250KBPS,	0x50,	0,	{ 512,	9,	80,	2,	B_DISK,	true,	true,	false}, FL_MFM,		"720K" },

	{ 0,	0,	0,		0,	0,	{0,	0,	0,	0,	0,	true,	false,	false}, 0,	NULL }
};

typedef struct floppy_cookie {
	struct floppy *flp;
} floppy_cookie;


static void motor_off_daemon(void *t, int tim);


status_t
init_hardware(void)
{
	TRACE("init_hardware()\n");
	/* assume there is always one */
	return B_OK;
}


status_t
init_driver(void)
{
	status_t err;
	unsigned int i, j;
	int current;
	uint64 cmcookie = 0;
	struct device_info info;
	
	TRACE("init_driver()\n");
	if ((err = get_module(B_ISA_MODULE_NAME, (module_info **)&isa)) < B_OK) {
		dprintf(FLO "no isa module!\n");
		return err;
	}
	if ((err = get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, (module_info **)&config_man)) < B_OK) {
		dprintf(FLO "no config_manager module!\n");
		put_module(B_ISA_MODULE_NAME);
		return err;
	}
	for (i = 0; i < MAX_FLOPPIES; i++)
		memset(&(floppies[i]), 0, sizeof(floppy_t));
	current = 0;
	register_kernel_daemon(motor_off_daemon, floppies, 5);
	while (config_man->get_next_device_info(B_ISA_BUS, &cmcookie, &info, sizeof(struct device_info)) == B_OK) {
		struct device_configuration *conf;
		struct floppy *master;
		struct floppy *last = NULL;
		int flops;
		if ((info.config_status != B_OK) || 
				((info.flags & B_DEVICE_INFO_ENABLED) == 0) || 
				((info.flags & B_DEVICE_INFO_CONFIGURED) == 0) || 
				(info.devtype.base != PCI_mass_storage) || 
				(info.devtype.subtype != PCI_floppy) || 
				(info.devtype.interface != 0) /* XXX: needed ?? */)
			continue;
		err = config_man->get_size_of_current_configuration_for(cmcookie);
		if (err < B_OK)
			continue;
		conf = (struct device_configuration *)malloc(err);
		if (conf == NULL)
			continue;
		if (config_man->get_current_configuration_for(cmcookie, conf, err) < B_OK) {
			free(conf);
			continue;
		}
		master = &(floppies[current]);
		for (i = 0; i < conf->num_resources; i++) {
			if (conf->resources[i].type == B_IO_PORT_RESOURCE) {
				int32 iobase = conf->resources[i].d.r.minbase;
				int32 len = conf->resources[i].d.r.len;
				/* WTF do I get 
				 * min 3f2 max 3f2 align 0 len 4 ?
				 * on my K6-2, I even get 2 bytes at 3f2 + 2 bytes at 3f4 !
				 * looks like AT interface, suppose PS/2, which is 8 bytes wide.
				 * answer: because the 8th byte is also for the IDE controller...
				 * good old PC stuff... PPC here I come !
				 */
				// XXX: maybe we shouldn't use DIGITAL_IN if register window 
				// is only 6 bytes ?
				if (len != 8) {
					if ((master->iobase & 0xfffffff8) == 0x3f0)
						iobase = 0x3f0;
					else {
						dprintf(FLO "controller has weird register window len %ld !?\n", 
							len);
						break;
					}
				}
				master->iobase = iobase;
			}
			if (conf->resources[i].type == B_IRQ_RESOURCE) {
				int val;
				for (val = 0; val < 32; val++) {
					if (conf->resources[i].d.m.mask & (1 << val)) {
						master->irq = val;
						break;
					}
				}
			}
			if (conf->resources[i].type == B_DMA_RESOURCE) {
				int val;
				for (val = 0; val < 32; val++) {
					if (conf->resources[i].d.m.mask & (1 << val)) {
						master->dma = val;
						break;
					}
				}
			}
		}
		if (master->iobase == 0) {
			dprintf(FLO "controller doesn't have any io ??\n");
			goto config_error;
		}
		dprintf(FLO "controller at 0x%04lx, irq %ld, dma %ld\n",
			master->iobase, master->irq, master->dma);
		//master->dma = -1; // XXX: DEBUG: disable DMA
		/* allocate resources */
		master->completion = create_sem(0, "floppy interrupt");
		if (master->completion < 0)
			goto config_error;
		new_lock(&master->ben, "floppy driver access");
		//if (new_lock(&master->ben, "floppy driver access") < B_OK)
		//	goto config_error;
		
		/* 20K should hold a full cylinder XXX: B_LOMEM for DMA ! */
		master->buffer_area = create_area("floppy cylinder buffer", (void **)&master->buffer,
									B_ANY_KERNEL_ADDRESS, CYL_BUFFER_SIZE,
									B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);
		if (master->buffer_area < B_OK)
			goto config_error2;
		master->slock = 0;
		master->isa = isa;
		if (install_io_interrupt_handler(master->irq, flo_intr, (void *)master, 0) < B_OK)
			goto config_error2;

		flops = count_floppies(master); /* actually a bitmap */
		flops = MAX(flops, 1); /* XXX: assume at least one */
		TRACE("drives found: 0x%01x\n", flops);
		for (i = 0; current < MAX_FLOPPIES && i < 4; i++) {
			if ((flops & (1 << i)) == 0)
				continue;
			floppies[current].next = NULL;
			if (last)
				last->next = &(floppies[current]);
			floppies[current].iobase = master->iobase;
			floppies[current].irq = master->irq;
			floppies[current].dma = master->dma;
			floppies[current].drive_num = i;
			floppies[current].master = master;
			floppies[current].isa = master->isa;
			floppies[current].completion = master->completion;
			floppies[current].track = -1;
			last = &(floppies[current]);
			current++;
		}
		/* XXX: TODO: remove "assume at least one" + cleanup if no drive on controller */
		
		goto config_ok;


config_error2:
		if (master->buffer_area)
			delete_area(master->buffer_area);
		free_lock(&master->ben);
config_error:
		
		if (master->completion > 0)
			delete_sem(master->completion);
		master->iobase = 0;
		put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
		put_module(B_ISA_MODULE_NAME);
config_ok:
		free(conf);
		if (current >= MAX_FLOPPIES)
			break;
	}
	
	/* make device names */
	for (i = 0, j = 0; i < MAX_FLOPPIES; i++) {
		if (floppies[i].iobase) {
#ifdef FIRST_ONE_NO_NUMBER
			if (!i)
				strcpy(floppy_dev_name[i], OLD_DEV_FORMAT);
			else
#endif
			sprintf(floppy_dev_name[i], DEV_FORMAT, i);
			dev_names[j++] = floppy_dev_name[i];
			TRACE("names[%d] = %s\n", j-1, dev_names[j-1]);
		} else
			strcpy(floppy_dev_name[i], "");
	}
	dev_names[j] = NULL;
	
	
#ifdef NEW_DEV_LAYOUT
#if !defined(NO_SYMLINK_OLD) && !defined(FIRST_ONE_NO_NUMBER)
	/* fake the good old single drive */
	mkdir("/dev/disk/floppy", 0755);
	symlink("0/raw", "/dev/disk/floppy/raw");
#endif
#endif
	return B_OK;
}


void
uninit_driver(void)
{
	int i;
	TRACE("uninit_driver()\n");
	unregister_kernel_daemon(motor_off_daemon, floppies);
	for (i = 0; i < MAX_FLOPPIES; i++) {
		if (floppies[i].iobase)
			turn_off_motor(&floppies[i]);
	}
	TRACE("deallocating...\n");
	for (i = 0; i < MAX_FLOPPIES; i++) {
		if (floppies[i].iobase) {
			if (floppies[i].master == &(floppies[i])) {
				remove_io_interrupt_handler(floppies[i].irq, flo_intr, (void *)&(floppies[i]));
				free_lock(&floppies[i].ben);
				delete_sem(floppies[i].completion);
				delete_area(floppies[i].buffer_area);
			}
		}
	}
	TRACE("uninit done\n");
	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
	put_module(B_ISA_MODULE_NAME);
}


const char **
publish_devices(void)
{
	if (dev_names[0] == NULL)
		return NULL;
	return dev_names;
}


device_hooks *
find_device(const char *name)
{
	(void)name;
	return &floppy_hooks;
}


static status_t
flo_open(const char *name, uint32 flags, floppy_cookie **cookie)
{
	int i;
	TRACE("open(%s)\n", name);
	if (flags & O_NONBLOCK)
		return EINVAL;
	for (i = 0; i < MAX_FLOPPIES; i++) {
		if (dev_names[i] && (strncmp(name, dev_names[i], strlen(dev_names[i])) == 0))
			break;
	}
	if (i >= MAX_FLOPPIES)
		return ENODEV;
	*cookie = (floppy_cookie *)malloc(sizeof(floppy_cookie));
	if (*cookie == NULL)
		return B_NO_MEMORY;
	(*cookie)->flp = &(floppies[i]);
	/* if we don't know yet if there's something in, check that */
	if ((*cookie)->flp->status <= FLOP_MEDIA_CHANGED)
		query_media((*cookie)->flp, false);
	return B_OK;
}


static status_t
flo_close(floppy_cookie *cookie)
{
	TRACE("close()\n");
	cookie->flp = NULL;
	return B_OK;
}


static status_t
flo_free(floppy_cookie *cookie)
{
	TRACE("free()\n");
	free(cookie);
	return B_OK;
}


static status_t
flo_read(floppy_cookie *cookie, off_t position, void *data, size_t *numbytes)
{
	status_t err;
	size_t len = *numbytes;
	size_t bytes_read = 0;
	int sectsize = SECTOR_SIZE;
	int cylsize = TRACK_SIZE; /* hmm, badly named, it's actually track_size (a cylinder has 2 tracks, one per head) */
	const device_geometry *geom = NULL;
	ssize_t disk_size = 0;
	
	if (cookie->flp->geometry)
		geom = &cookie->flp->geometry->g;

	if (geom) {
		sectsize = geom->bytes_per_sector;
		cylsize = sectsize * geom->sectors_per_track/* * geom->head_count*/;
		disk_size = (geom->bytes_per_sector)
				* (geom->sectors_per_track)
				* (geom->head_count)
				* (geom->cylinder_count);
	}
	if (disk_size <= 0) {
		*numbytes = 0;
		return B_DEV_NO_MEDIA;
	}
	if (position > disk_size) {
		dprintf(FLO "attempt to seek beyond device end!\n");
		*numbytes = 0;
		return B_OK;
	}
	if (position + *numbytes > disk_size) {
		dprintf(FLO "attempt to read beyond device end!\n");
		*numbytes = (size_t)((off_t)disk_size - position);
		if (*numbytes == 0)
			return B_OK;
	}

	// handle partial first block
	if ((position % SECTOR_SIZE) != 0) {
		size_t toread;
		TRACE("read: begin %Ld, %ld\n", position, bytes_read);
		err = read_sectors(cookie->flp, position / sectsize, 1);
		TRACE("PIO READ got %ld sect\n", err);
		if (err <= 0) {
			*numbytes = 0;
			return err;
		}
		toread = MIN(len, (size_t)sectsize);
		toread = MIN(toread, sectsize - (position % sectsize));
		memcpy(data, cookie->flp->master->buffer + position % cylsize/*(sectsize * ) + (position % sectsize)*/, toread);
		len -= toread;
		bytes_read += toread;
		position += toread;
	}

	// read the middle blocks
	while (len >= (size_t)sectsize) {
		TRACE("read: middle %Ld, %ld, %ld\n", position, bytes_read, len);

		// try to read as many sectors as we can
		err = read_sectors(cookie->flp, position / sectsize, len / sectsize);
		TRACE("PIO READ got %ld sect\n", err);
		if (err <= 0) {
			*numbytes = 0;
			return err;
		}
		memcpy(((char *)data) + bytes_read, cookie->flp->master->buffer + position % cylsize, err*sectsize);
		len -= err * sectsize;
		bytes_read += err * sectsize;
		position += err * sectsize;
	}
	// handle partial last block
	if (len > 0 && (len % sectsize) != 0) {
		TRACE("read: end %Ld, %ld %ld\n", position, bytes_read, len);

		err = read_sectors(cookie->flp, position / sectsize, 1);
		TRACE("PIO READ got %ld sect\n", err);
		if (err <= 0) {
			*numbytes = 0;
			return err;
		}
		memcpy(((char *)data) + bytes_read, cookie->flp->master->buffer + position % cylsize, len);
		bytes_read += len;
		position += len;
	}
	
	*numbytes = bytes_read;
	return B_OK;
}


static status_t
flo_write(floppy_cookie *cookie, off_t position, const void *data, size_t *numbytes)
{
	*numbytes = 0;
	return B_ERROR;
}


static status_t
flo_control(floppy_cookie *cookie, uint32 op, void *data, size_t len)
{
	device_geometry *geom;
	status_t err;
	floppy_t *flp = cookie->flp;
	switch (op) {
	case B_GET_ICON:
		TRACE("control(B_GET_ICON, %ld)\n", ((device_icon *)data)->icon_size);
		if (((device_icon *)data)->icon_size == 16) { /* mini icon */
			memcpy(((device_icon *)data)->icon_data, floppy_mini_icon, (16*16));
			return B_OK;
		}
		if (((device_icon *)data)->icon_size == 32) { /* icon */
			memcpy(((device_icon *)data)->icon_data, floppy_icon, (32*32));
			return B_OK;
		}
		return EINVAL;
	case B_GET_BIOS_DRIVE_ID:
		TRACE("control(B_GET_BIOS_DRIVE_ID)\n");
		*(uint8 *)data = 0;
		return B_OK;
	case B_GET_DEVICE_SIZE:
		TRACE("control(B_GET_DEVICE_SIZE)\n");
		err = query_media(cookie->flp, true);
		*(size_t *)data = (flp->bgeom.bytes_per_sector)
						* (flp->bgeom.sectors_per_track)
						* (flp->bgeom.head_count)
						* (flp->bgeom.cylinder_count);
		return B_OK;
	case B_GET_GEOMETRY:
	case B_GET_BIOS_GEOMETRY:
		TRACE("control(B_GET_(BIOS)_GEOMETRY)\n");
		err = query_media(cookie->flp, false);
		geom = (device_geometry *)data;
		geom->bytes_per_sector = flp->bgeom.bytes_per_sector;
		geom->sectors_per_track = flp->bgeom.sectors_per_track;
		geom->cylinder_count = flp->bgeom.cylinder_count;
		geom->head_count = flp->bgeom.head_count;
		geom->device_type = B_DISK;
		geom->removable = true;
		geom->read_only = flp->bgeom.read_only;
		geom->write_once = false;
		return B_OK;
	case B_GET_MEDIA_STATUS:
		TRACE("control(B_GET_MEDIA_STATUS)\n");
		err = query_media(cookie->flp, false);
		*(status_t *)data = err;
		return B_OK;
	}
	TRACE("control(%ld)\n", op);
	return EINVAL;
}


static void
motor_off_daemon(void *t, int tim)
{
	int i;
	for (i = 0; i < MAX_FLOPPIES; i++) {
		if (floppies[i].iobase && !floppies[i].pending_cmd && floppies[i].motor_timeout > 0) {
			TRACE("floppies[%d].motor_timeout = %ld\n", i, floppies[i].motor_timeout);
			if (atomic_add(&floppies[i].motor_timeout, -500000) <= 500000) {
				dprintf("turning off motor for drive %d\n", floppies[i].drive_num);
				turn_off_motor(&floppies[i]);
				floppies[i].motor_timeout = 0;
			}
		}
	}
}


device_hooks floppy_hooks = {
	(device_open_hook)flo_open,
	(device_close_hook)flo_close,
	(device_free_hook)flo_free,
	(device_control_hook)flo_control,
	(device_read_hook)flo_read,
	(device_write_hook)flo_write,
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	NULL  /* writev */
};

