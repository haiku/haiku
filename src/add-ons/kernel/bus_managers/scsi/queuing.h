/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*
	Part of Open SCSI bus manager

	Handling of bus/device blocking. Inline functions defined
	here don't wake service thread if required and don't lock bus, so 
	everyone using them must take care of that himself.
*/

#ifndef __BLOCKING_H__
#define __BLOCKING_H__

#include "dl_list.h"

void scsi_add_queued_request( scsi_ccb *request );
void scsi_add_queued_request_first( scsi_ccb *request );
void scsi_remove_queued_request( scsi_ccb *request );


static inline void scsi_add_req_queue_first( scsi_ccb *request )
{
	scsi_device_info *device = request->device;

	SHOW_FLOW( 3, "request=%p", request );

	ADD_CDL_LIST_HEAD( request, scsi_ccb, device->queued_reqs, );
}

static inline void scsi_add_req_queue_last( scsi_ccb *request )
{
	scsi_device_info *device = request->device;

	SHOW_FLOW( 3, "request=%p", request );

	ADD_CDL_LIST_TAIL( request, scsi_ccb, device->queued_reqs, );
}

static inline void scsi_remove_req_queue( scsi_ccb *request )
{
	scsi_device_info *device = request->device;
	
	SHOW_FLOW( 3, "request=%p", request );
	
	REMOVE_CDL_LIST( request, device->queued_reqs, );
}

// ignored, if device is already queued
static inline void scsi_add_device_queue_first( scsi_device_info *device )
{
	SHOW_FLOW0( 3, "" );

	if( DEVICE_IN_WAIT_QUEUE( device ))
		return;

	SHOW_FLOW0( 3, "was not in wait queue - adding" );
		
	ADD_CDL_LIST_HEAD( device, scsi_device_info, device->bus->waiting_devices, waiting_ );	
}

// ignored, if device is already queued
static inline void scsi_add_device_queue_last( scsi_device_info *device )
{
	SHOW_FLOW0( 3, "" );
	
	if( DEVICE_IN_WAIT_QUEUE( device ))
		return;
		
	SHOW_FLOW0( 3, "was not in wait queue - adding" );

	ADD_CDL_LIST_TAIL( device, scsi_device_info, device->bus->waiting_devices, waiting_ );
}

// ignored, if device is not in queue
static inline void scsi_remove_device_queue( scsi_device_info *device )
{
	SHOW_FLOW0( 3, "" );
	
	if( !DEVICE_IN_WAIT_QUEUE( device ))
		return;
		
	SHOW_FLOW0( 3, "was in wait queue - removing from it" );

	REMOVE_CDL_LIST( device, device->bus->waiting_devices, waiting_ );
	
	// reset next link so we can see that it's not in device queue
	device->waiting_next = NULL;
}

// set overflow bit of device; this will not remove device from bus queue!
// (multiple calls are ignored gracefully)
static inline void scsi_set_device_overflow( scsi_device_info *device )
{
	device->lock_count += device->sim_overflow ^ 1;
	device->sim_overflow = 1;
}

// set overflow bit of bus
// (multiple calls are ignored gracefully)
static inline void scsi_set_bus_overflow( scsi_bus_info *bus )
{
	bus->lock_count += bus->sim_overflow ^ 1;
	bus->sim_overflow = 1;
}

// clear overflow bit of device; this will not add device to bus queue!
// (multiple calls are ignored gracefully)
static inline void scsi_clear_device_overflow( scsi_device_info *device )
{
	device->lock_count -= device->sim_overflow;
	device->sim_overflow = 0;
}

// clear overflow bit of bus
// (multiple calls are ignored gracefully)
static inline void scsi_clear_bus_overflow( scsi_bus_info *bus )
{
	bus->lock_count -= bus->sim_overflow;
	bus->sim_overflow = 0;
}

// check whether bus has some pending requests it can process now
static inline bool scsi_can_service_bus( scsi_bus_info *bus )
{
	// bus must not be blocked and requests pending
	return (bus->lock_count == 0) & (bus->waiting_devices != NULL);
}

// unblock bus
// lock must be hold; service thread is not informed
static inline void scsi_unblock_bus_noresume( scsi_bus_info *bus, bool by_SIM )
{
	if( bus->blocked[by_SIM] > 0 ) {
		--bus->blocked[by_SIM];
		--bus->lock_count;
	} else {
		panic( "Tried to unblock bus %d which wasn't blocked", 
			bus->path_id );
	}
}


// unblock device
// lock must be hold; device is not added to queue and service thread is not informed
static inline void scsi_unblock_device_noresume( scsi_device_info *device, bool by_SIM )
{
	if( device->blocked[by_SIM] > 0 ) {
		--device->blocked[by_SIM];
		--device->lock_count;
	} else {
		panic( "Tried to unblock device %d/%d/%d which wasn't blocked", 
			device->bus->path_id, device->target_id, device->target_lun );
	}
}


// block bus
// lock must be hold
static inline void scsi_block_bus_nolock( scsi_bus_info *bus, bool by_SIM )
{
	++bus->blocked[by_SIM];
	++bus->lock_count;
}


// block device
// lock must be hold
static inline void scsi_block_device_nolock( scsi_device_info *device, bool by_SIM )
{
	++device->blocked[by_SIM];
	++device->lock_count;

	// remove device from bus queue as it cannot be processed anymore
	scsi_remove_device_queue( device );
}


void scsi_block_bus( scsi_bus_info *bus );
void scsi_unblock_bus( scsi_bus_info *bus );
void scsi_block_device( scsi_device_info *device );
void scsi_unblock_device( scsi_device_info *device );

void scsi_cont_send_bus( scsi_bus_info *bus );
void scsi_cont_send_device( scsi_device_info *device );

#endif
