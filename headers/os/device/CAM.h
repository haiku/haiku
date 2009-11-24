/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 *
 * Definitions for the SCSI Common Access Method as implemented in Haiku.
 *
 * See also "Draft Proposed American National Standard, SCSI-2 Common
 * Access Method Transport and SCSI Interface Module", Revision 12,
 * ANSI refernce number X3.232-199x.
 *
*/

#ifndef _CAM_H
#define _CAM_H

#include <bus_manager.h>

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef U32
typedef uint32 U32;
#endif

#ifndef I32
typedef int32 I32;
#endif

#ifndef U16
typedef uint16 U16;
#endif

/* Haiku specific additions */

typedef struct {
	uint32	serial;		/* operation serial number                    */
	uint32	micros;		/* operation time in microseconds (4294s max) */
	uint32	bytes;		/* number of bytes to transfer                */
	uchar	path;		/* target SIM ID                              */
	uchar	target;		/* target device ID                           */
	uchar	sgcount;	/* # of sg segments (0 if non-sg operation)   */
	uchar	scsi_op;	/* scsi operation byte                        */
} cam_iostat;

/* End of Haiku specific additions */

/* Defines for the XPT function codes, Table 8-2 in the CAM spec. */

/* Common function commands, 0x00 - 0x0F */
#define XPT_NOOP		0x00	/* Execute Nothing */
#define XPT_SCSI_IO		0x01	/* Execute the requested SCSI IO */
#define XPT_GDEV_TYPE	0x02	/* Get the device type information */
#define XPT_PATH_INQ	0x03	/* Path Inquiry */
#define XPT_REL_SIMQ	0x04	/* Release the SIM queue that is frozen */
#define XPT_SASYNC_CB	0x05	/* Set Async callback parameters */
#define XPT_SDEV_TYPE	0x06	/* Set the device type information */
#define XPT_SCAN_BUS	0x07	/* Scan the Scsi Bus */

/* XPT SCSI control functions, 0x10 - 0x1F */
#define XPT_ABORT		0x10	/* Abort the selected CCB */
#define XPT_RESET_BUS	0x11	/* Reset the SCSI bus */
#define XPT_RESET_DEV	0x12	/* Reset the SCSI device, BDR */
#define XPT_TERM_IO		0x13	/* Terminate the I/O process */

/* HBA engine commands, 0x20 - 0x2F */
#define XPT_ENG_INQ		0x20	/* HBA engine inquiry */
#define XPT_ENG_EXEC	0x21	/* HBA execute engine request */

/* Target mode commands, 0x30 - 0x3F */
#define XPT_EN_LUN		0x30	/* Enable LUN, Target mode support */
#define XPT_TARGET_IO	0x31	/* Execute the target IO request */
#define XPT_ACCEPT_TARG 0x32	/* Accept Host Target Mode CDB */
#define XPT_CONT_TARG	0x33	/* Cont. Host Target I/O Connection */
#define XPT_IMMED_NOTIFY 0x34	/* Notify Host Target driver of event*/
#define XPT_NOTIFY_ACK	0x35	/* Acknowledgement of event */

#define XPT_FUNC		0x7F	/* TEMPLATE */
#define XPT_VUNIQUE		0x80	/* All the rest are vendor unique commands */

/* General allocation length defines for the CCB structures. */

#define IOCDBLEN	12	/* Space for the CDB bytes/pointer */
#define VUHBA		14	/* Vendor Unique HBA length */
#define SIM_ID		16	/* ASCII string len for SIM ID */
#define HBA_ID		16	/* ASCII string len for HBA ID */

#define BE_SIM_CCB_SIZE		1536	/* we want to allocate 1.5k chunks */
#define BE_SIM_SCSIIO_SIZE	88		/* sizeof(CAM_CCB_SCSIIO) - SIM_PRIV */
#define SIM_PRIV	(BE_SIM_CCB_SIZE - BE_SIM_SCSIIO_SIZE) /* Length of SIM private data area */

/* SIM_PRIV (sim private data area)  Terms and Conditions:

 - the size of SIM_PRIV shall be such that sizeof(CCB_SIZE_UNION) = 1.5k
 - all CCB's shall be allocated from locked, contiguous memory
 - CCB's shall be aligned on 512 byte boundaries
 - SIM_PRIV will be >= 1408 bytes
 - this provides 128  8byte sg entries (512mb worth of pages, worstcase fragmentation)
 - and 256 bytes for sense data and 128 bytes for whatever else the SIM needs

 - These conditions are NULL and (void) where prohibited by law.
 - All sales are final.
 - Do not use near open flame.
*/

/* Structure definitions for the CAM control blocks, CCB's for the
subsystem. */

/* Common CCB header definition. */
typedef struct ccb_header
{
	uint32		phys_addr;		/* physical address of this CCB */
	uint16		cam_ccb_len;	/* Length of the entire CCB */
	uchar		cam_func_code;	/* XPT function code */
	uchar		cam_status;		/* Returned CAM subsystem status */
	uchar		cam_hrsvd0;		/* Reserved field, for alignment */
	uchar		cam_path_id;	/* Path ID for the request */
	uchar		cam_target_id;	/* Target device ID */
	uchar		cam_target_lun;	/* Target LUN number */
	uint32		cam_flags;		/* Flags for operation of the subsystem */
} CCB_HEADER;

/* Common SCSI functions. */

/* Union definition for the CDB space in the SCSI I/O request CCB */
typedef union cdb_un
{
	uchar*	cam_cdb_ptr;				/* Pointer to the CDB bytes to send */
	uchar	cam_cdb_bytes[IOCDBLEN];	/* Area for the CDB to send */
} CDB_UN;

/* Get device type CCB */
typedef struct ccb_getdev
{
	CCB_HEADER	cam_ch;			/* Header information fields */
	char*		cam_inq_data;	/* Ptr to the inquiry data space */
	uchar		cam_pd_type;	/* Periph device type from the TLUN */
} CCB_GETDEV;

/* Path inquiry CCB */
typedef struct ccb_pathinq
{
	CCB_HEADER	cam_ch;						/* Header information fields */
	uchar		cam_version_num;			/* Version number for the SIM/HBA */
	uchar		cam_hba_inquiry;			/* Mimic of INQ byte 7 for the HBA */
	uchar		cam_target_sprt;			/* Flags for target mode support */
	uchar		cam_hba_misc;				/* Misc HBA feature flags */
	uint16		cam_hba_eng_cnt;			/* HBA engine count */
	uchar		cam_vuhba_flags[VUHBA];		/* Vendor unique capabilities */
	uint32		cam_sim_priv;				/* Size of SIM private data area */
	uint32		cam_async_flags;			/* Event cap. for Async Callback */
	uchar		cam_hpath_id;				/* Highest path ID in the subsystem */
	uchar		cam_initiator_id;			/* ID of the HBA on the SCSI bus */
	uchar		cam_prsvd0;					/* Reserved field, for alignment */
	uchar		cam_prsvd1;					/* Reserved field, for alignment */
	char		cam_sim_vid[SIM_ID];		/* Vendor ID of the SIM */
	char		cam_hba_vid[HBA_ID];		/* Vendor ID of the HBA */
	uchar*		cam_osd_usage;				/* Ptr for the OSD specific area */
} CCB_PATHINQ;

/* Release SIM Queue CCB */
typedef struct ccb_relsim
{
	CCB_HEADER	cam_ch;				/* Header information fields */
} CCB_RELSIM;

/* SCSI I/O Request CCB */
typedef struct ccb_scsiio
{
	CCB_HEADER	cam_ch;				/* Header information fields */
	uchar*		cam_pdrv_ptr;		/* Ptr used by the Peripheral driver */
	CCB_HEADER*	cam_next_ccb;		/* Ptr to the next CCB for action */
	uchar*		cam_req_map;		/* Ptr for mapping info on the Req. */
	void		(*cam_cbfcnp)(struct ccb_scsiio*);
									/* Callback on completion function */
	uchar*		cam_data_ptr;		/* Pointer to the data buf/SG list */
	uint32		cam_dxfer_len;		/* Data xfer length */
	uchar*		cam_sense_ptr;		/* Pointer to the sense data buffer */
	uchar		cam_sense_len;		/* Num of bytes in the Autosense buf */
	uchar		cam_cdb_len;		/* Number of bytes for the CDB */
	uint16		cam_sglist_cnt;		/* Num of scatter gather list entries */
	uint32		cam_sort;			/* Value used by SIM to sort on */
	uchar		cam_scsi_status;	/* Returned scsi device status */
	uchar		cam_sense_resid;	/* Autosense resid length: 2's comp */
	uchar		cam_osd_rsvd1[2];	/* OSD Reserved field, for alignment */
	int32		cam_resid;			/* Transfer residual length: 2's comp */
	CDB_UN		cam_cdb_io;			/* Union for CDB bytes/pointer */
	uint32		cam_timeout;		/* Timeout value */
	uchar*		cam_msg_ptr;		/* Pointer to the message buffer */
	uint16		cam_msgb_len;		/* Num of bytes in the message buf */
	uint16		cam_vu_flags;		/* Vendor unique flags */
	uchar		cam_tag_action;		/* What to do for tag queuing */
	uchar		cam_iorsvd0[3];		/* Reserved field, for alignment */
	uchar		cam_sim_priv[SIM_PRIV];	/* SIM private data area */
} CCB_SCSIIO;

/* Set Async Callback CCB */
typedef struct ccb_setasync
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uint32		cam_async_flags;		/* Event enables for Callback resp */
	void		(*cam_async_func)();	/* Async Callback function address */
	uchar*		pdrv_buf;				/* Buffer set aside by the Per. drv */
	uchar		pdrv_buf_len;			/* The size of the buffer */
} CCB_SETASYNC;

/* Set device type CCB */
typedef struct ccb_setdev
{
	CCB_HEADER	cam_ch;				/* Header information fields */
	uchar		cam_dev_type;		/* Val for the dev type field in EDT */
} CCB_SETDEV;

/* SCSI Control Functions. */

/* Abort XPT Request CCB */
typedef struct ccb_abort
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	CCB_HEADER*	cam_abort_ch;			/* Pointer to the CCB to abort */
} CCB_ABORT;

/* Reset SCSI Bus CCB */
typedef struct ccb_resetbus
{
	CCB_HEADER	cam_ch;					/* Header information fields */
} CCB_RESETBUS;

/* Reset SCSI Device CCB */
typedef struct ccb_resetdev
{
	CCB_HEADER	cam_ch;					/* Header information fields */
} CCB_RESETDEV;

/* Terminate I/O Process Request CCB */
typedef struct ccb_termio
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	CCB_HEADER*	cam_termio_ch;			/* Pointer to the CCB to terminate */
} CCB_TERMIO;

/* Target mode structures. */

/* Host Target Mode Version 1 Enable LUN CCB */
typedef struct ccb_en_lun
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uint16		cam_grp6_len;			/* Group 6 VU CDB length */
	uint16		cam_grp7_len;			/* Group 7 VU CDB length */
	uchar*		cam_ccb_listptr;		/* Pointer to the target CCB list */
	uint16		cam_ccb_listcnt;		/* Count of Target CCBs in the list */
} CCB_EN_LUN;

/* Enable LUN CCB (HTM V2) */
typedef struct ccb_enable_lun
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uint16		cam_grp6_length;		/* Group 6 Vendor Unique CDB Lengths */
	uint16		cam_grp7_length;		/* Group 7 Vendor Unique CDB Lengths */
	uchar*		cam_immed_notify_list;	/* Ptr to Immediate Notify CCB list */
	uint32		cam_immed_notify_cnt;	/* Number of Immediate Notify CCBs */
	uchar*		cam_accept_targ_list;	/* Ptr to Accept Target I/O CCB list */
	uint32		cam_accept_targ_cnt;	/* Number of Accept Target I/O CCBs */
	uchar		cam_sim_priv[SIM_PRIV];	/* SIM private data area */
} CCB_ENABLE_LUN;

/* Immediate Notify CCB */
typedef struct ccb_immed_notify
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uchar*		cam_pdrv_ptr;			/* Ptr used by the Peripheral driver */
	void		(*cam_cbfnot)();		/* Callback on notification function */
	uchar*		cam_sense_ptr;			/* Pointer to the sense data buffer */
	uchar		cam_sense_len;			/* Num of bytes in the Autosense buf */
	uchar		cam_init_id;			/* ID of Initiator that selected */
	uint16		cam_seq_id;				/* Sequence Identifier */
	uchar		cam_msg_code;			/* Message Code */
	uchar		cam_msg_args[7];		/* Message Arguments */
} CCB_IMMED_NOTIFY;

/* Notify Acknowledge CCB */
typedef struct ccb_notify_ack
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uint16		cam_seq_id;				/* Sequence Identifier */
	uchar		cam_event;				/* Event */
	uchar		cam_rsvd;
} CCB_NOTIFY_ACK;

/* Accept Target I/O CCB */
typedef struct ccb_accept_targ
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uchar*		cam_pdrv_ptr;			/* Ptr used by the Peripheral driver */
	CCB_HEADER*	cam_next_ccb;			/* Ptr to the next CCB for action */
	uchar*		cam_req_map;			/* Ptr for mapping info on the Req. */
	void		(*cam_cbfcnot)();		/* Callback on completion function */
	uchar*		cam_data_ptr;			/* Pointer to the data buf/SG list */
	uint32		cam_dxfer_len;			/* Data xfer length */
	uchar*		cam_sense_ptr;			/* Pointer to the sense data buffer */
	uchar		cam_sense_len;			/* Num of bytes in the Autosense buf */
	uchar		cam_cdb_len;			/* Number of bytes for the CDB */
	uint16		cam_sglist_cnt;			/* Num of scatter gather list entries */
	uint32  	cam_sort;				/* Value used by SIM to sort on */
	uchar		cam_scsi_status;		/* Returned scsi device status */
	uchar		cam_sense_resid;		/* Autosense resid length: 2's comp */
	uchar		cam_osd_rsvd1[2];		/* OSD Reserved field, for alignment */
	int32		cam_resid;				/* Transfer residual length: 2's comp */
	CDB_UN		cam_cdb_io;				/* Union for CDB bytes/pointer */
	uint32		cam_timeout;			/* Timeout value */
	uchar*		cam_msg_ptr;			/* Pointer to the message buffer */
	uint16		cam_msgb_len;			/* Num of bytes in the message buf */
	uint16		cam_vu_flags;			/* Vendor unique flags */
	uchar		cam_tag_action;			/* What to do for tag queuing */
	uchar		cam_tag_id;				/* Tag ID */
	uchar		cam_initiator_id;		/* Initiator ID */
	uchar		cam_iorsvd0[1];			/* Reserved field, for alignment */
	uchar		cam_sim_priv[SIM_PRIV];	/* SIM private data area */
} CCB_ACCEPT_TARG;

/* Continue Target I/O CCB */
typedef CCB_ACCEPT_TARG CCB_CONT_TARG;

/* HBA engine structures. */

typedef struct ccb_eng_inq
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uint16		cam_eng_num;			/* The number for this inquiry */
	uchar		cam_eng_type;			/* Returned engine type */
	uchar		cam_eng_algo;			/* Returned algorithm type */
	uint32		cam_eng_memory;			/* Returned engine memory size */
} CCB_ENG_INQ;

typedef struct ccb_eng_exec	/* NOTE: must match SCSIIO size */
{
	CCB_HEADER	cam_ch;					/* Header information fields */
	uchar*		cam_pdrv_ptr;			/* Ptr used by the Peripheral driver */
	uint32		cam_engrsvd0;			/* Reserved field, for alignment */
	uchar*		cam_req_map;			/* Ptr for mapping info on the Req. */
	void		(*cam_cbfcnp)();		/* Callback on completion function */
	uchar*		cam_data_ptr;			/* Pointer to the data buf/SG list */
	uint32		cam_dxfer_len;			/* Data xfer length */
	uchar*		cam_engdata_ptr;		/* Pointer to the engine buffer data */
	uchar		cam_engrsvd1;			/* Reserved field, for alignment */
	uchar		cam_engrsvd2;			/* Reserved field, for alignment */
	uint16		cam_sglist_cnt;			/* Num of scatter gather list entries */
	uint32		cam_dmax_len;			/* Destination data maximum length */
	uint32		cam_dest_len;			/* Destination data length */
	int32		cam_src_resid;			/* Source residual length: 2's comp */
	uchar		cam_engrsvd3[12];		/* Reserved field, for alignment */
	uint32		cam_timeout;			/* Timeout value */
	uint32		cam_engrsvd4;			/* Reserved field, for alignment */
	uint16		cam_eng_num;			/* Engine number for this request */
	uint16		cam_vu_flags;			/* Vendor unique flags */
	uchar		cam_engrsvd5;			/* Reserved field, for alignment */
	uchar		cam_engrsvd6[3];		/* Reserved field, for alignment */
	uchar		cam_sim_priv[SIM_PRIV]; /* SIM private data area */
} CCB_ENG_EXEC;

/* The sim_module_info definition is used to define the entry points for
the SIMs contained in the SCSI CAM subsystem.	Each SIM file will
contain a declaration for it's entry.	The address for this entry will
be stored in the cam_conftbl[] array along will all the other SIM
entries. */

typedef struct sim_module_info sim_module_info;

struct sim_module_info {
	module_info	minfo;
};

typedef struct CAM_SIM_ENTRY CAM_SIM_ENTRY;

struct CAM_SIM_ENTRY {
	status_t	(*sim_init)();
	int32		(*sim_action)();
};

/* ---------------------------------------------------------------------- */

/* Defines for the CAM status field in the CCB header. */

#define	CAM_REQ_INPROG			0x00	/* CCB request is in progress */
#define CAM_REQ_CMP 			0x01	/* CCB request completed w/out error */
#define CAM_REQ_ABORTED			0x02	/* CCB request aborted by the host */
#define CAM_UA_ABORT			0x03	/* Unable to Abort CCB request */
#define CAM_REQ_CMP_ERR			0x04	/* CCB request completed with an err */
#define CAM_BUSY				0x05	/* CAM subsystem is busy */
#define CAM_REQ_INVALID			0x06	/* CCB request is invalid */
#define CAM_PATH_INVALID		0x07	/* Path ID supplied is invalid */
#define CAM_DEV_NOT_THERE		0x08	/* SCSI device not installed/there */
#define CAM_UA_TERMIO			0x09	/* Unable to Terminate I/O CCB req */
#define CAM_SEL_TIMEOUT			0x0A	/* Target selection timeout */
#define CAM_CMD_TIMEOUT			0x0B	/* Command timeout */
#define CAM_MSG_REJECT_REC		0x0D	/* Message reject received */
#define CAM_SCSI_BUS_RESET		0x0E	/* SCSI bus reset sent/received */
#define CAM_UNCOR_PARITY		0x0F	/* Uncorrectable parity err occurred */
#define CAM_AUTOSENSE_FAIL		0x10	/* Autosense: Request sense cmd fail */
#define CAM_NO_HBA				0x11	/* No HBA detected Error */
#define CAM_DATA_RUN_ERR		0x12	/* Data overrun/underrun error */
#define CAM_UNEXP_BUSFREE		0x13	/* Unexpected BUS free */
#define CAM_SEQUENCE_FAIL		0x14	/* Target bus phase sequence failure */
#define CAM_CCB_LEN_ERR			0x15	/* CCB length supplied is inadequate */
#define CAM_PROVIDE_FAIL		0x16	/* Unable to provide requ. capability */
#define CAM_BDR_SENT			0x17	/* A SCSI BDR msg was sent to target */
#define CAM_REQ_TERMIO			0x18	/* CCB request terminated by the host */
#define CAM_HBA_ERR				0x19	/* Unrecoverable host bus adaptor err*/
#define CAM_BUS_RESET_DENIED	0x1A	/* SCSI bus reset denied */

#define CAM_IDE					0x33	/* Initiator Detected Error Received */
#define CAM_RESRC_UNAVAIL		0x34	/* Resource unavailable */
#define CAM_UNACKED_EVENT		0x35	/* Unacknowledged event by host */
#define CAM_MESSAGE_RECV		0x36	/* Msg received in Host Target Mode */
#define CAM_INVALID_CDB			0x37	/* Invalid CDB recvd in HT Mode */
#define CAM_LUN_INVALID			0x38	/* LUN supplied is invalid */
#define CAM_TID_INVALID			0x39	/* Target ID supplied is invalid */
#define CAM_FUNC_NOTAVAIL		0x3A	/* The requ. func is not available */
#define CAM_NO_NEXUS			0x3B	/* Nexus is not established */
#define CAM_IID_INVALID			0x3C	/* The initiator ID is invalid */
#define CAM_CDB_RECVD			0x3D	/* The SCSI CDB has been received */
#define CAM_LUN_ALLREADY_ENAB	0x3E	/* LUN already enabled */
#define CAM_SCSI_BUSY			0x3F	/* SCSI bus busy */

#define CAM_SIM_QFRZN			0x40	/* The SIM queue is frozen w/this err */
#define CAM_AUTOSNS_VALID		0x80	/* Autosense data valid for target */

#define CAM_STATUS_MASK			0x3F	/* Mask bits for just the status # */

/* ---------------------------------------------------------------------- */

/* Defines for the CAM flags field in the CCB header. */

#define CAM_DIR_RESV			0x00000000	/* Data direction (00: reserved) */
#define CAM_DIR_IN				0x00000040	/* Data direction (01: DATA IN) */
#define CAM_DIR_OUT				0x00000080	/* Data direction (10: DATA OUT) */
#define CAM_DIR_NONE			0x000000C0	/* Data direction (11: no data) */
#define CAM_DIS_AUTOSENSE		0x00000020	/* Disable autosense feature */
#define CAM_SCATTER_VALID		0x00000010	/* Scatter/gather list is valid */
#define CAM_DIS_CALLBACK		0x00000008	/* Disable callback feature */
#define CAM_CDB_LINKED			0x00000004	/* The CCB contains a linked CDB */
#define CAM_QUEUE_ENABLE		0x00000002	/* SIM queue actions are enabled */
#define CAM_CDB_POINTER			0x00000001	/* The CDB field contains a pointer */

#define CAM_DIS_DISCONNECT 		0x00008000	/* Disable disconnect */
#define CAM_INITIATE_SYNC		0x00004000	/* Attempt Sync data xfer, and SDTR */
#define CAM_DIS_SYNC			0x00002000	/* Disable sync, go to async */
#define CAM_SIM_QHEAD			0x00001000	/* Place CCB at the head of SIM Q */
#define CAM_SIM_QFREEZE			0x00000800	/* Return the SIM Q to frozen state */
#define CAM_SIM_QFRZDIS			0x00000400	/* Disable the SIM Q frozen state */
#define CAM_ENG_SYNC			0x00000200	/* Flush resid bytes before cmplt */

#define CAM_ENG_SGLIST			0x00800000	/* The SG list is for the HBA engine */
#define CAM_CDB_PHYS			0x00400000	/* CDB pointer is physical */
#define CAM_DATA_PHYS			0x00200000	/* SG/Buffer data ptrs are physical */
#define CAM_SNS_BUF_PHYS		0x00100000	/* Autosense data ptr is physical */
#define CAM_MSG_BUF_PHYS		0x00080000	/* Message buffer ptr is physical */
#define CAM_NXT_CCB_PHYS		0x00040000	/* Next CCB pointer is physical */
#define CAM_CALLBCK_PHYS		0x00020000	/* Callback func ptr is physical */

#define CAM_SEND_STATUS			0x80000000	/* Send status after date phase */
#define CAM_DISCONNECT			0x40000000	/* Disc. mandatory after cdb recv */
#define CAM_TERM_IO				0x20000000	/* Terminate I/O Message supported */

#define CAM_DATAB_VALID			0x80000000	/* Data buffer valid */
#define CAM_STATUS_VALID		0x40000000	/* Status buffer valid */
#define CAM_MSGB_VALID			0x20000000	/* Message buffer valid */
#define CAM_TGT_PHASE_MODE 		0x08000000	/* The SIM will run in phase mode */
#define CAM_TGT_CCB_AVAIL		0x04000000	/* Target CCB available */
#define CAM_DIS_AUTODISC		0x02000000	/* Disable autodisconnect */
#define CAM_DIS_AUTOSRP			0x01000000	/* Disable autosave/restore ptrs */

/* ---------------------------------------------------------------------- */

/* Defines for the SIM/HBA queue actions.	These value are used in the
SCSI I/O CCB, for the queue action field. [These values should match the
defines from some other include file for the SCSI message phases.	We may
not need these definitions here. ] */

#define CAM_SIMPLE_QTAG			0x20		/* Tag for a simple queue */
#define CAM_HEAD_QTAG			0x21		/* Tag for head of queue */
#define CAM_ORDERED_QTAG		0x22		/* Tag for ordered queue */

/* ---------------------------------------------------------------------- */

/* Defines for the timeout field in the SCSI I/O CCB.	At this time a value
of 0xF-F indicates a infinite timeout.	A value of 0x0-0 indicates that the
SIM's default timeout can take effect. */

#define CAM_TIME_DEFAULT		0x00000000	/* Use SIM default value */
#define CAM_TIME_INFINITY		0xFFFFFFFF	/* Infinite timeout for I/O */

/* ---------------------------------------------------------------------- */

/* Defines for the Path Inquiry CCB fields. */

#define	CAM_VERSION				0x25	/* Binary value for the current ver */

#define PI_MDP_ABLE				0x80	/* Supports MDP message */
#define PI_WIDE_32				0x40	/* Supports 32 bit wide SCSI */
#define PI_WIDE_16				0x20	/* Supports 16 bit wide SCSI */
#define PI_SDTR_ABLE			0x10	/* Supports SDTR message */
#define PI_LINKED_CDB			0x08	/* Supports linked CDBs */
#define PI_TAG_ABLE				0x02	/* Supports tag queue message */
#define PI_SOFT_RST				0x01	/* Supports soft reset */

#define PIT_PROCESSOR			0x80	/* Target mode processor mode */
#define PIT_PHASE				0x40	/* Target mode phase cog. mode */

#define PIM_SCANHILO			0x80	/* Bus scans from ID 7 to ID 0 */
#define PIM_NOREMOVE			0x40	/* Removable dev not included in scan */
#define PIM_NOINQUIRY			0x20	/* Inquiry data not kept by XPT */

/* ---------------------------------------------------------------------- */

/* Defines for Asynchronous Callback CCB fields. */

#define AC_FOUND_DEVICES		0x80	/* During a rescan new device found */
#define AC_SIM_DEREGISTER		0x40	/* A loaded SIM has de-registered */
#define AC_SIM_REGISTER			0x20	/* A loaded SIM has registered */
#define AC_SENT_BDR				0x10	/* A BDR message was sent to target */
#define AC_SCSI_AEN				0x08	/* A SCSI AEN has been received */
#define AC_UNSOL_RESEL			0x02	/* A unsolicited reselection occurred */
#define AC_BUS_RESET			0x01	/* A SCSI bus RESET occurred */

/* ---------------------------------------------------------------------- */

/* Typedef for a scatter/gather list element. */

typedef struct sg_elem
{
	uchar*		cam_sg_address;		/* Scatter/Gather address */
	uint32		cam_sg_count;			/* Scatter/Gather count */
} SG_ELEM;

/* ---------------------------------------------------------------------- */

/* Defines for the "event" field in the CCB_NOTIFY_ACK */
#define CAM_RESET_CLEARED		0x80	/* Reset Cleared */

/* ---------------------------------------------------------------------- */

/* Defines for the HBA engine inquiry CCB fields. */

#define EIT_BUFFER				0x00	/* Engine type: Buffer memory */
#define EIT_LOSSLESS			0x01	/* Engine type: Lossless compression */
#define EIT_LOSSLY				0x02	/* Engine type: Lossly compression */
#define EIT_ENCRYPT				0x03	/* Engine type: Encryption */

#define EAD_VUNIQUE				0x00	/* Eng algorithm ID: vendor unique */
#define EAD_LZ1V1				0x00	/* Eng algorithm ID: LZ1 var. 1*/
#define EAD_LZ2V1				0x00	/* Eng algorithm ID: LZ2 var. 1*/
#define EAD_LZ2V2				0x00	/* Eng algorithm ID: LZ2 var. 2*/

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

/* Unix OSD defines and data structures. */

#define INQLEN	36		/* Inquiry string length to store. */

#define CAM_SUCCESS	0	/* For signaling general success */
#define CAM_FAILURE	1	/* For signaling general failure */

#define CAM_FALSE	0	/* General purpose flag value */
#define CAM_TRUE	1	/* General purpose flag value */

#define XPT_CCB_INVALID	-1	/* for signaling a bad CCB to free */

/* The typedef for the Async callback information.	This structure is used to
store the supplied info from the Set Async Callback CCB, in the EDT table
in a linked list structure. */

typedef struct async_info
{
	struct async_info*	cam_async_next;			/* pointer to the next structure */
	uint32				cam_event_enable;		/* Event enables for Callback resp */
	void				(*cam_async_func)();	/* Async Callback function address */
	uint32				cam_async_blen;			/* Length of "information" buffer */
	uchar*				cam_async_ptr;			/* Address for the "information */
} ASYNC_INFO;

/* The CAM EDT table contains the device information for all the
devices, SCSI ID and LUN, for all the SCSI busses in the system.	The
table contains a CAM_EDT_ENTRY structure for each device on the bus.
*/

typedef struct cam_edt_entry
{
	int32		cam_tlun_found;			/* Flag for the existence of the target/LUN */
	ASYNC_INFO*	cam_ainfo;				/* Async callback list info for this B/T/L */
	uint32		cam_owner_tag;			/* Tag for the peripheral driver's ownership */
	char		cam_inq_data[INQLEN];	/* storage for the inquiry data */
} CAM_EDT_ENTRY;


/* ============================================================================== */
/* ----------------------------- VENDOR UNIQUE DATA ----------------------------- */
/* ============================================================================== */

/* ---
	Vendor unique XPT function codes
--- */

#define XPT_EXTENDED_PATH_INQ	(XPT_VUNIQUE + 1)		/* Extended Path Inquiry */

/* Extended path inquiry CCB */

#define FAM_ID		16						/* ASCII string len for FAMILY ID */
#define TYPE_ID		16						/* ASCII string len for TYPE ID */
#define VERS		 8						/* ASCII string len for SIM & HBA vers */

typedef struct ccb_extended_pathinq
{
	CCB_PATHINQ	cam_path;					/* Default path inquiry */
	char		cam_sim_version[VERS];		/* SIM version number */
	char		cam_hba_version[VERS];		/* HBA version number */
	char		cam_controller_family[FAM_ID]; /* Controller family */
	char		cam_controller_type[TYPE_ID]; /* Controller type */
} CCB_EXTENDED_PATHINQ;


/* ---
	Vendor unique flags supported by Haiku (cam_vu_flags)
--- */

enum {
	VU_RESERVED_0			=	0x0001,
	VU_RESERVED_1			=	0x0002,
	VU_DISABLE_SEL_W_ATN	=	0x0004,
	VU_RESERVED_4			=	0x0008
};


/* ---
	XPT interface used by SCSI drivers
--- */

typedef struct cam_for_driver_module_info cam_for_driver_module_info;

struct cam_for_driver_module_info {
	bus_manager_info	minfo;
	CCB_HEADER*			(*xpt_ccb_alloc)(void);
	void				(*xpt_ccb_free)(void* ccb);
	long				(*xpt_action)(CCB_HEADER* ccbh);
};

#define	B_CAM_FOR_DRIVER_MODULE_NAME	"bus_managers/scsi/driver/v1"

/* ---
	XPT interface used by SCSI SIMs
--- */

typedef struct cam_for_sim_module_info cam_for_sim_module_info;

struct cam_for_sim_module_info {
	bus_manager_info	minfo;
	long				(*xpt_bus_register)(CAM_SIM_ENTRY* sim);
	long				(*xpt_bus_deregister)(long path);
};

#define	B_CAM_FOR_SIM_MODULE_NAME		"bus_managers/scsi/sim/v1"


/* General Union for Kernel Space allocation.	Contains all the possible CCB
structures.	This union should never be used for manipulating CCB's its only
use is for the allocation and deallocation of raw CCB space. */

typedef union ccb_size_union
{
	CCB_SCSIIO			csio;	/* Please keep this first, for debug/print */
	CCB_GETDEV			cgd;
	CCB_PATHINQ			cpi;
	CCB_RELSIM			crs;
	CCB_SETASYNC		csa;
	CCB_SETDEV			csd;
	CCB_ABORT			cab;
	CCB_RESETBUS		crb;
	CCB_RESETDEV		crd;
	CCB_TERMIO			ctio;
	CCB_EN_LUN			cel;
	CCB_ENABLE_LUN		cel2;
	CCB_IMMED_NOTIFY	cin;
	CCB_NOTIFY_ACK		cna;
	CCB_ACCEPT_TARG		cat;
	CCB_ENG_INQ			cei;
	CCB_ENG_EXEC		cee;
	CCB_EXTENDED_PATHINQ	cdpi;
} CCB_SIZE_UNION;


#ifdef __cplusplus
}
#endif

#endif
