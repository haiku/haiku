/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <bus/SCSI.h>

#define TRACE(a...) dprintf("ahci: " a)
#define FLOW(a...)	dprintf("ahci: " a)



/*

// SIM interface
// SCSI controller drivers must provide this interface
typedef struct scsi_sim_interface {
	driver_module_info info;

	// execute request
	void (*scsi_io)( scsi_sim_cookie cookie, scsi_ccb *ccb );
	// abort request
	uchar (*abort)( scsi_sim_cookie cookie, scsi_ccb *ccb_to_abort );
	// reset device
	uchar (*reset_device)( scsi_sim_cookie cookie, uchar target_id, uchar target_lun );
	// terminate request
	uchar (*term_io)( scsi_sim_cookie cookie, scsi_ccb *ccb_to_terminate );

	// get information about bus
	uchar (*path_inquiry)( scsi_sim_cookie cookie, scsi_path_inquiry *inquiry_data );
	// scan bus
	// this is called immediately before the SCSI bus manager scans the bus
	uchar (*scan_bus)( scsi_sim_cookie cookie );
	// reset bus
	uchar (*reset_bus)( scsi_sim_cookie cookie );
	
	// get restrictions of one device 
	// (used for non-SCSI transport protocols and bug fixes)
	void (*get_restrictions)(
		scsi_sim_cookie 	cookie,
		uchar				target_id,		// target id
		bool				*is_atapi, 		// set to true if this is an ATAPI device that
											// needs some commands emulated
		bool				*no_autosense,	// set to true if there is no autosense;
											// the SCSI bus manager will request sense on
											// SCSI_REQ_CMP_ERR/SCSI_DEVICE_CHECK_CONDITION
		uint32 				*max_blocks );	// maximum number of blocks per transfer if > 0;
											// used for buggy devices that cannot handle
											// large transfers (read: ATAPI ZIP drives)

	status_t (*ioctl)(scsi_sim_cookie, uint8 targetID, uint32 op, void *buffer, size_t length);
} scsi_sim_interface;

*/
