/*
 * Copyright 2003-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */

#ifndef _FLOPPY_H
#define _FLOPPY_H

#include <Drivers.h>
#include <ISA.h>
#include <KernelExport.h>

// TODO: switch to native lock.
#ifdef __HAIKU__
#include <lock.h>
typedef recursive_lock lock;
#define new_lock recursive_lock_init
#define free_lock recursive_lock_destroy
#define	LOCK(l)		recursive_lock_lock(&l);
#define	UNLOCK(l)	recursive_lock_unlock(&l);
#else
#ifndef _IMPEXP_KERNEL
#define _IMPEXP_KERNEL
#endif
#include "lock.h"
#endif

#define FLO "floppy: "
#if defined(DEBUG) && DEBUG > 0
#	define TRACE(x...) dprintf(FLO x)
#else
#	define TRACE(x...)
#endif

#define MOTOR_TIMEOUT 5000000		// 5 seconds
#define MOTOR_SPINUP_DELAY 500000	// 500 msecs
#define FLOPPY_CMD_TIMEOUT 500000 // .5s
#define MAX_DRIVES_PER_CTRL 2
#define CYL_BUFFER_SIZE (5*B_PAGE_SIZE)

/*** those belong to ISA.h ***/
#define B_DMA_MODE_DEMAND 0x00     // modeByte bits for various dma modes
#define B_DMA_MODE_BLOCK  0x80
#define B_DMA_MODE_SINGLE 0x40

#define B_DMA_MODE_MEMR 0x08       // modeByte memory read or write
#define B_DMA_MODE_MEMW 0x04
/*** !ISA.h ***/

typedef enum {
	FLOP_NO_MEDIA,
	FLOP_MEDIA_CHANGED,
	FLOP_MEDIA_UNREADABLE,
	FLOP_MEDIA_FORMAT_FOUND,
	FLOP_WORKING
} floppy_status;

#define CMD_MODE	0x01
#define CMD_SETTRK	0x21	//21
#define CMD_READTRK	0x02
#define CMD_SPECIFY	0x03
#define CMD_SENSED	0x04
#define CMD_WRITED	0x05
#define CMD_READD	0x06
#define CMD_RECAL	0x07
#define CMD_SENSEI	0x08
#define CMD_WRITEDD	0x09
#define CMD_READID	0x0A
#define CMD_READDD	0x0C
#define CMD_FORMAT	0x0D
#define CMD_DUMPREG	0x0E
#define CMD_SEEK	0x0F
#define CMD_RELSEEK	0x8F	//8F
#define CMD_VERSION	0x10
#define CMD_SCANE	0x11
#define CMD_PERPEND	0x12
#define CMD_CONFIG	0x13
#define CMD_LOCK	0x14
#define CMD_VERIFY	0x16
#define CMD_SCANLE	0x19
#define CMD_SCANHE	0x1D

#define CMD_RESET	0xF0
#define CMD_SWITCH_MASK	0x1F

typedef struct floppy_geometry {
	int gap;
	int numsectors;
	int data_rate;
	int format_gap;
	int side2_offset;
	device_geometry g;
	int flags;
	const char *name;
} floppy_geometry;

extern const floppy_geometry supported_geometries[];

#define FL_MFM		0x0001	/* MFM recording */
#define FL_2STEP	0x0002	/* 2 steps between cylinders */
#define FL_PERP		0x0004	/* perpendicular recording */

#define FDC_500KBPS	0x00	/* 500KBPS MFM drive transfer rate */
#define FDC_300KBPS	0x01	/* 300KBPS MFM drive transfer rate */
#define FDC_250KBPS	0x02	/* 250KBPS MFM drive transfer rate */
#define FDC_1MBPS	0x03	/* 1MPBS MFM drive transfer rate */

#define FDC_MAX_CYLINDER 85

typedef struct floppy {
	const floppy_geometry *geometry;
	device_geometry bgeom;
	uint32 iobase;	/* controller address */
	uint32 irq;
	int32 dma;
	int drive_num;	/* number of this drive */
	int current;	/* currently selected drive */
	floppy_status status;
	sem_id completion;
	uint16 pending_cmd;
	uint8 result[8]; /* status of the last finished command */
	
	lock ben;
	spinlock slock;
	isa_module_info *isa;
	
	uint8 data_rate; /* FDC_*BPS */
	uint32 motor_timeout;
	
	area_id buffer_area;
	uint8 *buffer;
	long buffer_index; /* index at which to put/get the next byte in the buffer */
	long avail;	/* valid bytes in the buffer */
	int track;	/* the track currently in the cylinder buffer */
	
	struct floppy *master; /* the 'controller' */
	struct floppy *next; /* next floppy for same controller */
} floppy_t;

typedef struct {
	uint8 id;
	uint8 drive;
	uint8 cylinder;
	uint8 head;
	uint8 sector;
	uint8 sector_size;
	uint8 track_length;
	uint8 gap3_length;
	uint8 data_length;
} floppy_command;

typedef struct {
	uint8 st0;
	uint8 st1;
	uint8 st2;
	uint8 cylinder;
	uint8 head;
	uint8 sector;
	uint8 sector_size;
} floppy_result;

typedef enum {
	STATUS_A = 0,
	STATUS_B = 1,
	DIGITAL_OUT = 2,
	TAPE_DRIVE = 3,
	MAIN_STATUS = 4,
	DATA_RATE_SELECT = 4,
	DATA = 5,
	/* RESERVED = 6, */
	DIGITAL_IN = 7,
	CONFIG_CONTROL = 7
} floppy_reg_selector;

#define FDC_SR0_IC 0xc0
#define FDC_SR0_SE 0x20
#define FDC_SR0_EC 0x10
#define FDC_SR0_HS 0x04
#define FDC_SR0_DS 0x03

/* low level */
extern void write_reg(floppy_t *flp, floppy_reg_selector selector, uint8 data);
extern uint8 read_reg(floppy_t *flp, floppy_reg_selector selector);

/* init */
	/* initialize the controller */
extern void reset_controller(floppy_t *master);
	/* returns a bitmap of the present drives */
extern int count_floppies(floppy_t *master);
	/* seek to track 0 and detect drive presence */
extern int recalibrate_drive(floppy_t *flp);
/* debug */
	/* issue dumpreg command */
extern void floppy_dump_reg(floppy_t *flp);

/* drive handling */
extern void drive_select(floppy_t *flp);
extern void drive_deselect(floppy_t *flp);
extern void turn_on_motor(floppy_t *flp);
extern void turn_off_motor(floppy_t *flp);

/* transactions */
extern void wait_for_rqm(floppy_t *flp);
extern bool can_read_result(floppy_t *flp);
	/* send a command and schedule for irq */
extern int send_command(floppy_t *flp, const uint8 *data, int len);
extern int wait_til_ready(floppy_t *flp);
	/* called by irq handler */
extern int read_result(floppy_t *flp, uint8 *data, int len);
	/* wait for the irq to occur */
extern int wait_result(floppy_t *flp);

extern int32 flo_intr(void *arg);

	/* read sectors to internal flp->master->buffer */
extern ssize_t pio_read_sectors(floppy_t *flp, int lba, int num_sectors);
	/* write sectors from internal flp->master->buffer */
extern ssize_t pio_write_sectors(floppy_t *flp, int lba, int num_sectors);
	/* read a track to internal flp->master->buffer */
extern ssize_t pio_read_track(floppy_t *flp, int lba);

/* high level */
	/* query the media type and fill the geometry information */
extern status_t query_media(floppy_t *flp, bool forceupdate);

extern ssize_t read_sectors(floppy_t *flp, int lba, int num_sectors);

#endif	/* _FLOPPY_H */
