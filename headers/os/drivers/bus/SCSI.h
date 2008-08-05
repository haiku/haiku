/*
 * Copyright 2004-2008, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_BUSMANAGER_H
#define _SCSI_BUSMANAGER_H

/*
	SCSI bus manager interface

	The bus manager interface is _based_ on CAM, but I've modified it because :-
	- HBA engine, target mode and queue freezing (and probably other features)
	  aren't supported (at least the first two aren't supported by linux too ;)
	- Asynchronous events aren't supported (no OS/driver I know uses them)
	- P/T/L was defined by number not by handle, requiring many redundant tests
	  and thus making adding/removing of devices/busses very hard, especially if
	  PnP is to be supported
	- single entry system as proposed by CAM involves extra tests and overhead
	  because of generalized data structure


	For peripheral driver writers:

	Something about requests involving data transfer: you can either specify
	the virtual address in <data> of CCB (in which case it must be continuous),
	or store a pointer to a S/G list that contains physical addresses in
	<sg_list>/<sg_count>. If <sg_list> is non-Null, <data> is ignored.
	The S/G list must be in kernel space because the request can be executed
	in a different thread context. This is also the	reason why the S/G list has
	to contain physical addresses. For obvious reason, the data buffer specified
	by <sg_list> must be locked, but <data> doesn't need to be.

	You can either execute the request synchronously ("sync_io") or
	asynchronously ("async_io"; you have to acquire <completion_sem> to find
	out when the request is finished). In the first case you can use either
	<data> or <sg_list>, in the latter <sg_list> only.

	The SCSI bus manager takes care that the controller can access the data
	via DMA by copying it into a buffer if necessary. For the paging path,
	this can lead to problems (if the system writes a page to disk and the SCSI
	bus manager has to allocate a buffer during execution you are in trouble),
	therefore the blk_man takes care that is not necessary for reads/writes.
	To safe some microseconds, you should set the SCSI_DMA_SAFE flag for these
	requests, so the SCSI bus manager ommittes the test.

	Effectively, using synchronous execution and specifying the address via
	<data> is a safe bet.


	For SIM writers:

	Requests sent by peripheral drivers are forwarded to the <scsi_io> entry
	of the SIM. You should return as soon as some waiting is required.
	Usually, the controller raises an IRQ when a request can be continued
	or is finished. As interrupt handlers must be as fast as possible, you
	can schedule a DPC in the handler (<schedule_dpc>) which executed by a
	high priority service thread that is spawned by the SCSI bus manager
	for each bus. This service thread also takes care to submit waiting
	requests.

	You can specify a maximum number of concurrent requests per bus via
	path_inquiry (<hba_queue_size>) for the bus. The device limit is
	determined via INQUIRY. If you need a lower/dynamic limit, you can refuse
	a request by <requeue>. If <bus_overflow> is true, no further requests
	to the bus will be sent, if <bus_overflow> is false, no further requests
	to the device will be sent. To terminate the overflow condition, call
	<cont_send_device>/<cont_send_bus>. It also terminated when a request
	for the bus/device is finished via <finished> or <resubmit>.
	Because of the asynchronous nature,	requests may still arrive after the
	overflow condition being signalled, so you should add a safety test to
	<scsi_io>.

	If a problem occurs during execution, you can ask for a restart via
	<resubmit>. The request in question will be submitted as soon as possible.

	If you want to be not disturbed, you can block further requests via
	<block_bus>/<block_device>. As said above, you must have a safety test
	at <scsi_io> though.

	If the SIM uses a non-SCSI protocol, it can ask the SCSI bus manager
	to emulate unsupported SCSI commands by translating them other (supported)
	commands. The bus manager calls <get_restriction> during detection for
	each device, setting <is_atapi> on return makes the bus manager translate
	READ6/WRITE6 commands to READ10/WRITE10 commands, MODE REQUEST6/SENSE6
	to MODE REQUEST10/SENSE10 and fix the version fields of INQUIRY results,
	so ATAPI devices can be used like standard SCSI devices. Further, the
	SCSI bus manager can emulate auto-sense by executing a REQUEST SENSE
	if <subsys_status> is SCSI_REQ_CMP_ERR and <device_status> is
	SCSI_STATUS_CHECK_CONDITION when a request is finished. This emulation
	may be enhanced/generalized in the future.
*/


#include <KernelExport.h>
#include <device_manager.h>


#define SCSI_MAX_CDB_SIZE 16	// max size of cdb
#define SCSI_MAX_SENSE_SIZE	64	// max size of sense data
#define SCSI_SIM_PRIV	1536	// SIM private data; this may be a bit much but
								// we currently need that for the compatibility layer

// bus/device handle
typedef struct scsi_bus_info *scsi_bus;
typedef struct scsi_device_info *scsi_device;


// structure of one scsi i/o CCB (command control block)
typedef struct scsi_ccb {
	struct scsi_ccb *next, *prev;	// internal

	uchar		subsys_status;		// Returned subsystem status
	uchar		device_status;		// Returned scsi device status

	uchar		path_id;			// Path ID for the request
	uchar		target_id;			// Target device ID
	uchar		target_lun;			// Target LUN number
	uint32		flags;				// Flags for operation of the subsystem

	// released once after asynchronous execution of request;
	// initialised by alloc_ccb, can be replaced for action but
	// must be restored before returning via free_ccb
	sem_id		completion_sem;

	uint8		cdb[SCSI_MAX_CDB_SIZE];  // command data block
	uchar		cdb_length;			// length of command in bytes
	int64		sort;				// value of command to sort on (<0 means n/a)
	bigtime_t	timeout;			// timeout - 0 = use default

	uchar		*data;				// pointer to data
	const physical_entry *sg_list;	// scatter/gather list
	uint16		sg_count;			// number of S/G entries
	uint32		data_length;		// length of data
	int32		data_resid;			// data transfer residual length: 2's comp
	void		*io_operation;

	uchar		sense[SCSI_MAX_SENSE_SIZE]; // autosense data
	uchar		sense_resid;		// autosense resid length: 2's comp

	// private
	bool		ordered : 1;		// request cannot overtake/be overtaken by others
	bool		buffered : 1;		// data is buffered to make it DMA safe
	bool		emulated : 1;		// command is executed as part of emulation

	scsi_bus	bus;				// associated bus
	scsi_device	device;				// associated device
	struct dma_buffer *dma_buffer;	// used dma buffer, or NULL
	uchar 		state;				// bus manager state

	// original data before command emulation was applied
	uint8		orig_cdb[SCSI_MAX_CDB_SIZE];
	uchar		orig_cdb_length;
	const physical_entry *orig_sg_list;
	uint16		orig_sg_count;
	uint32		orig_data_length;

	// private SIM data
	uchar		sim_state;			// set to zero when request is submitted first time
	uchar		sim_priv[SCSI_SIM_PRIV];	/* SIM private data area */
} scsi_ccb;


// Defines for the subsystem status field

#define	SCSI_REQ_INPROG			0x00	/* request is in progress */
#define SCSI_REQ_CMP 			0x01	/* request completed w/out error */
#define SCSI_REQ_ABORTED		0x02	/* request aborted by the host */
#define SCSI_UA_ABORT			0x03	/* Unable to Abort request */
#define SCSI_REQ_CMP_ERR		0x04	/* request completed with an err */
#define SCSI_BUSY				0x05	/* subsystem is busy */
#define SCSI_REQ_INVALID		0x06	/* request is invalid */
#define SCSI_PATH_INVALID		0x07	/* Path ID supplied is invalid */
#define SCSI_DEV_NOT_THERE		0x08	/* SCSI device not installed/there */
#define SCSI_UA_TERMIO			0x09	/* Unable to Terminate I/O req */
#define SCSI_SEL_TIMEOUT		0x0A	/* Target selection timeout */
#define SCSI_CMD_TIMEOUT		0x0B	/* Command timeout */
#define SCSI_MSG_REJECT_REC		0x0D	/* Message reject received */
#define SCSI_SCSI_BUS_RESET		0x0E	/* SCSI bus reset sent/received */
#define SCSI_UNCOR_PARITY		0x0F	/* Uncorrectable parity err occurred */
#define SCSI_AUTOSENSE_FAIL		0x10	/* Autosense: Request sense cmd fail */
#define SCSI_NO_HBA				0x11	/* No HBA detected Error */
#define SCSI_DATA_RUN_ERR		0x12	/* Data overrun/underrun error */
#define SCSI_UNEXP_BUSFREE		0x13	/* Unexpected BUS free */
#define SCSI_SEQUENCE_FAIL		0x14	/* Target bus phase sequence failure */
#define SCSI_PROVIDE_FAIL		0x16	/* Unable to provide requ. capability */
#define SCSI_BDR_SENT			0x17	/* A SCSI BDR msg was sent to target */
#define SCSI_REQ_TERMIO			0x18	/* request terminated by the host */
#define SCSI_HBA_ERR			0x19	/* Unrecoverable host bus adaptor err*/
#define SCSI_BUS_RESET_DENIED	0x1A	/* SCSI bus reset denied */

#define SCSI_IDE				0x33	/* Initiator Detected Error Received */
#define SCSI_RESRC_UNAVAIL		0x34	/* Resource unavailable */
#define SCSI_UNACKED_EVENT		0x35	/* Unacknowledged event by host */
#define SCSI_LUN_INVALID		0x38	/* LUN supplied is invalid */
#define SCSI_TID_INVALID		0x39	/* Target ID supplied is invalid */
#define SCSI_FUNC_NOTAVAIL		0x3A	/* The requ. func is not available */
#define SCSI_NO_NEXUS			0x3B	/* Nexus is not established */
#define SCSI_IID_INVALID		0x3C	/* The initiator ID is invalid */
#define SCSI_CDB_RECVD			0x3D	/* The SCSI CDB has been received */
#define SCSI_LUN_ALLREADY_ENAB	0x3E	/* LUN already enabled */
#define SCSI_SCSI_BUSY			0x3F	/* SCSI bus busy */

#define SCSI_AUTOSNS_VALID		0x80	/* Autosense data valid for target */

#define SCSI_SUBSYS_STATUS_MASK 0x3F	/* Mask bits for just the status # */


// Defines for the flags field

#define SCSI_DIR_RESV			0x00000000	/* Data direction (00: reserved) */
#define SCSI_DIR_IN				0x00000040	/* Data direction (01: DATA IN) */
#define SCSI_DIR_OUT			0x00000080	/* Data direction (10: DATA OUT) */
#define SCSI_DIR_NONE			0x000000C0	/* Data direction (11: no data) */
#define SCSI_DIR_MASK			0x000000C0

#define SCSI_DIS_AUTOSENSE		0x00000020	/* Disable autosense feature */
#define SCSI_ORDERED_QTAG		0x00000010	// ordered queue (cannot overtake/be overtaken)
#define SCSI_DMA_SAFE			0x00000008	// set if data buffer is DMA approved

#define SCSI_DIS_DISCONNECT		0x00008000	/* Disable disconnect */
#define SCSI_INITIATE_SYNC		0x00004000	/* Attempt Sync data xfer, and SDTR */
#define SCSI_DIS_SYNC			0x00002000	/* Disable sync, go to async */
#define SCSI_ENG_SYNC			0x00000200	/* Flush resid bytes before cmplt */


// Defines for the Path Inquiry CCB fields

// flags in hba_inquiry
#define SCSI_PI_MDP_ABLE		0x80	/* Supports MDP message */
#define SCSI_PI_WIDE_32			0x40	/* Supports 32 bit wide SCSI */
#define SCSI_PI_WIDE_16			0x20	/* Supports 16 bit wide SCSI */
#define SCSI_PI_SDTR_ABLE		0x10	/* Supports SDTR message */
#define SCSI_PI_TAG_ABLE		0x02	/* Supports tag queue message */
#define SCSI_PI_SOFT_RST		0x01	/* Supports soft reset */

// flags in hba_misc
#define SCSI_PIM_SCANHILO		0x80	/* Bus scans from ID 7 to ID 0 */
#define SCSI_PIM_NOREMOVE		0x40	/* Removable dev not included in scan */

// sizes of inquiry fields
#define SCSI_VUHBA		14				/* Vendor Unique HBA length */
#define SCSI_SIM_ID		16				/* ASCII string len for SIM ID */
#define SCSI_HBA_ID		16				/* ASCII string len for HBA ID */
#define SCSI_FAM_ID		16				/* ASCII string len for FAMILY ID */
#define SCSI_TYPE_ID	16				/* ASCII string len for TYPE ID */
#define SCSI_VERS		 8				/* ASCII string len for SIM & HBA vers */


// Path inquiry, extended by BeOS XPT_EXTENDED_PATH_INQ parameters
typedef struct {
	uchar		version_num;			/* Version number for the SIM/HBA */
	uchar		hba_inquiry;			/* Mimic of INQ byte 7 for the HBA */
	uchar		hba_misc;				/* Misc HBA feature flags */
	uint32		sim_priv;				/* Size of SIM private data area */
	uchar		vuhba_flags[SCSI_VUHBA];/* Vendor unique capabilities */
	uchar		initiator_id;			/* ID of the HBA on the SCSI bus */
	uint32		hba_queue_size;			// size of adapaters command queue
	char		sim_vid[SCSI_SIM_ID];	/* Vendor ID of the SIM */
	char		hba_vid[SCSI_HBA_ID];	/* Vendor ID of the HBA */

	char		sim_version[SCSI_VERS];	/* SIM version number */
	char		hba_version[SCSI_VERS];	/* HBA version number */
	char		controller_family[SCSI_FAM_ID]; /* Controller family */
	char		controller_type[SCSI_TYPE_ID]; /* Controller type */
} scsi_path_inquiry;


// Device node

// target (uint8)
#define SCSI_DEVICE_TARGET_ID_ITEM "scsi/target_id"
// lun (uint8)
#define SCSI_DEVICE_TARGET_LUN_ITEM "scsi/target_lun"
// node type
#define SCSI_DEVICE_TYPE_NAME "scsi/device/v1"
// device inquiry data (raw scsi_res_inquiry)
#define SCSI_DEVICE_INQUIRY_ITEM "scsi/device_inquiry"
// device type (uint8)
#define SCSI_DEVICE_TYPE_ITEM "scsi/type"
// vendor name (string)
#define SCSI_DEVICE_VENDOR_ITEM "scsi/vendor"
// product name (string)
#define SCSI_DEVICE_PRODUCT_ITEM "scsi/product"
// revision (string)
#define SCSI_DEVICE_REVISION_ITEM "scsi/revision"

// maximum targets on scsi bus
#define SCSI_DEVICE_MAX_TARGET_COUNT "scsi/max_target_count"

// directory containing links to peripheral drivers
#define SCSI_PERIPHERAL_DRIVERS_DIR "scsi"

// bus manager device interface for peripheral driver
typedef struct scsi_device_interface {
	driver_module_info info;

	// get CCB
	// warning: if pool of CCBs is exhausted, this call is delayed until a
	// CCB is freed, so don't try to allocate more then one CCB at once!
	scsi_ccb *(*alloc_ccb)(scsi_device device);
	// free CCB
	void (*free_ccb)(scsi_ccb *ccb);

	// execute command asynchronously
	// when it's finished, the semaphore of the ccb is released
	// you must provide a S/G list if data_len != 0
	void (*async_io)(scsi_ccb *ccb);
	// execute command synchronously
	// you don't need to provide a S/G list nor have to lock data
	void (*sync_io)(scsi_ccb *ccb);

	// abort request
	uchar (*abort)(scsi_ccb *ccb_to_abort);
	// reset device
	uchar (*reset_device)(scsi_device device);
	// terminate request
	uchar (*term_io)(scsi_ccb *ccb_to_terminate);

	status_t (*ioctl)(scsi_device device, uint32 op, void *buffer, size_t length);
} scsi_device_interface;

#define SCSI_DEVICE_MODULE_NAME "bus_managers/scsi/device/driver_v1"


// Bus node

// attributes:

// path (uint8)
#define SCSI_BUS_PATH_ID_ITEM "scsi/path_id"
// node type
#define SCSI_BUS_TYPE_NAME "scsi/bus"

// SCSI bus node driver.
// This interface can be used by peripheral drivers to access the
// bus directly.
typedef struct scsi_bus_interface {
	driver_module_info info;

	// get information about host controller
	uchar (*path_inquiry)(scsi_bus bus, scsi_path_inquiry *inquiry_data);
	// reset SCSI bus
	uchar (*reset_bus)(scsi_bus bus);
} scsi_bus_interface;

// name of SCSI bus node driver
#define SCSI_BUS_MODULE_NAME "bus_managers/scsi/bus/driver_v1"


// Interface for SIM

// cookie for dpc
typedef struct scsi_dpc_info *scsi_dpc_cookie;

// Bus manager interface used by SCSI controller drivers.
// SCSI controller drivers get this interface passed via their init_device
// method. Further, they must specify this driver as their fixed consumer.
typedef struct scsi_for_sim_interface {
	driver_module_info info;

	// put request into wait queue because of overflow
	// bus_overflow: true - too many bus requests
	//               false - too many device requests
	// bus/device won't receive requests until cont_sent_bus/cont_send_device
	// is called or a request is finished via finished();
	// to avoid race conditions (reporting a full and a available bus at once)
	// the SIM should synchronize calls to requeue, resubmit and finished
	void (*requeue)(scsi_ccb *ccb, bool bus_overflow);
	// resubmit request ASAP
	// to be used if execution of request went wrong and must be retried
	void (*resubmit)(scsi_ccb *ccb);
	// mark request as being finished
	// num_requests: number of requests that were handled by device
	//               when the request was sent (read: how full was the device
	//               queue); needed to find out how large the device queue is;
	//               e.g. if three were already running plus this request makes
	//               num_requests=4
	void (*finished)(scsi_ccb *ccb, uint num_requests);

	// following functions return error on invalid arguments only
	status_t (*alloc_dpc)(scsi_dpc_cookie *dpc);
	status_t (*free_dpc)(scsi_dpc_cookie dpc);
	status_t (*schedule_dpc)(scsi_bus cookie, scsi_dpc_cookie dpc, /*int flags,*/
		void (*func)( void * ), void *arg);

	// block entire bus (can be nested)
	// no more request will be submitted to this bus
	void (*block_bus)(scsi_bus bus);
	// unblock entire bus
	// requests will be submitted to bus ASAP
	void (*unblock_bus)(scsi_bus bus);
	// block one device
	// no more requests will be submitted to this device
	void (*block_device)(scsi_device device);
	// unblock device
	// requests for this device will be submitted ASAP
	void (*unblock_device)(scsi_device device);

	// terminate bus overflow condition (see "requeue")
	void (*cont_send_bus)(scsi_bus bus);
	// terminate device overflow condition (see "requeue")
	void (*cont_send_device)(scsi_device device);
} scsi_for_sim_interface;


#define SCSI_FOR_SIM_MODULE_NAME "bus_managers/scsi/sim/driver_v1"


// SIM Node

// attributes:

// node type
#define SCSI_SIM_TYPE_NAME "bus/scsi/v1"
// controller name (required, string)
#define SCSI_DESCRIPTION_CONTROLLER_NAME "controller_name"

typedef void *scsi_sim_cookie;

// SIM interface
// SCSI controller drivers must provide this interface
typedef struct scsi_sim_interface {
	driver_module_info info;

	void (*set_scsi_bus)(scsi_sim_cookie cookie, scsi_bus bus);

	// execute request
	void (*scsi_io)(scsi_sim_cookie cookie, scsi_ccb *ccb);
	// abort request
	uchar (*abort)(scsi_sim_cookie cookie, scsi_ccb *ccb_to_abort);
	// reset device
	uchar (*reset_device)(scsi_sim_cookie cookie, uchar target_id, uchar target_lun);
	// terminate request
	uchar (*term_io)(scsi_sim_cookie cookie, scsi_ccb *ccb_to_terminate);

	// get information about bus
	uchar (*path_inquiry)(scsi_sim_cookie cookie, scsi_path_inquiry *inquiry_data);
	// scan bus
	// this is called immediately before the SCSI bus manager scans the bus
	uchar (*scan_bus)(scsi_sim_cookie cookie);
	// reset bus
	uchar (*reset_bus)(scsi_sim_cookie cookie);

	// get restrictions of one device
	// (used for non-SCSI transport protocols and bug fixes)
	void (*get_restrictions)(scsi_sim_cookie 	cookie,
		uchar				target_id,		// target id
		bool				*is_atapi, 		// set to true if this is an ATAPI device that
											// needs some commands emulated
		bool				*no_autosense,	// set to true if there is no autosense;
											// the SCSI bus manager will request sense on
											// SCSI_REQ_CMP_ERR/SCSI_STATUS_CHECK_CONDITION
		uint32 				*max_blocks );	// maximum number of blocks per transfer if > 0;
											// used for buggy devices that cannot handle
											// large transfers (read: ATAPI ZIP drives)

	status_t (*ioctl)(scsi_sim_cookie, uint8 targetID, uint32 op, void *buffer, size_t length);
} scsi_sim_interface;


#endif	/* _SCSI_BUSMANAGER_H */
