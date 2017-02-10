/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*
	Part of Open SCSI bus manager

	Queuing of SCSI request

	Some words about queuing, i.e. making sure that no request
	gets stuck in some queue forever

	As long as the SIM accepts new reqs, XPT doesn't take care of 
	stuck requests - thus, peripheral drivers should use synced reqs 
	once in a while (or even always) to force the HBA to finish 
	previously submitted reqs. This is also important for non-queuing
	HBAs as in this case the queuing is done by the SCSI bus manager
	which uses elevator sort.

	Requests can be blocked by SIM or by the SCSI bus manager on a per-device
	or per-bus basis. There are three possible reasons:
	
	1. the hardware queue is too small
	   - detected by bus manager automatically, or
	   - detected by SIM which calls "requeue" for affected request
	2. SIM blocks bus/device explictely
	3. bus manager blocks bus/device explictly (currently used for 
	   ordered requests)
	4. the device has queued requests or the bus has waiting devices resp.
	   
	The first condition is automatically cleared when a request has
	been completed, but can be reset manually by the SIM. In the second and
	third cases, the SIM/bus manager must explictely unblock the bus/device.
	For easier testing,	the lock_count is the sum of the overflow bit, the 
	SIM lock count and the bus manager lock count. The blocking test is in
	scsi_check_enqueue_request() in scsi_io.c.
	
	If a bus/device is blocked or has waiting requests/devices, new requests 
	are added to a device-specific request queue in an elevator-sort style, 
	taking care that no ordered	requests are overtaken. Exceptions are 
	requeued request and autosense-requests, which are added first. Each bus 
	has a queue of non-blocked devices that have waiting requests. If a 
	device is to be added to this queue, it is always appended at tail 
	to ensure fair processing.
*/

#include "scsi_internal.h"
#include "queuing.h"


// add request to device queue, using evelvator sort
static void scsi_insert_new_request( scsi_device_info *device, 
	scsi_ccb *new_request )
{
	scsi_ccb *first, *last, *before, *next;
	
	SHOW_FLOW( 3, "inserting new_request=%p, pos=%" B_PRId64, new_request,
		new_request->sort );
	
	first = device->queued_reqs;
	
	if( first == NULL ) {
		SHOW_FLOW0( 1, "no other queued request" );
		scsi_add_req_queue_first( new_request );
		return;
	}
	
	SHOW_FLOW( 3, "first=%p, pos=%" B_PRId64 ", last_pos=%" B_PRId64, 
		first, first->sort, device->last_sort );

	// don't let syncs bypass others
	if( new_request->ordered ) {
		SHOW_FLOW0( 1, "adding synced request to tail" );
		scsi_add_req_queue_last( new_request );
		return;
	}
	
	if( new_request->sort < 0 ) {
		SHOW_FLOW0( 1, "adding unsortable request to tail" );
		scsi_add_req_queue_last( new_request );
		return;
	}

	// to reduce head seek time, we have three goals:
	// - sort request accendingly according to head position
	//   as as disks use to read ahead and not backwards
	// - ordered accesses can neither get overtaken by or overtake other requests
	//
	// in general, we only have block position, so head, track or
	// whatever specific optimizations can only be done by the disks
	// firmware;
	//
	// thus, sorting is done ascendingly with only a few exceptions:
	// - if position of request to be inserted is between current
	//   (i.e. last) position and position of first queued request, 
	//   insert it as first queue entry; i.e. we get descending order
	// - if position of first queued request is before current position
	//   and position of new req is before first queued request, add it 
	//   as first queue entry; i.e. the new and the (previously) first 
	//   request are sorted monotically increasing
	//
	// the first exception should help if the queue is short (not sure
	// whether this actually hurts if we have a long queue), the
	// second one maximizes monotonic ranges
	last = first->prev;
	
	if( (device->last_sort <= new_request->sort &&
		 new_request->sort <= first->sort) ||
		(first->sort < device->last_sort &&
		 new_request->sort <= first->sort) ) 
	{
		// these are the exceptions described above
		SHOW_FLOW0( 3, "trying to insert req at head of device req queue" );
		
		// we should have a new first request, make sure we don't bypass syncs
		for( before = last; !before->ordered; ) {
			before = before->prev;
			if( before == last )
				break;
		}
		
		if( !before->ordered ) {
			SHOW_FLOW0( 1, "scheduled request in front of all other reqs of device" );
			scsi_add_req_queue_first( new_request );
			return;
		} else
			SHOW_FLOW0( 1, "req would bypass ordered request" );
	}
	
	// the insertion sort loop ignores ordered flag of last request, 
	// so check that here
	if( last->ordered ) {
		SHOW_FLOW0( 1, "last entry is ordered, adding new request as last" );
		scsi_add_req_queue_last( new_request );
		return;
	}
	
	SHOW_FLOW0( 3, "performing insertion sort" );

	// insertion sort starts with last entry to avoid unnecessary overtaking
	for( before = last->prev, next = last; 
		 before != last && !before->ordered; 
		 next = before, before = before->prev ) 
	{
		if( before->sort <= new_request->sort && new_request->sort <= next->sort )
			break;
	}
	
	// if we bumped into ordered request, append new request at tail
	if( before->ordered ) {
		SHOW_FLOW0( 1, "overtaking ordered request in sorting - adding as last" );
		scsi_add_req_queue_last( new_request );
		return;
	}
	
	SHOW_FLOW( 1, "inserting after %p (pos=%" B_PRId64 ") and before %p (pos=%" B_PRId64 ")", 
		before, before->sort, next, next->sort );

	// if we haven't found a proper position, we automatically insert 
	// new request as last because request list is circular;
	// don't check whether we added request as first as this is impossible
	new_request->next = next;
	new_request->prev = before;
	next->prev = new_request;
	before->next = new_request;
}


// add request to end of device queue and device to bus queue
// used for normal requests
void scsi_add_queued_request( scsi_ccb *request )
{
	scsi_device_info *device = request->device;
	
	SHOW_FLOW0( 3, "" );

	request->state = SCSI_STATE_QUEUED;
	scsi_insert_new_request( device, request );
	
/*	{
		scsi_ccb *tmp = device->queued_reqs;
		
		dprintf( "pos=%Ld, to_insert=%Ld; ", device->last_sort,
			request->sort );
		
		do {
			dprintf( "%Ld, %s", tmp->sort, tmp->next == device->queued_reqs ? "\n" : "" );
			tmp = tmp->next;
		} while( tmp != device->queued_reqs );
	}*/

	// if device is not deliberately locked, mark it as waiting
	if( device->lock_count == 0 ) {
		SHOW_FLOW0( 3, "mark device as waiting" );
		scsi_add_device_queue_last( device );
	}
}


// add request to begin of device queue and device to bus queue
// used only for auto-sense request
void scsi_add_queued_request_first( scsi_ccb *request )
{
	scsi_device_info *device = request->device;
	
	SHOW_FLOW0( 3, "" );

	request->state = SCSI_STATE_QUEUED;
	scsi_add_req_queue_first( request );

	// if device is not deliberately locked, mark it as waiting
	if( device->lock_count == 0 ) {
		SHOW_FLOW0( 3, "mark device as waiting" );
		// make device first in bus queue to execute sense ASAP
		scsi_add_device_queue_first( device );
	}
}


// remove requests from queue, removing device from queue if idle
void scsi_remove_queued_request( scsi_ccb *request )
{
	scsi_remove_req_queue( request );
	
	if( request->device->queued_reqs == NULL )
		scsi_remove_device_queue( request->device );
}


// explictely unblock bus
static void scsi_unblock_bus_int( scsi_bus_info *bus, bool by_SIM )
{
	bool was_servicable, start_retry;
	
	SHOW_FLOW0( 3, "" );

	ACQUIRE_BEN( &bus->mutex );
	
	was_servicable = scsi_can_service_bus( bus );

	scsi_unblock_bus_noresume( bus, by_SIM );
	
	start_retry = !was_servicable && scsi_can_service_bus( bus );

	RELEASE_BEN( &bus->mutex );
	
	if( start_retry )
		release_sem( bus->start_service );
}


// explicitely unblock bus as requested by SIM
void scsi_unblock_bus( scsi_bus_info *bus )
{
	scsi_unblock_bus_int( bus, true );
}


// explicitly unblock device
static void scsi_unblock_device_int( scsi_device_info *device, bool by_SIM )
{
	scsi_bus_info *bus = device->bus;
	bool was_servicable, start_retry;
	
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( &bus->mutex );
	
	was_servicable = scsi_can_service_bus( bus );

	scsi_unblock_device_noresume( device, by_SIM );
	
	// add to bus queue if not locked explicitly anymore and requests are waiting
	if( device->lock_count == 0 && device->queued_reqs != NULL )
		scsi_add_device_queue_last( device );
	
	start_retry = !was_servicable && scsi_can_service_bus( bus );

	RELEASE_BEN( &bus->mutex );
	
	if( start_retry )
		release_sem( bus->start_service );
}


// explicitely unblock device as requested by SIM
void scsi_unblock_device( scsi_device_info *device )
{
	return scsi_unblock_device_int( device, true );
}


// SIM signals that it can handle further requests for this bus
void scsi_cont_send_bus( scsi_bus_info *bus )
{
	bool was_servicable, start_retry;
	
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( &bus->mutex );
	
	was_servicable = scsi_can_service_bus( bus );

	scsi_clear_bus_overflow( bus );	
	
	start_retry = !was_servicable && scsi_can_service_bus( bus );
				
	RELEASE_BEN( &bus->mutex );

	if( start_retry )
		release_sem_etc( bus->start_service, 1, 0/*B_DO_NOT_RESCHEDULE*/ );
}


// SIM signals that it can handle further requests for this device
void scsi_cont_send_device( scsi_device_info *device )
{
	scsi_bus_info *bus = device->bus;
	bool was_servicable, start_retry;
	
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( &bus->mutex );
	
	was_servicable = scsi_can_service_bus( bus );
	
	if( device->sim_overflow ) {			
		device->sim_overflow = false;
		--device->lock_count;
		
		// add to bus queue if not locked explicitly anymore and requests are waiting
		if( device->lock_count == 0 && device->queued_reqs != NULL )
			scsi_add_device_queue_last( device );
	}

	// no device overflow implicits no bus overflow
	// (and if not, we'll detect that on next submit)
	scsi_clear_bus_overflow( bus );
	
	start_retry = !was_servicable && scsi_can_service_bus( bus );
				
	RELEASE_BEN( &bus->mutex );
	
	// tell service thread if there are pending requests which
	// weren't pending before	
	if( start_retry )
		release_sem_etc( bus->start_service, 1, 0/*B_DO_NOT_RESCHEDULE*/ );
}


// explicitly block bus
static void scsi_block_bus_int( scsi_bus_info *bus, bool by_SIM )
{
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( &bus->mutex );

	scsi_block_bus_nolock( bus, by_SIM );
	
	RELEASE_BEN( &bus->mutex );
}


// explicitly block bus as requested by SIM
void scsi_block_bus( scsi_bus_info *bus )
{
	return scsi_block_bus_int( bus, true );
}


// explicitly block device
static void scsi_block_device_int( scsi_device_info *device, bool by_SIM )
{
	scsi_bus_info *bus = device->bus;
	
	SHOW_FLOW0( 3, "" );
	
	ACQUIRE_BEN( &bus->mutex );

	scsi_block_device_nolock( device, by_SIM );

	// remove device from bus queue as it cannot be processed anymore
	scsi_remove_device_queue( device );
	
	RELEASE_BEN( &bus->mutex );
}


// explicitly block device as requested by SIM
void scsi_block_device( scsi_device_info *device )
{
	return scsi_block_device_int( device, true );
}
