/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __IDE_INTERNAL_H__
#define __IDE_INTERNAL_H__

/*
	Part of Open IDE bus manager

	Internal structures
*/


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
// SIM interface
#define IDE_SIM_MODULE_NAME "bus_managers/ide/sim/v1"


extern device_manager_info *pnp;


typedef struct ide_bus_info ide_bus_info;


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

// ide request 
typedef struct ata_request {

	struct ide_device_info *	device;
	scsi_ccb *					ccb;				// basic scsi request
	uint8 						is_write : 1;		// true for write request
	uint8 						uses_dma : 1;		// true if using dma
	uint8 						packet_irq : 1;		// true if waiting for command packet irq
} ata_request;


//void init_ata_request(ata_request *request, struct ide_device_info *device, scsi_ccb *ccb);

static inline void
init_ata_request(ata_request *request, struct ide_device_info *device, scsi_ccb *ccb)
{
	request->device = device;
	request->ccb = ccb;
	request->is_write = 0;
	request->uses_dma = 0;
	request->packet_irq = 0;
}


typedef struct ide_device_info {
	struct ide_bus_info *bus;

	uint8 use_LBA : 1;				// true for LBA, false for CHS
	uint8 use_48bits : 1;			// true for LBA48
	uint8 is_atapi : 1;				// true for ATAPI, false for ATA
	uint8 DMA_supported : 1;		// DMA supported
	uint8 DMA_enabled : 1;			// DMA enabled
	uint8 is_device1 : 1;			// true for slave, false for master

	uint8 last_lun;					// last LUN 

	uint8 DMA_failures;				// DMA failures in a row
	uint8 num_failed_send;			// number of consequetive send problems

	// next two error codes are copied to request on finish_request & co.
	uint8 subsys_status;			// subsystem status of current request
	uint32 new_combined_sense;		// emulated sense of current request

	// pending error codes
	uint32 combined_sense;			// emulated sense of device

	struct ide_device_info *other_device;	// other device on same bus

	// entry for scsi's exec_io request
	void (*exec_io)( struct ide_device_info *device, struct ata_request *request );

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

	// pio from here on
	bigtime_t pio_timeout;
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


// state of ide bus
typedef enum {
	ata_state_idle,				// not is using it
	ata_state_busy,				// got bus but no command issued yet
	ata_state_pio,				// bus is executing a PIO command
	ata_state_dma				// bus is executing a DMA command
} ata_bus_state;


struct ide_bus_info {

	// controller
	ide_controller_interface *controller;
	void *channel_cookie;

	// lock, used for changes of bus state
	spinlock lock;
	cpu_status prev_irq_state;

	ata_bus_state			state;		// current state of bus

	struct ata_request *	requestActive;
	struct ata_request *	requestFree;


	benaphore status_report_ben; // to lock when you report XPT about bus state
								// i.e. during requeue, resubmit or finished

	bool disconnected;			// true, if controller is lost

	scsi_bus scsi_cookie;		// cookie for scsi bus

	ide_bus_timer_info timer;	// timeout
	scsi_dpc_cookie irq_dpc;

	ide_device_info *active_device;
	ide_device_info *devices[2];
	ide_device_info *first_device;

	uchar path_id;

	device_node_handle node;		// our pnp node

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


// ata.c

void ata_select_device(ide_bus_info *bus, int device);
void ata_select(ide_device_info *device);
bool ata_is_device_present(ide_bus_info *bus, int device);
status_t ata_wait(ide_bus_info *bus, uint8 set, uint8 not_set, bool check_err, bigtime_t timeout);
status_t ata_wait_for_drq(ide_bus_info *bus);
status_t ata_wait_for_drqdown(ide_bus_info *bus);
status_t ata_wait_for_drdy(ide_bus_info *bus);
status_t ata_reset_bus(ide_bus_info *bus, bool *_devicePresent0, uint32 *_sigDev0, bool *_devicePresent1, uint32 *_sigDev1);
status_t ata_reset_device(ide_device_info *device, bool *_devicePresent);
status_t ata_send_command(ide_device_info *device, ata_request *request, bool need_drdy, uint32 timeout, ata_bus_state new_state);
status_t ata_finish_command(ide_device_info *device);

bool check_rw_error(ide_device_info *device, ata_request *request);
bool check_output(ide_device_info *device, bool drdy_required, int error_mask, bool is_write);

void ata_exec_read_write(ide_device_info *device, ata_request *request,	uint64 pos, size_t length, bool write);

void ata_dpc_DMA(ata_request *request);
void ata_dpc_PIO(ata_request *request);

void ata_exec_io(ide_device_info *device, ata_request *request);

status_t ata_read_infoblock(ide_device_info *device, bool isAtapi);

status_t configure_ata_device(ide_device_info *device);

// atapi.c
status_t configure_atapi_device(ide_device_info *device);
void send_packet(ide_device_info *device, ata_request *request, bool write);
void packet_dpc(ata_request *request);
void atapi_exec_io(ide_device_info *device, ata_request *request);


// basic_prot.c

// timeout in seconds

// channel_mgr.c

extern ide_for_controller_interface ide_for_controller_module;


// device_mgr.c

status_t scan_device(ide_device_info *device, bool isAtapi);
void destroy_device(ide_device_info *device);
ide_device_info *create_device(ide_bus_info *bus, bool is_device1);

status_t configure_device(ide_device_info *device, bool isAtapi);

// dma.c

bool prepare_dma(ide_device_info *device, ata_request *request);
void start_dma(ide_device_info *device, ata_request *request);
void start_dma_wait(ide_device_info *device, ata_request *request);
void start_dma_wait_no_lock(ide_device_info *device, ata_request *request);
bool finish_dma(ide_device_info *device);
void abort_dma(ide_device_info *device, ata_request *request);

bool configure_dma(ide_device_info *device);


// emulation.c

bool copy_sg_data(scsi_ccb *request, uint offset, uint req_size_limit,
	void *buffer, int size, bool to_buffer);
void ide_request_sense(ide_device_info *device, ata_request *request);


// pio.c

void prep_PIO_transfer(ide_device_info *device, ata_request *request);
status_t read_PIO_block(ata_request *request, int length);
status_t write_PIO_block(ata_request *request, int length);



// sync.c

// timeout in seconds (according to CAM)
void start_waiting(ide_bus_info *bus, uint32 timeout, int new_state);
void start_waiting_nolock(ide_bus_info *bus, uint32 timeout, int new_state);
void wait_for_sync(ide_bus_info *bus);
void cancel_irq_timeout(ide_bus_info *bus);


void ide_dpc(void *arg);
void access_finished(ide_bus_info *bus, ide_device_info *device);

status_t ide_irq_handler(ide_bus_info *bus, uint8 status);
status_t ide_timeout(timer *arg);


#endif	/* __IDE_INTERNAL_H__ */
