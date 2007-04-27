#ifndef _FSSH_DRIVERS_DRIVERS_H
#define _FSSH_DRIVERS_DRIVERS_H

#include "fssh_defs.h"
#include "fssh_fs_interface.h"


#ifdef __cplusplus
extern "C" {
#endif

/* ---
	these hooks are how the kernel accesses the device
--- */

typedef fssh_status_t (*fssh_device_open_hook) (const char *name,
								uint32_t flags, void **cookie);
typedef fssh_status_t (*fssh_device_close_hook) (void *cookie);
typedef fssh_status_t (*fssh_device_free_hook) (void *cookie);
typedef fssh_status_t (*fssh_device_control_hook) (void *cookie, uint32_t op,
								void *data, fssh_size_t len);
typedef fssh_status_t  (*fssh_device_read_hook) (void *cookie,
								fssh_off_t position, void *data,
								fssh_size_t *numBytes);
typedef fssh_status_t  (*fssh_device_write_hook) (void *cookie,
								fssh_off_t position, const void *data,
								fssh_size_t *numBytes);
typedef fssh_status_t (*fssh_device_select_hook) (void *cookie, uint8_t event,
								uint32_t ref, fssh_selectsync *sync);
typedef fssh_status_t (*fssh_device_deselect_hook) (void *cookie, uint8_t event,
								fssh_selectsync *sync);
typedef fssh_status_t (*fssh_device_read_pages_hook)(void *cookie,
								fssh_off_t position, const fssh_iovec *vec,
								fssh_size_t count, fssh_size_t *_numBytes);
typedef fssh_status_t (*fssh_device_write_pages_hook) (void *cookie,
								fssh_off_t position, const fssh_iovec *vec,
								fssh_size_t count, fssh_size_t *_numBytes);

#define	FSSH_B_CUR_DRIVER_API_VERSION	2

/* ---
	the device_hooks structure is a descriptor for the device, giving its
	entry points.
--- */

typedef struct {
	fssh_device_open_hook			open;			/* called to open the device */
	fssh_device_close_hook			close;			/* called to close the device */
	fssh_device_free_hook			free;			/* called to free the cookie */
	fssh_device_control_hook		control;		/* called to control the device */
	fssh_device_read_hook			read;			/* reads from the device */
	fssh_device_write_hook			write;			/* writes to the device */
	fssh_device_select_hook			select;			/* start select */
	fssh_device_deselect_hook		deselect;		/* stop select */
	fssh_device_read_pages_hook		read_pages;		/* scatter-gather physical read from the device */
	fssh_device_write_pages_hook	write_pages;	/* scatter-gather physical write to the device */
} fssh_device_hooks;

fssh_status_t		fssh_init_hardware(void);
const char		  **fssh_publish_devices(void);
fssh_device_hooks	*fssh_find_device(const char *name);
fssh_status_t		fssh_init_driver(void);
void				fssh_uninit_driver(void);	

extern int32_t	fssh_api_version;

enum {
	FSSH_B_GET_DEVICE_SIZE = 1,			/* get # bytes */
										/*   returns size_t in *data */

	FSSH_B_SET_DEVICE_SIZE,				/* set # bytes */
										/*   passed size_t in *data */

	FSSH_B_SET_NONBLOCKING_IO,			/* set to non-blocking i/o */

	FSSH_B_SET_BLOCKING_IO,				/* set to blocking i/o */

	FSSH_B_GET_READ_STATUS,				/* check if can read w/o blocking */
										/*   returns bool in *data */

	FSSH_B_GET_WRITE_STATUS,			/* check if can write w/o blocking */
										/*   returns bool in *data */

	FSSH_B_GET_GEOMETRY,				/* get info about device geometry */
										/*   returns struct geometry in *data */

	FSSH_B_GET_DRIVER_FOR_DEVICE,		/* get the path of the executable serving that device */

	FSSH_B_GET_PARTITION_INFO,			/* get info about a device partition */
										/*   returns struct partition_info in *data */

	FSSH_B_SET_PARTITION,				/* create a user-defined partition */

	FSSH_B_FORMAT_DEVICE,				/* low-level device format */

	FSSH_B_EJECT_DEVICE,				/* eject the media if supported */

	FSSH_B_GET_ICON,					/* return device icon (see struct below) */

	FSSH_B_GET_BIOS_GEOMETRY,			/* get info about device geometry */
										/* as reported by the bios */
										/*   returns struct geometry in *data */

	FSSH_B_GET_MEDIA_STATUS,			/* get status of media. */
										/* return fssh_status_t in *data: */
										/* B_NO_ERROR: media ready */
										/* B_DEV_NO_MEDIA: no media */
										/* B_DEV_NOT_READY: device not ready */
										/* B_DEV_MEDIA_CHANGED: media changed */
										/*  since open or last B_GET_MEDIA_STATUS */
										/* B_DEV_MEDIA_CHANGE_REQUESTED: user */
										/*  pressed button on drive */
										/* B_DEV_DOOR_OPEN: door open */
	
	FSSH_B_LOAD_MEDIA,					/* load the media if supported */
	
	FSSH_B_GET_BIOS_DRIVE_ID,			/* get bios id for this device */

	FSSH_B_SET_UNINTERRUPTABLE_IO,		/* prevent cntl-C from interrupting i/o */
	FSSH_B_SET_INTERRUPTABLE_IO,		/* allow cntl-C to interrupt i/o */

	FSSH_B_FLUSH_DRIVE_CACHE,			/* flush drive cache */

	FSSH_B_GET_PATH_FOR_DEVICE,			/* get the absolute path of the device */

	FSSH_B_GET_NEXT_OPEN_DEVICE = 1000,	/* iterate through open devices */
	FSSH_B_ADD_FIXED_DRIVER,			/* private */
	FSSH_B_REMOVE_FIXED_DRIVER,			/* private */

	FSSH_B_AUDIO_DRIVER_BASE = 8000,	/* base for codes in audio_driver.h */
	FSSH_B_MIDI_DRIVER_BASE = 8100,		/* base for codes in midi_driver.h */
	FSSH_B_JOYSTICK_DRIVER_BASE = 8200,	/* base for codes in joystick.h */
	FSSH_B_GRAPHIC_DRIVER_BASE = 8300,	/* base for codes in graphic_driver.h */

	FSSH_B_DEVICE_OP_CODES_END = 9999	/* end of Be-defined contol id's */
};

/* ---
	geometry structure for the B_GET_GEOMETRY opcode
--- */

typedef struct {
	uint32_t	bytes_per_sector;		/* sector size in bytes */
	uint32_t	sectors_per_track;		/* # sectors per track */
	uint32_t	cylinder_count;			/* # cylinders */
	uint32_t	head_count;				/* # heads */
	uint8_t		device_type;			/* type */
	bool		removable;				/* non-zero if removable */
	bool		read_only;				/* non-zero if read only */
	bool		write_once;				/* non-zero if write-once */
} fssh_device_geometry;


/* ---
	Be-defined device types returned by B_GET_GEOMETRY.  Use these if it makes
	sense for your device.
--- */

enum {
	FSSH_B_DISK = 0,					/* Hard disks, floppy disks, etc. */
	FSSH_B_TAPE,						/* Tape drives */
	FSSH_B_PRINTER,						/* Printers */
	FSSH_B_CPU,							/* CPU devices */
	FSSH_B_WORM,						/* Write-once, read-many devices */
	FSSH_B_CD,							/* CD ROMS */
	FSSH_B_SCANNER,						/* Scanners */
	FSSH_B_OPTICAL,						/* Optical devices */
	FSSH_B_JUKEBOX,						/* Jukeboxes */
	FSSH_B_NETWORK						/* Network devices */
};


/* ---
	partition_info structure used by B_GET_PARTITION_INFO and B_SET_PARTITION
--- */

typedef struct {
	fssh_off_t	offset;					/* offset (in bytes) */
	fssh_off_t	size;					/* size (in bytes) */
	int32_t		logical_block_size;		/* logical block size of partition */
	int32_t		session;				/* id of session */
	int32_t		partition;				/* id of partition */
	char		device[256];			/* path to the physical device */
} fssh_partition_info;

/* ---
	driver_path structure returned by the B_GET_DRIVER_FOR_DEVICE
--- */

typedef char	fssh_driver_path[256];


/* ---
	open_device_iterator structure used by the B_GET_NEXT_OPEN_DEVICE opcode
--- */

typedef struct {
	uint32_t	cookie;			/* must be set to 0 before iterating */
	char		device[256];	/* device path */	
} fssh_open_device_iterator;


/* ---
	icon structure for the B_GET_ICON opcode
--- */

typedef struct {
	int32_t	icon_size;			/* icon size requested */
	void	*icon_data;			/* where to put 'em (usually BBitmap->Bits()) */
} fssh_device_icon;


#ifdef __cplusplus
}
#endif

#endif /* _FSSH_DRIVERS_DRIVERS_H */
