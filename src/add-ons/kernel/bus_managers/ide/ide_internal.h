/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __IDE_INTERNAL_H__
#define __IDE_INTERNAL_H__


#include <sys/cdefs.h>

#include <bus/IDE.h>
#include <bus/SCSI.h>
#include "ide_device_infoblock.h"
#include <ide_types.h>
#include <device_manager.h>

#define debug_level_error 2
#define debug_level_info 1
#define debug_level_flow 0

#define DEBUG_MSG_PREFIX "IDE -- "

#include "wrapper.h"


#define IDE_STD_TIMEOUT 10
#define IDE_RELEASE_TIMEOUT 10000000

// number of timeouts before we disable DMA automatically
#define MAX_DMA_FAILURES 3

// name of pnp generator of channel ids
#define IDE_CHANNEL_ID_GENERATOR "ide/channel_id"
// node item containing channel id (uint32)
#define IDE_CHANNEL_ID_ITEM "ide/channel_id"

extern device_manager_info *pnp;


typedef struct ide_bus_info ide_bus_info;

typedef void (*ide_synced_pc_func)(ide_bus_info *bus, void *arg);

typedef struct ide_synced_pc {
	struct ide_synced_pc *next;
	ide_synced_pc_func func;
	void *arg;
	bool registered;
} ide_synced_pc;

// structure for device time-outs
typedef struct ide_device_timer_info {
	timer te;
	struct ide_device_info *device;
} ide_device_timer_info;

// structure for bus time-outs
typedef struct ide_bus_timer_info {
	timer te;
	struct ide_bus_info *bus;
} ide_bus_timer_info;


typedef struct ide_device_info {
	struct ide_bus_info *bus;

	uint8 use_LBA : 1;				// true for LBA, false for CHS
	uint8 use_48bits : 1;			// true for LBA48
	uint8 is_atapi : 1;				// true for ATAPI, false for ATA
	uint8 CQ_supported : 1;			// Command Queuing supported
	uint8 CQ_enabled : 1;			// Command Queuing enabled
	uint8 DMA_supported : 1;		// DMA supported
	uint8 DMA_enabled : 1;			// DMA enabled
	uint8 is_device1 : 1;			// true for slave, false for master

	uint8 queue_depth;				// maximum Command Queueing depth

	uint8 last_lun;					// last LUN

	uint8 DMA_failures;				// DMA failures in a row
	uint8 CQ_failures;				// Command Queuing failures during _last_ command
	uint8 num_failed_send;			// number of consequetive send problems

	// next two error codes are copied to request on finish_request & co.
	uint8 subsys_status;			// subsystem status of current request
	uint32 new_combined_sense;		// emulated sense of current request

	// pending error codes
	uint32 combined_sense;			// emulated sense of device

	struct ide_qrequest *qreq_array;	// array of ide requests
	struct ide_qrequest *free_qrequests;	// free list
	int num_running_reqs;			// number of running requests

	struct ide_device_info *other_device;	// other device on same bus

	// entry for scsi's exec_io request
	void (*exec_io)( struct ide_device_info *device, struct ide_qrequest *qrequest );

	int target_id;					// target id (currently, same as is_device1)

	ide_reg_mask tf_param_mask;		// flag of valid bytes in task file
	ide_task_file tf;				// task file

	// ata from here on
	uint64 total_sectors;			// size in sectors

	// atapi from here on
	uint8 packet[12];				// atapi command packet

	struct {
		uint8 packet_irq : 1;		// true, if command packet irq required
		bigtime_t packet_irq_timeout;	// timeout for it
	} atapi;

	uint8 device_type;				// atapi device type

	bool reconnect_timer_installed;	// true, if reconnect timer is running
	ide_device_timer_info reconnect_timer;	// reconnect timeout
	scsi_dpc_cookie reconnect_timeout_dpc;	// dpc fired by timeout
	ide_synced_pc reconnect_timeout_synced_pc;	// spc fired by dpc

	// pio from here on
	int left_sg_elem;				// remaining sg elements
	const physical_entry *cur_sg_elem;	// active sg element
	int cur_sg_ofs;					// offset in active sg element

	int left_blocks;				// remaining blocks

	bool has_odd_byte;				// remaining odd byte
	int odd_byte;					// content off odd byte

	ide_device_infoblock infoblock;	// infoblock of device
} ide_device_info;


/*// state as stored in sim_state of scsi_ccb
typedef enum {
	ide_request_normal = 0,			// this must be zero as this is initial value
	ide_request_start_autosense = 1,
	ide_request_autosense = 2
} ide_request_state;*/

// ide request
typedef struct ide_qrequest {
	struct ide_qrequest *next;
	ide_device_info *device;
	scsi_ccb *request;			// basic request

	uint8 is_write : 1;			// true for write request
	uint8 running : 1;			// true if "on bus"
	uint8 uses_dma : 1;			// true if using dma
	uint8 packet_irq : 1;		// true if waiting for command packet irq
	uint8 queuable : 1;			// true if command queuing is used
	uint8 tag;					// command queuing tag
} ide_qrequest;


// state of ide bus
typedef enum {
	ide_state_idle,				// noone is using it, but overlapped
								// commands may be pending
	ide_state_accessing,		// bus is in use
	ide_state_async_waiting,	// waiting for IRQ, to be reported via irq_dpc
	ide_state_sync_waiting,		// waiting for IRQ, to be reported via sync_wait_sem
} ide_bus_state;

struct ide_bus_info {
	ide_qrequest *active_qrequest;

	// controller
	ide_controller_interface *controller;
	void *channel_cookie;

	// lock, used for changes of bus state
	spinlock lock;
	cpu_status prev_irq_state;

	ide_bus_state state;		// current state of bus

	mutex status_report_ben; 	// to lock when you report XPT about bus state
								// i.e. during requeue, resubmit or finished

	bool disconnected;			// true, if controller is lost
	int num_running_reqs;		// total number of running requests

	scsi_bus scsi_cookie;		// cookie for scsi bus

	ide_bus_timer_info timer;	// timeout
	scsi_dpc_cookie irq_dpc;
	ide_synced_pc *synced_pc_list;

	sem_id sync_wait_sem;		// released when sync_wait finished
	bool sync_wait_timeout;		// true, if timeout occured

	ide_device_info *active_device;
	ide_device_info *devices[2];
	ide_device_info *first_device;

	ide_synced_pc scan_bus_syncinfo;	// used to start bus scan
	sem_id scan_device_sem;		// released when device has been scanned

	ide_synced_pc disconnect_syncinfo;	// used to handle lost controller

	uchar path_id;

	device_node *node;		// our pnp node

	// restrictions, read from controller node
	uint8 max_devices;
	uint8 can_DMA;
	uint8 can_CQ;

	char name[32];
};


// call this before you change bus state
#define IDE_LOCK( bus ) { \
	cpu_status prev_irq_state = disable_interrupts(); \
	acquire_spinlock( &bus->lock ); \
	bus->prev_irq_state = prev_irq_state; \
}

// call this after you changed bus state
#define IDE_UNLOCK( bus ) { \
	cpu_status prev_irq_state = bus->prev_irq_state; \
	release_spinlock( &bus->lock ); \
	restore_interrupts( prev_irq_state ); \
}

// SIM interface
#define IDE_SIM_MODULE_NAME "bus_managers/ide/sim/v1"


enum {
	ev_ide_send_command = 1,
	ev_ide_device_start_service,
	ev_ide_device_start_service2,
	ev_ide_dpc_service,
	ev_ide_dpc_continue,
	ev_ide_irq_handle,
	ev_ide_cancel_irq_timeout,
	ev_ide_start_waiting,
	ev_ide_timeout_dpc,
	ev_ide_timeout,
	ev_ide_reset_bus,
	ev_ide_reset_device,

	ev_ide_scsi_io,
	ev_ide_scsi_io_exec,
	ev_ide_scsi_io_invalid_device,
	ev_ide_scsi_io_bus_busy,
	ev_ide_scsi_io_device_busy,
	ev_ide_scsi_io_disconnected,
	ev_ide_finish_request,
	ev_ide_finish_norelease,

	ev_ide_scan_device_int,
	ev_ide_scan_device_int_cant_send,
	ev_ide_scan_device_int_keeps_busy,
	ev_ide_scan_device_int_found
};



// get selected device
static inline ide_device_info *
get_current_device(ide_bus_info *bus)
{
	ide_task_file tf;

	bus->controller->read_command_block_regs(bus->channel_cookie, &tf,
		ide_mask_device_head);

	return bus->devices[tf.lba.device];
}


// check if device has released the bus
// return: true, if bus was released
static inline int
device_released_bus(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;

	bus->controller->read_command_block_regs(bus->channel_cookie,
		&device->tf, ide_mask_sector_count);

	return device->tf.queued.release;
}


__BEGIN_DECLS


// ata.c

bool check_rw_error(ide_device_info *device, ide_qrequest *qrequest);
bool check_output(ide_device_info *device, bool drdy_required, int error_mask, bool is_write);

bool prep_ata(ide_device_info *device);
void enable_CQ(ide_device_info *device, bool enable);
void ata_send_rw(ide_device_info *device, ide_qrequest *qrequest,
	uint64 pos, size_t length, bool write);

void ata_dpc_DMA(ide_qrequest *qrequest);
void ata_dpc_PIO(ide_qrequest *qrequest);

void ata_exec_io(ide_device_info *device, ide_qrequest *qrequest);


// atapi.c

bool prep_atapi(ide_device_info *device);

void send_packet(ide_device_info *device, ide_qrequest *qrequest, bool write);
void packet_dpc(ide_qrequest *qrequest);
void atapi_exec_io(ide_device_info *device, ide_qrequest *qrequest);


// basic_prot.c

bool ide_wait(ide_device_info *device, int mask, int not_mask, bool check_err,
	bigtime_t timeout);
bool wait_for_drq(ide_device_info *device);
bool wait_for_drqdown(ide_device_info *device);
bool wait_for_drdy(ide_device_info *device);

// timeout in seconds
bool send_command(ide_device_info *device, ide_qrequest *qrequest,
	bool need_drdy, uint32 timeout, ide_bus_state new_state);
bool device_start_service( ide_device_info *device, int *tag);
bool reset_device(ide_device_info *device, ide_qrequest *ignore);
bool reset_bus(ide_device_info *device, ide_qrequest *ignore);

bool check_service_req(ide_device_info *device);


// channel_mgr.c

extern ide_for_controller_interface ide_for_controller_module;


// device_mgr.c

void scan_device_worker(ide_bus_info *bus, void *arg);


// dma.c

bool prepare_dma(ide_device_info *device, ide_qrequest *qrequest);
void start_dma(ide_device_info *device, ide_qrequest *qrequest);
void start_dma_wait(ide_device_info *device, ide_qrequest *qrequest);
void start_dma_wait_no_lock(ide_device_info *device, ide_qrequest *qrequest);
bool finish_dma(ide_device_info *device);
void abort_dma(ide_device_info *device, ide_qrequest *qrequest);

bool configure_dma(ide_device_info *device);
int get_device_dma_mode(ide_device_info *device);

// emulation.c

bool copy_sg_data(scsi_ccb *request, uint offset, uint req_size_limit,
	void *buffer, int size, bool to_buffer);
void ide_request_sense(ide_device_info *device, ide_qrequest *qrequest);


// pio.c

void prep_PIO_transfer(ide_device_info *device, ide_qrequest *qrequest);
status_t read_PIO_block(ide_qrequest *qrequest, int length);
status_t write_PIO_block(ide_qrequest *qrequest, int length);


// queuing.c

bool send_abort_queue(ide_device_info *device);
bool try_service(ide_device_info *device);

void reconnect_timeout_worker(ide_bus_info *bus, void *arg);
int32 reconnect_timeout(timer *arg);

bool initialize_qreq_array(ide_device_info *device, int queue_depth);
void destroy_qreq_array(ide_device_info *device);


// sync.c

// timeout in seconds (according to CAM)
void start_waiting(ide_bus_info *bus, uint32 timeout, int new_state);
void start_waiting_nolock(ide_bus_info *bus, uint32 timeout, int new_state);
void wait_for_sync(ide_bus_info *bus);
void cancel_irq_timeout(ide_bus_info *bus);

status_t schedule_synced_pc(ide_bus_info *bus, ide_synced_pc *pc, void *arg);
void init_synced_pc(ide_synced_pc *pc, ide_synced_pc_func func);
void uninit_synced_pc(ide_synced_pc *pc);

void ide_dpc(void *arg);
void access_finished(ide_bus_info *bus, ide_device_info *device);

status_t ide_irq_handler(ide_bus_info *bus, uint8 status);
status_t ide_timeout(timer *arg);


__END_DECLS


#endif	/* __IDE_INTERNAL_H__ */
