/*
 * Copyright 2004-2015, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_CMDS_H
#define _SCSI_CMDS_H


//! SCSI commands and their data structures and constants


#include <lendian_bitfield.h>


// Always keep in mind that SCSI is big-endian!

#define SCSI_STD_TIMEOUT 10

// SCSI device status (as the result of a command)
#define SCSI_STATUS_GOOD					(0 << 1)
#define SCSI_STATUS_CHECK_CONDITION			(1 << 1)	// error occured
#define SCSI_STATUS_CONDITION_MET			(2 << 1)
	// "found" for SEARCH DATA and PREFETCH
#define SCSI_STATUS_BUSY					(4 << 1)
	// try again later (??? == QUEUE_FULL ???)
#define SCSI_STATUS_INTERMEDIATE			(8 << 1)
	// used by linked command only
#define SCSI_STATUS_INTERMEDIATE_COND_MET	(10 << 1)	// ditto
#define SCSI_STATUS_RESERVATION_CONFLICT	(12 << 1)
	// only if RESERVE/RELEASE is used
#define SCSI_STATUS_COMMAND_TERMINATED		(17 << 1)
	// aborted by TERMINATE I/O PROCESS
#define SCSI_STATUS_QUEUE_FULL				(20 << 1)	// queue full

#define SCSI_STATUS_MASK 0xfe

// SCSI sense key
#define SCSIS_KEY_NO_SENSE					0
#define SCSIS_KEY_RECOVERED_ERROR			1
#define SCSIS_KEY_NOT_READY					2
	// operator intervention may be required
#define SCSIS_KEY_MEDIUM_ERROR				3
	// can be set if source could be hardware error
#define SCSIS_KEY_HARDWARE_ERROR			4
#define SCSIS_KEY_ILLEGAL_REQUEST			5	// invalid command
#define SCSIS_KEY_UNIT_ATTENTION			6
	// medium changed or target reset
#define SCSIS_KEY_DATA_PROTECT				7	// data access forbidden
#define SCSIS_KEY_BLANK_CHECK				8
	// tried to read blank or to write non-blank medium
#define SCSIS_KEY_VENDOR_SPECIFIC			9
#define SCSIS_KEY_COPY_ABORTED				10
	// error in COPY or COMPARE command
#define SCSIS_KEY_ABORTED_COMMAND			11
	// aborted by target, retry *may* help
#define SCSIS_KEY_EQUAL						12	// during SEARCH: data found
#define SCSIS_KEY_VOLUME_OVERFLOW			13
	// tried to write buffered data beyond end of medium
#define SCSIS_KEY_MISCOMPARE				14
#define SCSIS_KEY_RESERVED					15

// SCSI ASC and ASCQ data - (ASC << 8) | ASCQ
// all codes with bit 7 of ASC or ASCQ set are vendor-specific
#define SCSIS_ASC_NO_SENSE					0x0000
#define SCSIS_ASC_IO_PROC_TERMINATED		0x0006
#define SCSIS_ASC_AUDIO_PLAYING				0x0011
#define SCSIS_ASC_AUDIO_PAUSED				0x0012
#define SCSIS_ASC_AUDIO_COMPLETED			0x0013
#define SCSIS_ASC_AUDIO_ERROR				0x0014
	// playing has stopped due to error
#define SCSIS_ASC_AUDIO_NO_STATUS			0x0015
#define SCSIS_ASC_NO_INDEX					0x0100	// no index/sector signal
#define SCSIS_ASC_NO_SEEK_CMP				0x0200	// ???
#define SCSIS_ASC_WRITE_FAULT				0x0300
#define SCSIS_ASC_LUN_NOT_READY				0x0400
	// LUN not ready, cause not reportable
#define SCSIS_ASC_LUN_BECOMING_READY		0x0401
	// LUN in progress of becoming ready
#define SCSIS_ASC_LUN_NEED_INIT				0x0402
	// LUN need initializing command
#define SCSIS_ASC_LUN_NEED_MANUAL_HELP		0x0403
	// LUN needs manual intervention
#define SCSIS_ASC_LUN_FORMATTING			0x0404
	// LUN format in progress
#define SCSIS_ASC_LUN_SEL_FAILED			0x0500
	// LUN doesn't respond to selection
#define SCSIS_ASC_LUN_COM_FAILURE			0x0800	// LUN communication failure
#define SCSIS_ASC_LUN_TIMEOUT				0x0801
	// LUN communication time-out
#define SCSIS_ASC_LUN_COM_PARITY			0x0802
	// LUN communication parity failure
#define SCSIS_ASC_LUN_COM_CRC				0x0803
	// LUN communication CRC failure (SCSI-3)
#define SCSIS_ASC_WRITE_ERR_AUTOREALLOC		0x0c01
	// recovered by auto-reallocation
#define SCSIS_ASC_WRITE_ERR_AUTOREALLOC_FAILED 0x0c02
#define SCSIS_ASC_ECC_ERROR					0x1000
#define SCSIS_ASC_UNREC_READ_ERR			0x1100	// unrecovered read error
#define SCSIS_ASC_READ_RETRIES_EXH			0x1101	// read retries exhausted
#define SCSIS_ASC_UNREC_READ_ERR_AUTOREALLOC_FAILED 0x1104
	// above + auto-reallocate failed
#define SCSIS_ASC_RECORD_NOT_FOUND			0x1401
#define SCSIS_ASC_RANDOM_POS_ERROR			0x1500	// random positioning error
#define SCSIS_ASC_POSITIONING_ERR			0x1501
	// mechanical positioning error
#define SCSIS_ASC_POS_ERR_ON_READ			0x1502
	// positioning error detected by reading
#define SCSIS_ASC_DATA_RECOV_NO_ERR_CORR	0x1700
	// recovered with no error correction applied
#define SCSIS_ASC_DATA_RECOV_WITH_RETRIES	0x1701
#define SCSIS_ASC_DATA_RECOV_POS_HEAD_OFS	0x1702
	// ?recovered with positive head offset
#define SCSIS_ASC_DATA_RECOV_NEG_HEAD_OFS	0x1703
	// ?recovered with negative head offset
#define SCSIS_ASC_DATA_RECOV_WITH_RETRIES_CIRC 0x1704
	// recovered with retries/CIRC
#define SCSIS_ASC_DATA_RECOV_PREV_SECT_ID	0x1705
	// recovered using previous sector ID
#define SCSIS_ASC_DATA_RECOV_NO_ECC_AUTOREALLOC 0x1706
#define SCSIS_ASC_DATA_RECOV_NO_ECC_REASSIGN 0x1707 // reassign recommended
#define SCSIS_ASC_DATA_RECOV_NO_ECC_REWRITE	0x1708	// rewrite recommended
#define SCSIS_ASC_DATA_RECOV_WITH_CORR		0x1800
	// recovered using error correction
#define SCSIS_ASC_DATA_RECOV_WITH_CORR_RETRIES 0x1801
	// used error correction and retries
#define SCSIS_ASC_DATA_RECOV_AUTOREALLOC	0x1802
#define SCSIS_ASC_DATA_RECOV_CIRC			0x1803	// recovered using CIRC
#define SCSIS_ASC_DATA_RECOV_LEC			0x1804	// recovered using LEC
#define SCSIS_ASC_DATA_RECOV_REASSIGN		0x1805	// reassign recommended
#define SCSIS_ASC_DATA_RECOV_REWRITE		0x1806	// rewrite recommended
#define SCSIS_ASC_PARAM_LIST_LENGTH_ERR		0x1a00	// parameter list too short
#define SCSIS_ASC_ID_RECOV					0x1e00	// recoved ID with ECC
#define SCSIS_ASC_INV_OPCODE				0x2000
#define SCSIS_ASC_LBA_OOR					0x2100	// LBA out of range
#define SCSIS_ASC_ILL_FUNCTION				0x2200
	// better use 0x2000/0x2400/0x2600 instead
#define SCSIS_ASC_INV_CDB_FIELD				0x2400
#define SCSIS_ASC_LUN_NOT_SUPPORTED			0x2500
#define SCSIS_ASC_INV_PARAM_LIST_FIELD		0x2600
#define SCSIS_ASC_PARAM_NOT_SUPPORTED		0x2601
#define SCSIS_ASC_PARAM_VALUE_INV			0x2602
#define SCSIS_ASC_WRITE_PROTECTED			0x2700
#define SCSIS_ASC_MEDIUM_CHANGED			0x2800
#define SCSIS_ASC_WAS_RESET					0x2900
	// reset by power-on/bus reset/device reset
#define SCSIS_ASC_PARAMS_CHANGED			0x2a00
#define SCSIS_ASC_CAPACITY_DATA_HAS_CHANGED	0x2a09
#define SCSIS_ASC_MEDIUM_FORMAT_CORRUPTED	0x3100
#define SCSIS_ASC_ROUNDED_PARAM				0x3700	// parameter got rounded
#define SCSIS_ASC_NO_MEDIUM					0x3a00	// medium not present
#define SCSIS_ASC_INTERNAL_FAILURE			0x4400
#define SCSIS_ASC_SEL_FAILURE				0x4500	// select/reselect failure
#define SCSIS_ASC_UNSUCC_SOFT_RESET			0x4600	// unsuccessful soft reset
#define SCSIS_ASC_SCSI_PARITY_ERR			0x4700	// SCSI parity error
#define SCSIS_ASC_LOAD_EJECT_FAILED			0x5300
	// media load or eject failed
#define SCSIS_ASC_REMOVAL_PREVENTED			0x5302	// medium removal prevented
#define SCSIS_ASC_REMOVAL_REQUESTED			0x5a01
	// operator requests medium removal

// some scsi op-codes
#define SCSI_OP_TEST_UNIT_READY				0x00
#define SCSI_OP_REQUEST_SENSE				0x03
#define SCSI_OP_FORMAT						0x04
#define SCSI_OP_READ_6						0x08
#define SCSI_OP_WRITE_6						0x0a
#define SCSI_OP_INQUIRY						0x12
#define SCSI_OP_VERIFY_6					0x13
#define SCSI_OP_MODE_SELECT_6				0x15
#define SCSI_OP_RESERVE						0x16
#define SCSI_OP_RELEASE						0x17
#define SCSI_OP_MODE_SENSE_6				0x1a
#define SCSI_OP_START_STOP					0x1b
#define SCSI_OP_RECEIVE_DIAGNOSTIC			0x1c
#define SCSI_OP_SEND_DIAGNOSTIC				0x1d
#define SCSI_OP_PREVENT_ALLOW				0x1e
#define SCSI_OP_READ_CAPACITY				0x25
#define SCSI_OP_READ_10						0x28
#define SCSI_OP_WRITE_10					0x2a
#define SCSI_OP_POSITION_TO_ELEMENT			0x2b
#define SCSI_OP_VERIFY_10					0x2f
#define SCSI_OP_SYNCHRONIZE_CACHE			0x35
#define SCSI_OP_WRITE_BUFFER				0x3b
#define SCSI_OP_READ_BUFFER					0x3c
#define SCSI_OP_CHANGE_DEFINITION			0x40
#define SCSI_OP_WRITE_SAME_10				0x41
#define SCSI_OP_UNMAP						0x42
#define SCSI_OP_READ_SUB_CHANNEL			0x42
#define SCSI_OP_READ_TOC					0x43
#define SCSI_OP_PLAY_MSF					0x47
#define SCSI_OP_PLAY_AUDIO_TRACK_INDEX		0x48	// obsolete, spec missing
#define SCSI_OP_PAUSE_RESUME				0x4b
#define SCSI_OP_STOP_PLAY					0x4e
#define SCSI_OP_MODE_SELECT_10				0x55
#define SCSI_OP_MODE_SENSE_10				0x5a
#define SCSI_OP_VARIABLE_LENGTH_CDB			0x7f
#define SCSI_OP_READ_16						0x88
#define SCSI_OP_WRITE_16					0x8a
#define SCSI_OP_VERIFY_16					0x8f
#define SCSI_OP_WRITE_SAME_16				0x93
#define SCSI_OP_SERVICE_ACTION_IN			0x9e
#define SCSI_OP_SERVICE_ACTION_OUT			0x9f
#define SCSI_OP_MOVE_MEDIUM					0xa5
#define SCSI_OP_READ_12						0xa8
#define SCSI_OP_WRITE_12					0xaa
#define SCSI_OP_VERIFY_12					0xaf
#define SCSI_OP_READ_ELEMENT_STATUS			0xb8
#define SCSI_OP_SCAN						0xba
#define SCSI_OP_READ_CD						0xbe

// Service-Action-In defines
#define SCSI_SAI_READ_CAPACITY_16			0x10
#define SCSI_SAI_READ_LONG					0x11

// Service-Action-Out defines
#define SCSI_SAO_WRITE_LONG					0x11


// INQUIRY

typedef struct scsi_cmd_inquiry {
	uint8	opcode;
	B_LBITFIELD8_3(
		evpd : 1,						// enhanced vital product data
		_res1_1 : 4,
		lun : 3
	);
	uint8	page_code;
	uint8	_res3;
	uint8	allocation_length;
	uint8	control;
} _PACKED scsi_cmd_inquiry;

typedef struct scsi_res_inquiry {
	B_LBITFIELD8_2(
		device_type : 5,
		device_qualifier : 3
	);
	B_LBITFIELD8_2(
		device_type_modifier : 7,		// obsolete, normally set to zero
		removable_medium : 1
	);
	B_LBITFIELD8_3(						// 0 always means "not conforming"
		ansi_version : 3,				// 1 for SCSI-1, 2 for SCSI-2 etc.
		ecma_version : 3,
		iso_version : 2
	);
	B_LBITFIELD8_4(
		response_data_format : 4,		// 2 = SCSI/2 compliant
		_res3_4 : 2,
		term_iop : 1,					// 1 = supports TERMINATE I/O PROCESS
		async_enc : 1					// processor devices only :
										// Asynchronous Event Notification Capable
	);
	uint8	additional_length;			// total (whished) length = this + 4
	B_LBITFIELD8_2(
		protect : 1,
		_res5_1 : 7
	);
	uint8	_res6;
	B_LBITFIELD8_8(
		soft_reset : 1,					// 0 = soft reset leads to hard reset
		cmd_queue : 1,					// 1 = supports tagged command queuing
		_res7_2 : 1,
		linked : 1,						// 1 = supports linked commands
		sync : 1,						// 1 = supports synchronous transfers
		write_bus16 : 1,				// 1 = supports 16 bit transfers
		write_bus32 : 1,				// 1 = supports 32 bit transfers
		relative_address : 1			// 1 = supports relative addr. for linking
	);
	char	vendor_ident[8];
	char	product_ident[16];
	char	product_rev[4];

	// XPT doesn't return following data on XPT_GDEV_TYPE
	uint8	vendor_spec[20];
	uint8	_res56[2];

	uint16	version_descriptor[8];		// array of supported standards, big endian

	uint8	_res74[22];
	/* additional vendor specific data */
} _PACKED scsi_res_inquiry;

enum scsi_peripheral_qualifier {
	scsi_periph_qual_connected = 0,
	scsi_periph_qual_not_connected = 2,
	scsi_periph_qual_not_connectable = 3
	// value 1 is reserved, values of 4 and above are vendor-specific
};

enum scsi_device_type {
	scsi_dev_direct_access = 0,
	scsi_dev_sequential_access = 1,
	scsi_dev_printer = 2,
	scsi_dev_processor = 3,
	scsi_dev_WORM = 4,
	scsi_dev_CDROM = 5,
	scsi_dev_scanner = 6,
	scsi_dev_optical = 7,
	scsi_dev_medium_changer = 8,
	scsi_dev_communication = 9,
	// 0xa - 0xb are graphics arts pre-press devices
	// 0xc - 0x1e reserved
	scsi_dev_storage_array = 0xc,
	scsi_dev_enclosure_services = 0xd,
	scsi_dev_simplified_direct_access = 0xe,
	scsi_dev_optical_card = 0xf,
	scsi_dev_unknown = 0x1f 	// used for scsi_periph_qual_not_connectable
};


// vital product data: pages
#define SCSI_PAGE_SUPPORTED_VPD 0x00	/* Supported VPD Pages */
#define SCSI_PAGE_USN 0x80				/* Unit serial number */
#define SCSI_PAGE_BLOCK_LIMITS 0xb0		/* Block limits */
#define SCSI_PAGE_BLOCK_DEVICE_CHARS 0xb1	/* Block device characteristics */
#define SCSI_PAGE_LB_PROVISIONING 0xb2	/* Logical block provisioning */
#define SCSI_PAGE_REFERRALS 0xb3		/* Referrals */

// vital product data: supported pages
typedef struct scsi_page_list {
	B_LBITFIELD8_2(
		device_type : 5,
		device_qualifier : 3
	);
	uint8	page_code;
	uint8	_res2;

	uint8	page_length;
	uint8	pages[1]; 			// size according to page_length
} _PACKED scsi_page_list;

// vital product data: unit serial number page
typedef struct scsi_page_usn {
	B_LBITFIELD8_2(
		device_type : 5,
		device_qualifier : 3
	);
	uint8	page_code;
	uint8	_res2;

	uint8	_page_length;		// total size = this + 3
	char	psn[1];			// size according to page_length
} _PACKED scsi_page_usn;

typedef struct scsi_page_block_limits {
	B_LBITFIELD8_2(
		device_type : 5,
		device_qualifier : 3
	);
	uint8	page_code;

	uint16	page_length;
	B_LBITFIELD8_2(
		wsnz : 1,
		_res4_1 : 7
	);
	uint8	max_cmp_write_length;
	uint16	opt_transfer_length_grain;
	uint32	max_transfer_length;
	uint32	opt_transfer_length;
	uint32	max_prefetch_length;
	uint32	max_unmap_lba_count;
	uint32	max_unmap_blk_count;
	uint32	opt_unmap_grain;
	uint32	unmap_grain_align;
	uint64	max_write_same_length;
	uint8	_res44[20];
} _PACKED scsi_page_block_limits;

typedef struct scsi_page_lb_provisioning {
	B_LBITFIELD8_2(
		device_type : 5,
		device_qualifier : 3
	);
	uint8	page_code;

	uint16	page_length;
	uint8	threshold_exponent;
	B_LBITFIELD8_7(
		dp : 1,
		anc_sup : 1,
		lbprz : 1,
		_res5_3 : 2,
		lbpws10 : 1,
		lbpws : 1,
		lbpu : 1
	);
	B_LBITFIELD8_2(
		provisioning_type : 3,
		_res6_3 : 5
	);
	uint8 _res7;
} _PACKED scsi_page_lb_provisioning;


// READ CAPACITY (10)

typedef struct scsi_cmd_read_capacity {
	uint8	opcode;
	B_LBITFIELD8_3(
		relative_address : 1,		// relative address
		_res1_1 : 4,
		lun : 3
	);
	uint32	lba;
	uint8	_res6[2];
	B_LBITFIELD8_2(
		pmi : 1,							// partial medium indicator
		_res8_1 : 7
	);
	uint8	control;
} _PACKED scsi_cmd_read_capacity;

typedef struct scsi_res_read_capacity {
	uint32	lba;					// big endian
	uint32	block_size;				// in bytes
} _PACKED scsi_res_read_capacity;

// READ CAPACITY (16)

typedef struct scsi_cmd_read_capacity_long {
	uint8	opcode;
	uint8	service_action;
	uint64	lba;
	uint32	alloc_length;
	B_LBITFIELD8_2(
		pmi : 1,
		_res14_1 : 7
	);
	uint8	control;
} _PACKED scsi_cmd_read_capacity_long;

typedef struct scsi_res_read_capacity_long {
	uint64	lba;					// big endian
	uint32	block_size;				// in bytes
	B_LBITFIELD8_4(
		prot_en : 1,
		p_type : 3,
		rc_basis : 2,
		_res12_6 : 2
	);
	B_LBITFIELD8_2(
		logical_blocks_per_physical_block_exponent : 4,
		p_i_exponent : 4
	);
	B_LBITFIELD8_3(
		lowest_aligned_lba_p1 : 6,
			// first part of the Lowest Aligned LBA field
		lbprz : 1,
		lbpme : 1
	);
	uint8	lowest_aligned_lba_p2;
		// second part of the Lowest Aligned LBA field
		// (B_LBITFIELD16_3 would not help here because of its alignment)
	uint8	_res16[16];
} _PACKED scsi_res_read_capacity_long;


// READ (6), WRITE (6)

typedef struct scsi_cmd_rw_6 {
	uint8	opcode;
	B_LBITFIELD8_2(
		high_lba : 5,
		lun : 3
	);
	uint8	mid_lba;
	uint8	low_lba;
	uint8	length;					// 0 = 256 blocks
	uint8	control;
} _PACKED scsi_cmd_rw_6;


// READ (10), WRITE (10)

typedef struct scsi_cmd_rw_10 {
	uint8	opcode;
	B_LBITFIELD8_5(
		relative_address : 1,		// relative address
		_res1_1 : 2,
		force_unit_access : 1,		// force unit access (1 = safe, cacheless access)
		disable_page_out : 1,		// disable page out (1 = not worth caching)
		lun : 3
	);
	uint32	lba;					// big endian
	uint8	_res6;
	uint16	length;					// 0 = no block
	uint8	control;
} _PACKED scsi_cmd_rw_10;


// READ (12), WRITE (12)

typedef struct scsi_cmd_rw_12 {
	uint8	opcode;
	B_LBITFIELD8_5(
		relative_address : 1,		// relative address
		_res1_1 : 2,
		force_unit_access : 1,		// force unit access (1 = safe, cacheless access)
		disable_page_out : 1,		// disable page out (1 = not worth caching)
		lun : 3
	);
	uint32	lba;					// big endian
	uint32	length;					// 0 = no block
	uint8	_res10;
	uint8	control;
} _PACKED scsi_cmd_rw_12;


// READ (16), WRITE (16)

typedef struct scsi_cmd_rw_16 {
	uint8	opcode;
	B_LBITFIELD8_6(
		_res1_0 : 1,
		force_unit_access_non_volatile : 1,
		_res1_2 : 1,
		force_unit_access : 1,
		disable_page_out : 1,
		read_protect : 3
	);
	uint64	lba;					// big endian
	uint32	length;
	B_LBITFIELD8_3(
		group_number : 5,
		_res_14_5 : 2,
		_res_14_7 : 1
	);
	uint8	control;
} _PACKED scsi_cmd_rw_16;


// WRITE SAME (10)

typedef struct scsi_cmd_wsame_10 {
	uint8	opcode;
	B_LBITFIELD8_6(
		_obsolete1_0 : 1,
		_obsolete1_1 : 1,
		_obsolete1_2 : 1,
		unmap : 1,
		anchor : 1,
		write_protect : 3
	);
	uint32	lba;
	B_LBITFIELD8_2(
		group_number : 5,
		_res6_5 : 3
	);
	uint16	length;
	uint8	control;
} _PACKED scsi_cmd_wsame_10;


// WRITE SAME (16)

typedef struct scsi_cmd_wsame_16 {
	uint8	opcode;
	B_LBITFIELD8_6(
		ndob : 1,
		lb_data : 1,
		pb_data : 1,
		unmap : 1,
		anchor : 1,
		write_protect : 3
	);
	uint64	lba;
	uint32	length;
	B_LBITFIELD8_2(
		group_number : 5,
		_res14_5 : 3
	);
	uint8	control;
} _PACKED scsi_cmd_wsame_16;


// UNMAP

typedef struct scsi_cmd_unmap {
	uint8	opcode;
	B_LBITFIELD8_2(
		anchor : 1,
		_reserved1_7 : 7
	);
	uint32	_reserved1;
	B_LBITFIELD8_2(
		group_number : 5,
		_reserved5_7 : 3
	);
	uint16	length;
	uint8	control;
} _PACKED scsi_cmd_unmap;

struct scsi_unmap_block_descriptor {
	uint64	lba;
	uint32	block_count;
	uint32	_reserved1;
} _PACKED;

struct scsi_unmap_parameter_list {
	uint16	data_length;
	uint16	block_data_length;
	uint32	_reserved1;
	struct scsi_unmap_block_descriptor blocks[1];
} _PACKED;


// REQUEST SENSE

typedef struct scsi_cmd_request_sense {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res2[2];
	uint8	allocation_length;
	uint8	control;
} _PACKED scsi_cmd_request_sense;


// sense data structures

#define SCSIS_CURR_ERROR 0x70
#define SCSIS_DEFERRED_ERROR 0x71

typedef struct scsi_sense {
	B_LBITFIELD8_2(
		error_code : 7,
		valid : 1							// 0 = not conforming to standard
	);
	uint8 segment_number;					// for COPY/COPY AND VERIFY/COMPARE
	B_LBITFIELD8_5(
		sense_key : 4,
		res2_4 : 1,
		ILI : 1,							// incorrect length indicator - req. block
											// length doesn't match physical block length
		EOM : 1,							// serial devices only
		Filemark : 1						// optional for random access
	);

	uint8 highest_inf;						// device-type or command specific
	uint8 high_inf;							// device-type 0, 4, 5, 7: block address
	uint8 mid_inf;							// device-type 1, 2, 3: req length - act. length
	uint8 low_inf;							// (and others for sequential dev. and COPY cmds

	uint8 add_sense_length; 				// total length = this + 7

	uint8 highest_cmd_inf;
	uint8 high_cmd_inf;
	uint8 mid_cmd_inf;
	uint8 low_cmd_inf;
	uint8 asc;
	uint8 ascq;								// this can be zero if unsupported
	uint8 unit_code;						// != 0 to specify internal device unit

	union {
		struct {
		B_LBITFIELD8_2(
			high_key_spec : 7,
			SKSV : 1						// 1 = sense key specific (byte 15-17) valid
		);
		uint8 mid_key_spec;
		uint8 low_key_spec;
		} raw;

		// ILLEGAL REQUEST
		struct {
		B_LBITFIELD8_5(
			bit_pointer : 3,				// points to (highest) invalid bit of parameter
			BPV : 1,						// 1 = bit_pointer is valid
			res15_4 : 2,
			c_d : 2,						// 1 = error command, 0 = error in data
			SKSV : 1						// s.a.
		);
		uint8 high_field_pointer;			// points to (highest) invalid byte of parameter
		uint8 low_field_pointer;			// (!using big endian, this means the first byte!)
		} ill_request;

		// access error (RECOVERED, HARDWARE or MEDIUM ERROR)
		struct {
		B_LBITFIELD8_2(
			res15_0 : 7,
			SKSV : 1
		);
		uint8 high_retry_cnt;
		uint8 low_retry_cnt;
		} acc_error;

		// format progress (if sense key = NOT READY)
		struct {
		B_LBITFIELD8_2(
			res15_0 : 7,
			SKSV : 1
		);
		uint16	progress;				// 0 = start, 0xffff = almost finished
		} format_progress;
	} sense_key_spec;

	// starting with offset 18 there are additional sense byte
} _PACKED scsi_sense;


// PREVENT ALLOW

typedef struct scsi_cmd_prevent_allow {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res2[2];
	B_LBITFIELD8_2(
		prevent : 1,		// 1 - prevent medium removal, 0 - allow removal
		_res4_1 : 7
	);
	uint8	control;
} _PACKED scsi_cmd_prevent_allow;

// START STOP UNIT

typedef struct scsi_cmd_ssu {
	uint8	opcode;
	B_LBITFIELD8_3(
		immediately : 1,			// 1 - return immediately, 0 - return on completion
		_res1_1 : 4,
		lun : 3
	);
	uint8 res2[2];
	B_LBITFIELD8_3(
		start : 1,			// 1 - load+start, i.e. allow, 0 - eject+stop, i.e. deny
		load_eject : 1,			// 1 - include loading/ejecting, 0 - only to allow/deny
		_res4_2 : 6
	);
	uint8	control;
} _PACKED scsi_cmd_ssu;


// MODE SELECT (6)

typedef struct scsi_cmd_mode_select_6 {
	uint8	opcode;
	B_LBITFIELD8_4(
		save_pages : 1,		// 1 = save pages to non-volatile memory
		_res1_1 : 3,
		pf : 1,				// 0 = old SCSI-1; 1 = new SCSI-2 format
		lun : 3
	);
	uint8	_res2[2];
	uint8	param_list_length;	// data size
	uint8	control;
} _PACKED scsi_cmd_mode_select_6;


// MODE SENSE (6)

typedef struct scsi_cmd_mode_sense_6 {
	uint8	opcode;
	B_LBITFIELD8_4(
		_res1_0 : 3,
		disable_block_desc : 1,		// disable block descriptors
		_res1_4 : 1,
		lun : 3
	);
	B_LBITFIELD8_2(
		page_code : 6,
		page_control : 2			// page control field
	);
	uint8	_res3;
	uint8	allocation_length;		// maximum amount of data
	uint8	control;
} _PACKED scsi_cmd_mode_sense_6;


// MODE SELECT (10)

typedef struct scsi_cmd_mode_select_10 {
	uint8	opcode;
	B_LBITFIELD8_4(
		save_pages : 1,				// 1 = save pages to non-volatile memory
		_res1_1 : 3,
		pf : 1,						// 0 = old SCSI-1; 1 = new SCSI-2 format
		lun : 3
	);
	uint8	_res2[5];
	uint16	param_list_length;		// data size, big endian
	uint8	control;
} _PACKED scsi_cmd_mode_select_10;


// MODE SENSE (10)

typedef struct scsi_cmd_mode_sense_10 {
	uint8	opcode;
	B_LBITFIELD8_4(
		_res1_0 : 3,
		disable_block_desc : 1,		// disable block descriptors
		_res1_4 : 1,
		lun : 3
	);
	B_LBITFIELD8_2(
		page_code : 6,
		page_control : 2			// page control field
	);
	uint8	_res3[4];
	uint16	allocation_length;		// maximum amount of data, big endian
	uint8	control;
} _PACKED scsi_cmd_mode_sense_10;

// possible contents of page control (PC)
#define SCSI_MODE_SENSE_PC_CURRENT 0
#define SCSI_MODE_SENSE_PC_CHANGABLE 1
	// changable field are filled with "1"
#define SCSI_MODE_SENSE_PC_DEFAULT 2
#define SCSI_MODE_SENSE_PC_SAVED 3

// special mode page indicating to return all mode pages
#define SCSI_MODEPAGE_ALL 0x3f

// header of mode data; followed by block descriptors and mode pages
typedef struct scsi_mode_param_header_6 {
	uint8	mode_data_length;		// total length excluding this byte
	uint8	medium_type;
	uint8	dev_spec_parameter;
	uint8	block_desc_length;		// total length of all transmitted block descriptors
} _PACKED scsi_mode_param_header_6;

typedef struct scsi_mode_param_header_10 {
	uint16	mode_data_length;		// total length excluding these two bytes
	uint8	medium_type;
	uint8	dev_spec_parameter;
	uint8	_res4[2];
	uint16	block_desc_length;		// total length of all transmitted block descriptors
} _PACKED scsi_mode_param_header_10;


// content of dev_spec_parameter for direct access devices
typedef struct scsi_mode_param_dev_spec_da {
	B_LBITFIELD8_4(
		_res0_0 : 4,
		dpo_fua : 1,			// 1 = supports DPO and FUA, see READ (10) (sense only)
		_res0_6 : 1,
		write_protected : 1		// write protected (sense only)
	);
} _PACKED scsi_mode_param_dev_spec_da;

typedef struct scsi_mode_param_block_desc {
	uint8	density;			// density code of area
	uint8	high_numblocks;		// size of this area in blocks
	uint8	med_numblocks;		// 0 = all remaining blocks
	uint8	low_numblocks;
	uint8	_res4;
	uint8	high_blocklen;		// block size
	uint8	med_blocklen;
	uint8	low_blocklen;
} _PACKED scsi_mode_param_block_desc;


// header of a mode pages
typedef struct scsi_modepage_header {
	B_LBITFIELD8_3(
		page_code : 6,
		_res0_6 : 1,
		PS : 1				// 1 = page can be saved (only valid for MODE SENSE)
	);
	uint8	page_length;	// size of page excluding this common header
} _PACKED scsi_modepage_header;


// control mode page
#define SCSI_MODEPAGE_CONTROL 0xa

typedef struct scsi_modepage_control {
	scsi_modepage_header header;
	B_LBITFIELD8_2(
		RLEC : 1,			// Report Log Exception Condition
		res2_1 : 7
	);
	B_LBITFIELD8_4(
		DQue : 1,			// disable Queuing
		QErr : 1,			// abort queued commands on contingent allegiance condition
		res3_2 : 2,
		QAM : 4				// Queue Algorithm Modifier
	);
	B_LBITFIELD8_5(
		EAENP : 1,			// error AEN permission; true = send AEN on deferred error
							// false = generate UA condition after deferred error
		UAAENP : 1,			// unit attention AEN permission; true = send AEN,
							// false = generate UA condition (for everything but init.)
		RAENP : 1,			// ready AEN permission; true = send async event notification
							// (AEN) instead of generating an Unit Attention (UA) Condition
							// after initialization
		res4_3 : 4,
		EECA : 1			// enable Extended Contingent Allegiance
	);
	uint8 res5;
	uint8 high_AEN_holdoff;	// ready AEN hold off period - delay in ms between
	uint8 low_AEN_holdoff;	// initialization and AEN
} scsi_modepage_control;

// values for QAM
#define SCSI_QAM_RESTRICTED 0
#define SCSI_QAM_UNRESTRICTED 1
// 2 - 7 reserved, 8 - 0xf vendor-specific


// CD audio control page
#define SCSI_MODEPAGE_AUDIO 0xe

typedef struct scsi_modepage_audio {
	scsi_modepage_header header;
	B_LBITFIELD8_4(
		_res2_0 : 1,
		stop_on_track_crossing : 1,		// Stop On Track Crossing
			// 0 - stop according transfer length, 1 - stop at end of track
		immediately : 1,					// must be one
		_res2_3 : 5
	);
	uint8 _res3[3];
	uint8 _obsolete6[2];
	struct {
		B_LBITFIELD8_2(
			channel : 4,	// select channel to connect to this port
			_res0_4 : 4
		);
		uint8 volume;
	} ports[4];
} _PACKED scsi_modepage_audio;

// connection between output port and audio channel
#define SCSI_CHANNEL_SEL_MUTED		0	// mute port
#define SCSI_CHANNEL_SEL_CHANNEL0	1	// connect to channel 0
#define SCSI_CHANNEL_SEL_CHANNEL1	2	// connect to channel 1
#define SCSI_CHANNEL_SEL_CHANNEL0_1	3	// connect to channel 0 and channel 1
#define SCSI_CHANNEL_SEL_CHANNEL2	4	// connect to channel 2
#define SCSI_CHANNEL_SEL_CHANNEL3	8	// connect to channel 3

// TUR

typedef struct scsi_cmd_tur {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res3[3];
	uint8	control;
} _PACKED scsi_cmd_tur;


// READ_TOC

typedef struct scsi_cmd_read_toc {
	uint8	opcode;
	B_LBITFIELD8_4(
		_res1_0 : 1,
		time : 1,					// true, to use MSF format, false for LBA format
		_res1_2 : 3,
		lun : 3
	);
	B_LBITFIELD8_2(
		format : 4,					// see below
		_res2_4 : 4
	);
	uint8	_res3[3];
	uint8	track;					// (starting) track
	uint16	allocation_length;		// maximum amount of data (big endian)
	uint8	control;
} _PACKED scsi_cmd_read_toc;

// values of <format> in TOC command
#define SCSI_TOC_FORMAT_TOC 0			// all TOCs starting with <track> (0xaa for lead-out)
#define SCSI_TOC_FORMAT_SESSION_INFO 1	// Session info
#define SCSI_TOC_FORMAT_FULL_TOC 2		// all Q-channel data in TOC
#define SCSI_TOC_FORMAT_PMA 3			// Q-channel data in PMA area
#define SCSI_TOC_FORMAT_ATIP 4			// get ATIP data
#define SCSI_TOC_FORMAT_CD_TEXT 5		// get CD-Text from R/W-channel in lead-in

// general structure of response
typedef struct scsi_toc_general {
	uint16	data_length;				// big endian, total length - 2
	uint8	first;						// first track/session/reserved
	uint8	last;							// last one
	// remainder are parameter list descriptors
} _PACKED scsi_toc_general;

// definition of CD-ROM LBA
typedef uint32 scsi_cd_lba;				// big endian

// definition of CD-ROM MSF time
typedef struct scsi_cd_msf {
	uint8	_reserved;
	uint8	minute;
	uint8	second;
	uint8	frame;
} _PACKED scsi_cd_msf;

// definition of Track Number address format
typedef struct scsi_cd_track_number {
	uint8	_res0[3];
	uint8	track;
} _PACKED scsi_cd_track_number;

// one track for SCSI_TOC_FORMAT_TOC
typedef struct scsi_toc_track {
	uint8	_res0;
	B_LBITFIELD8_2(
		control : 4,
		adr : 4
	);
	uint8	track_number;		// track number (hex)
	uint8	_res3;
	union {					// start of track (time or LBA, see TIME of command)
		scsi_cd_lba lba;
		scsi_cd_msf time;
	} start;
} _PACKED scsi_toc_track;

// possible value of ADR-field (described Q-channel content)
enum scsi_adr {
	scsi_adr_none = 0,				// no Q-channel mode info
	scsi_adr_position = 1,			// Q-channel encodes current position data
	scsi_adr_mcn = 2,				// Q-channel encodes Media Catalog Number
	scsi_adr_isrc = 3				// Q-channel encodes ISRC
};

// value of Q-channel control field (CONTROL)
enum scsi_q_control {
	scsi_q_control_2audio 			= 0,	// stereo audio
	scsi_q_control_2audio_preemp	= 1,	// stereo audio with 50/15µs pre-emphasis
	scsi_q_control_1audio			= 8,	// audio (reserved in CD-R/W)
	scsi_q_control_1audio_preemp	= 9,	// audio with pre-emphasis (reserved in CD-R/W)
	scsi_q_control_data_un_intr 	= 4,	// data, recorded un-interrupted
	scsi_q_control_data_incr		= 5,	// data, recorded incremental
	scsi_q_control_ddcd				= 4,	// DDCD data
	scsi_q_control_copy_perm		= 2		// copy permitted (or-ed with value above)
};

// format SCSI_TOC_FORMAT_TOC
typedef struct scsi_toc_toc {
	uint16	data_length;			// big endian, total length - 2
	uint8	first_track;			// first track
	uint8	last_track;				// last track

	scsi_toc_track tracks[1];		// one entry per track
} _PACKED scsi_toc_toc;


// READ SUB-CHANNEL

typedef struct scsi_cmd_read_subchannel {
	uint8	opcode;
	B_LBITFIELD8_4(
		_res1_0 : 1,
		time : 1,					// true, to use MSF format, false for LBA format
		_res1_2 : 3,
		lun : 3
	);
	B_LBITFIELD8_3(
		_res2_0 : 6,
		subq : 1,					// 1 - return Q sub-channel data
		_res2_7 : 1
	);
	uint8	parameter_list;			// see below
	uint8	_res4[2];
	uint8	track;					// track number (hex)
	uint16	allocation_length;		// maximum amount of data, big endian
	uint8	control;
} _PACKED scsi_cmd_read_subchannel;

// values of parameter_list
enum scsi_sub_channel_parameter_list {
	scsi_sub_channel_parameter_list_cd_pos 	= 1,	// CD current position
	scsi_sub_channel_parameter_list_mcn		= 2,	// Media Catalog Number
	scsi_sub_channel_parameter_list_isrc	= 3		// Track International Standard Recording Code
};

// header of response
typedef struct scsi_subchannel_data_header {
	uint8	_res0;
	uint8	audio_status;			// see below
	uint16	data_length;			// total length - 4, big endian
} _PACKED scsi_subchannel_data_header;

// possible audio_status
enum scsi_audio_status {
	scsi_audio_status_not_supported		= 0,
	scsi_audio_status_playing			= 0x11,
	scsi_audio_status_paused			= 0x12,
	scsi_audio_status_completed			= 0x13,
	scsi_audio_status_error_stop		= 0x14,
	scsi_audio_status_no_status			= 0x15
};

typedef struct scsi_cd_current_position {
	uint8	format_code;			// always 1
	B_LBITFIELD8_2(
		control : 4,				// see scsi_q_control
		adr : 4						// see scsi_adr
	);
	uint8	track;
	uint8	index;
	union {							// current position, relative to logical beginning
		scsi_cd_lba lba;
		scsi_cd_msf time;
	} absolute_address;
	union {							// current position, relative to track
		scsi_cd_lba lba;
		scsi_cd_msf time;
	} track_relative_address;
} _PACKED scsi_cd_current_position;


// PLAY AUDIO MSF

typedef struct scsi_cmd_play_msf {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res2;
	uint8	start_minute;			// start time
	uint8	start_second;
	uint8	start_frame;
	uint8	end_minute;				// end time (excluding)
	uint8	end_second;
	uint8	end_frame;
	uint8	control;
} _PACKED scsi_cmd_play_msf;


// STOP AUDIO

typedef struct scsi_cmd_stop_play {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res2[7];
	uint8	control;
} _PACKED scsi_cmd_stop_play;


// PAUSE/RESUME

typedef struct scsi_cmd_pause_resume {
	uint8	opcode;
	B_LBITFIELD8_2(
		_res1_0 : 5,
		lun : 3
	);
	uint8	_res2[6];
	B_LBITFIELD8_2(
		resume : 1,				// 1 for resume, 0 for pause
		_res8_2 : 7
	);
	uint8	control;
} _PACKED scsi_cmd_pause_resume;


// SCAN

typedef struct scsi_cmd_scan {
	uint8	opcode;
	B_LBITFIELD8_4(
		relative_address : 1,	// must be zero
		_res1_1 : 3,
		direct : 1,				// direction: 0 forward, 1 backward
		lun : 3
	);
	union {						// start of track (depends on <type>)
		scsi_cd_lba lba;
		scsi_cd_msf time;
		scsi_cd_track_number track_number;
	} start;
	uint8	_res6[3];
	B_LBITFIELD8_2(
		res9_0 : 6,
		type : 2				// actual type of <start> (see below)
	);
	uint8	_res10;
	uint8	control;
} _PACKED scsi_cmd_scan;

// possible values for type
enum scsi_scan_type {
	scsi_scan_lba = 0,
	scsi_scan_msf = 1,
	scsi_scan_tno = 2
};


// READ_CD

typedef struct scsi_cmd_read_cd {
	uint8	opcode;
	B_LBITFIELD8_4(
		relative_address : 1,	// must be zero
		_res1_1 : 1,
		sector_type : 3,		// required sector type (1=CDDA)
		lun : 3
	);
	scsi_cd_lba lba;
	uint8	high_length;
	uint8	mid_length;
	uint8	low_length;
	B_LBITFIELD8_6(
		_res9_0 : 1,
		error_field : 2,
		edc_ecc : 1,			// include EDC/ECC; includes 8 byte padding for Mode 1 format
		user_data : 1,			// if 1, include user data
								// (mode select block size is ignored)
		header_code : 2,
		sync : 1				// if 1, include sync field from sector
	);
	B_LBITFIELD8_2(
		sub_channel_selection : 4,
		_res10_4 : 4
	);
	uint8	control;
} _PACKED scsi_cmd_read_cd;

// possible values for header_code
enum scsi_read_cd_header_code {
	scsi_read_cd_header_none			= 0,
	scsi_read_cd_header_hdr_only		= 1,
	scsi_read_cd_header_sub_header_only	= 2,
	scsi_read_cd_header_all_headers		= 3,
};

// possible values for error_field
enum scsi_read_cd_error_field {
	scsi_read_cd_error_none					= 0,
	scsi_read_cd_error_c2_error				= 1, // include 2352 bits indicating error in byte
	scsi_read_cd_error_c2_and_block_error	= 2, // include or of C2 data plus pad byte
};

// possible values for sub_channel_selection
enum scsi_read_cd_sub_channel_selection {
	scsi_read_cd_sub_channel_none			= 0,
	scsi_read_cd_sub_channel_RAW			= 1,
	scsi_read_cd_sub_channel_Q				= 2,
	scsi_read_cd_sub_channel_P_W			= 4	// R/W data, depending on CD capabilities
												// and Mechanism status page
};

// SYNCHRONIZE CACHE (10)

typedef struct scsi_cmd_sync_cache {
	uint8	opcode;
	B_LBITFIELD8_4(
		relative_address : 1,	// must be zero
		immediately : 1,		// 1 - return immediately, 0 - return on completion
		_res1_1 : 3,
		lun : 3
	);
	scsi_cd_lba lba;
	uint8	_res2;
	uint16	block_count;		// big endian
	uint8	control;
} _PACKED scsi_cmd_sync_cache;


#endif	/* _SCSI_CMDS_H */
