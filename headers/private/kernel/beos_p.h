/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _BEOS_PRIV_H
#define _BEOS_PRIV_H

#include <kernel.h>
#include <types.h>
#include <ktypes.h>

typedef int status_t;
typedef unsigned short ushort;

/*
** NOTE:
** Contains stuff taken from beos headers.
** Please dont sue me. I'll clean it up when I get the chance.
*/

/* beos driver interface */
struct selectsync;
typedef struct selectsync selectsync;

typedef status_t (*device_open_hook) (const char *name, uint32 flags, void **cookie);
typedef status_t (*device_close_hook) (void *cookie);
typedef status_t (*device_free_hook) (void *cookie);
typedef status_t (*device_control_hook) (void *cookie, uint32 op, void *data, size_t len);
typedef status_t (*device_read_hook) (void *cookie, off_t position, void *data, size_t *numBytes);
typedef status_t (*device_write_hook) (void *cookie, off_t position, const void *data, size_t *numBytes);
typedef status_t (*device_select_hook) (void *cookie, uint8 event, uint32 ref, selectsync *sync);
typedef status_t (*device_deselect_hook) (void *cookie, uint8 event, selectsync *sync);
typedef status_t (*device_readv_hook) (void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes);
typedef status_t (*device_writev_hook) (void *cookie, off_t position, const iovec *vec, size_t count, size_t *numBytes);

#define	B_CUR_DRIVER_API_VERSION	2

typedef struct {
	device_open_hook		open;
	device_close_hook		close;
	device_free_hook		free;
	device_control_hook		control;
	device_read_hook		read;
	device_write_hook		write;
	device_select_hook		select;
	device_deselect_hook	deselect;
	device_readv_hook		readv;
	device_writev_hook		writev;
} beos_device_hooks;

/* sem stuff */
#define B_CAN_INTERRUPT 1
#define B_DO_NOT_RESCHEDULE 2
#define B_CHECK_PERMISSION 4
#define B_TIMEOUT 8
#define	B_RELATIVE_TIMEOUT 8
#define B_ABSOLUTE_TIMEOUT 16

/* module stuff */
struct module_info {
	const char	*name;
	uint32		flags;
	int	        (*std_ops)(int32, ...);
};
typedef struct module_info module_info;

struct bus_manager_info {
	module_info		minfo;
	status_t		(*rescan)(void);
};
typedef struct bus_manager_info bus_manager_info;

/* isa module */
typedef struct {
	ulong	address;				/* memory address (little endian!) 4 bytes */
	ushort	transfer_count;			/* # transfers minus one (little endian!) 2 bytes*/
	uchar	reserved;				/* filler, 1byte*/
    uchar   flag;					/* end of link flag, 1byte */
} isa_dma_entry;

struct isa_module_info {
	bus_manager_info	binfo;

	uint8			(*read_io_8) (int mapped_io_addr);
	void			(*write_io_8) (int mapped_io_addr, uint8 value);
	uint16			(*read_io_16) (int mapped_io_addr);
	void			(*write_io_16) (int mapped_io_addr, uint16 value);
	uint32			(*read_io_32) (int mapped_io_addr);
	void			(*write_io_32) (int mapped_io_addr, uint32 value);

	void *			(*ram_address) (const void *physical_address_in_system_memory);

	long			(*make_isa_dma_table) (
							const void		*buffer,		/* buffer to make a table for */
							long			buffer_size,	/* buffer size */
							ulong			num_bits,		/* dma transfer size that will be used */
							isa_dma_entry	*table,			/* -> caller-supplied scatter/gather table */
							long			num_entries		/* max # entries in table */
						);
	long			(*start_isa_dma) (
							long	channel,				/* dma channel to use */
							void	*buf,					/* buffer to transfer */
							long	transfer_count,			/* # transfers */
							uchar	mode,					/* mode flags */
							uchar	e_mode					/* extended mode flags */
						);
	long			(*start_scattered_isa_dma) (
							long				channel,	/* channel # to use */
							const isa_dma_entry	*table,		/* physical address of scatter/gather table */
							uchar				mode,		/* mode flags */
							uchar				emode		/* extended mode flags */
						);
	long			(*lock_isa_dma_channel) (long channel);
	long			(*unlock_isa_dma_channel) (long channel);
};
typedef struct isa_module_info isa_module_info;
#define	B_ISA_MODULE_NAME		"bus_managers/isa/v1"

/* beos error codes */

#define LONG_MIN		(-2147483647L-1)

#define B_GENERAL_ERROR_BASE		LONG_MIN
#define B_OS_ERROR_BASE				B_GENERAL_ERROR_BASE + 0x1000

enum {
	B_NO_MEMORY = B_GENERAL_ERROR_BASE,
	B_IO_ERROR,
	B_PERMISSION_DENIED,
	B_BAD_INDEX,
	B_BAD_TYPE,
	B_BAD_VALUE,
	B_MISMATCHED_VALUES,
	B_NAME_NOT_FOUND,
	B_NAME_IN_USE,
	B_TIMED_OUT,
    B_INTERRUPTED,
	B_WOULD_BLOCK,
    B_CANCELED,
	B_NO_INIT,
	B_BUSY,
	B_NOT_ALLOWED,
	B_BAD_DATA,

	B_ERROR = -1,
	B_OK = 0,
	B_NO_ERROR = 0
};
enum {
	B_BAD_SEM_ID = B_OS_ERROR_BASE,
	B_NO_MORE_SEMS,

	B_BAD_THREAD_ID = B_OS_ERROR_BASE + 0x100,
	B_NO_MORE_THREADS,
	B_BAD_THREAD_STATE,
	B_BAD_TEAM_ID,
	B_NO_MORE_TEAMS,

	B_BAD_PORT_ID = B_OS_ERROR_BASE + 0x200,
	B_NO_MORE_PORTS,

	B_BAD_IMAGE_ID = B_OS_ERROR_BASE + 0x300,
	B_BAD_ADDRESS,
	B_NOT_AN_EXECUTABLE,
	B_MISSING_LIBRARY,
	B_MISSING_SYMBOL,

	B_DEBUGGER_ALREADY_INSTALLED = B_OS_ERROR_BASE + 0x400
};


/*
 * BeOS functions protypes, same functions as BeOS but with
 * '_beos_' prepended to the names, the kernel loader
 * handles the magick of matching the names
 */
int _beos_atomic_add(volatile int *val, int incr);
int _beos_atomic_and(volatile int *val, int incr);
int _beos_atomic_or(volatile int *val, int incr);
int _beos_acquire_sem(sem_id id);
int _beos_acquire_sem_etc(sem_id id, uint32 count, uint32 flags, bigtime_t timeout);
sem_id _beos_create_sem(uint32 count, const char *name);
int _beos_delete_sem(sem_id id);
int _beos_get_sem_count(sem_id id, int32 *count);
int _beos_release_sem(sem_id id);
int _beos_release_sem_etc(sem_id id, int32 count, uint32 flags);
int _beos_strcmp(const char *cs, const char *ct);
void _beos_spin(bigtime_t microseconds);
int _beos_get_module(const char *path, module_info **vec);
int _beos_put_module(const char *path);

#endif
