/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Siarzhuk Zharski <imker@gmx.li>
 */
#ifndef _SCSI_COMMANDS_H_ 
#define _SCSI_COMMANDS_H_

/*!	Definitions for SCSI commands, structures etc. */

#include <lendian_bitfield.h>

/* References:
 * http://www.t10.org/ftp/t10/drafts/rbc/rbc-r10a.pdf
 * http://www.t10.org/ftp/t10/drafts/rbc/rbc-a101.pdf
 * http://www.t10.org/ftp/t10/drafts/s2/s2-r10l.pdf
 * http://www.t10.org/ftp/t10/drafts/cam/cam-r12b.pdf
 * http://www.qic.org/html/standards/15x.x/qic157d.pdf
 *
 * http://www.usb.org/developers/data/devclass/usbmass-ufi10.pdf
 */

#define CAM_DIR_MASK (CAM_DIR_IN | CAM_DIR_OUT | CAM_DIR_NONE )

/* SCSI status defines */
#define SCSI_STATUS_OK				0x00
#define SCSI_STATUS_CHECK_CONDITION	0x02
	
/* SCSI command opcodes */
#define READ_6			0x08
#define WRITE_6			0x0a	
#define READ_10			0x28
#define WRITE_10		0x2a	
#define MODE_SELECT_6	0x15
#define MODE_SENSE_6	0x1a	
#define MODE_SELECT_10	0x55
#define MODE_SENSE_10	0x5a	
#define READ_CAPACITY	0x25
#define TEST_UNIT_READY	0x00
#define START_STOP_UNIT	0x1b

/* come from UFI specs */
#define FORMAT_UNIT			0x04
#define INQUIRY				0x12
/*#define START_STOP_UNIT	0x1b
#define MODE_SELECT		 	0x55
#define MODE_SENSE			0x5a */
#define PREVENT_ALLOW_MEDIA_REMOVAL 0x1e
#define READ_10				0x28
#define READ_12				0xa8
/*#define READ_CAPACITY	 0x25*/
#define READ_FORMAT_CAPACITY	0x23
#define REQUEST_SENSE	 		0x03
#define REZERO_UNIT		 		0x01
#define SEEK_10				 	0x2b
#define SEND_DIAGNOSTICS		0x1d
/*#define TEST_UNIT_READY	 0x00*/
#define VERIFY					0x2f
#define WRITE_10				0x2a
#define WRITE_12				0xaa
#define WRITE_AND_VERIFY		0x2e
/* end of UFI commands*/

/* come from ATAPI specs*/
/*#define FORMAT_UNIT	 0x04
#define INQUIRY			 0x12
#define MODE_SELECT	 0x55
#define MODE_SENSE		0x5a
#define PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define READ_10			 0x28
#define READ_12			 0xa8
#define READ_CAPACITY 0x25
#define READ_FORMAT_CAPACITIES	0x23
#define REQUEST_SENSE 0x03
#define SEEK					0x2b
#define START_STOP_UNIT	0x1b
#define TEST_UNIT_READY	0x00
#define VERIFY				0x2f 
#define WRITE_10			0x2a
#define WRITE_12			0xaa
#define WRITE_AND_VEIRIFY 0x2e */
#define PAUSE_RESUME		0x4b
#define PLAY_AUDIO			0x45
#define PLAY_AUDIO_MSF 		0x47
#define REWIND				0x01
#define PLAY_AUDIO_TRACK 	0x48

/* end of ATAPI commands*/

/* come from MMC2 specs */
#define READ_BUFFER			0x3c 
#define READ_SUBCHANNEL		0x42 
#define READ_TOC			0x43
#define READ_HEADER			0x44
#define READ_DISK_INFO		0x51
#define READ_TRACK_INFO		0x52
#define SEND_OPC			0x54
#define READ_MASTER_CUE		0x59
#define CLOSE_TR_SESSION	0x5b
#define READ_BUFFER_CAP		0x5c
#define SEND_CUE_SHEET		0x5d
#define BLANK				0xa1
#define EXCHANGE_MEDIUM		0xa6
#define READ_DVD_STRUCTURE	0xad
#define DVD_REPORT_KEY		0xa4
#define DVD_SEND_KEY		0xa3
#define SET_CD_SPEED		0xbb
#define GET_CONFIGURATION	0x46
/* end of MMC2 specs */

/* come from RBC specs */
/*#define FORMAT_UNIT				0x04
#define INQUIRY						0x12
#define MODE_SELECT_6			0x15
#define MODE_SENSE_6			 0x1a*/
#define PERSISTENT_RESERVE_IN		0x5e
#define PERSISTENT_RESERVE_OUT		0x5f
/*#define PREVENT_ALLOW_MEDIUM_REMOVAL	0x1e
#define READ_10						0x28
#define READ_CAPACITY			0x25*/
#define RELEASE_6					0x17
/*#define REQUEST_SENSE			0x03*/
#define RESERVE_6					0x16
/*#define START_STOP_UNIT		0x1b*/
#define SYNCHRONIZE_CACHE			0x35
/*#define TEST_UNIT_READY		0x00
#define VERIFY_10					0x2f
#define WRITE_10					 0x2a*/
#define WRITE_BUFFER			 	0x3b
/* end of RBC commands */

/* come from QIC-157 specvs */
#define ERASE			0x19
/*#define INQUIRY			0x12*/
#define LOAD_UNLOAD		0x1b
#define LOCATE			0x2b
#define LOG_SELECT	 	0x4c
#define LOG_SENSE		0x4d
/*#define MODE_SELECT	0x15
#define MODE_SENSE	 0x1a
#define READ				 0x08*/
#define READ_POSITION	0x34
/*#define REQUEST_SENSE	 0x03*/
#define REWIND			0x01
#define SPACE			0x11
/*#define TEST_UNIT_READY 0x00
#define WRITE					 0x0a*/
#define WRITE_FILEMARK	0x10
/* end of QIC-157 */

/* SCSI commands layout*/
/* generic commands layout*/
typedef struct{
	uint8 opcode;
	uint8 bytes[11];
}scsi_cmd_generic;
/* six-bytes command*/
typedef struct{
	uint8 opcode;
	uint8 addr[3];
#define CMD_GEN_6_ADDR	0x1f /* mask used for addr field*/
#define	CMD_LUN			0xE0 /* mask used for LUN	*/
#define	CMD_LUN_SHIFT	0x05 /* shift for LUN */
	uint8 len;
	uint8 ctrl;
}scsi_cmd_generic_6;
/* ten-bytes command*/
typedef struct{
	uint8 opcode;
	uint8 byte2; 
	uint8 addr[4];
	uint8 reserved;
	uint8 len[2];
	uint8 ctrl;
}scsi_cmd_generic_10;
/* twelve-bytes command*/
typedef struct{
	uint8 opcode;
	uint8 byte2; 
	uint8 addr[4];
	uint8 len[4];
	uint8 reserved;
	uint8 ctrl;
}scsi_cmd_generic_12;
/* READ_6 / WRITE_6 */
typedef scsi_cmd_generic_6 scsi_cmd_rw_6;

/* READ_10 / WRITE_10 */
typedef struct {
	uint8 opcode;
	LBITFIELD8_5(
		relative_address : 1,		// relative address
		_res1_1 : 2,
		force_unit_access : 1,		// force unit access (1 = safe, cacheless access)
		disable_page_out : 1,		// disable page out (1 = not worth caching)
		lun : 3
	);
	uint32 lba;			// big endian
	uint8 _reserved;
	uint16 length;		// big endian
	uint8 control;
} scsi_cmd_rw_10;

/* MODE_SELECT_6 */
typedef struct{
	uint8 opcode;
	uint8 byte2;
#define CMD_MSEL_6_SP	0x01
#define CMD_MSEL_6_PF	0x10
	uint8 reserved[2];
	uint8 len;
	uint8 ctrl;
}scsi_cmd_mode_select_6;
/* MODE_SELECT_10 */
typedef struct{
	uint8 opcode;
	uint8 byte2;
#define CMD_MSEL_10_SP	0x01
#define CMD_MSEL_10_PF	0x10
	uint8 reserved[5];
	uint8 len[2];
	uint8 ctrl;
}scsi_cmd_mode_select_10;
/* MODE_SENSE_6 */
typedef struct{
	uint8 opcode;
	uint8 byte2;
#define CMD_MSENSE_6_DBD	0x08
	uint8 byte3;
#define CMD_MSENSE_6_PC		0x0c
	uint8 reserved;
	uint8 len;
	uint8 ctrl;
}scsi_cmd_mode_sense_6;
typedef struct{
	uint8 mode_data_len;
	uint8 medium_type;
	uint8 device_spec_params;
	uint8 block_descr_len;
}scsi_mode_param_header_6;
/* MODE_SENSE_10 */
typedef struct{
	uint8 opcode;
	uint8 byte2;
#define CMD_MSENSE_10_DBD	0x08
	uint8 byte3;
#define CMD_MSENSE_10_PC	0x0c
	uint8 reserved[4];
	uint8 len[2];
	uint8 ctrl;
}scsi_cmd_mode_sense_10;
typedef struct{
	uint8 mode_data_len[2];
	uint8 medium_type;
	uint8 device_spec_params;
	uint8 reserved1; 
	uint8 reserved2; 
	uint8 block_descr_len[2];
}scsi_mode_param_header_10;
/* TEST_UNIT_READY */
typedef struct{
	uint8 opcode;
	uint8 byte2;
	uint8 reserved[3];
	uint8 ctrl;
}scsi_cmd_test_unit_ready;
/* START_STOP_UNIT */
typedef struct{
	uint8 opcode;
	uint8 byte2;
#define CMD_SSU_IMMED	0x01	
	uint8 reserved[2];
	uint8 start_loej;
#define CMD_SSU_LOEJ	0x02
#define CMD_SSU_START	0x01
	uint8 ctrl;
}scsi_cmd_start_stop_unit;

/* SCSI REQUEST SENSE data. See 8.2.14*/
typedef struct _scsi_sense_data{
	uint8 error_code;
#define SSD_ERRCODE			0x7F
#define	SSD_CURRENT_ERROR	0x70
#define	SSD_DEFERRED_ERROR	0x71
#define	SSD_ERRCODE_VALID	0x80	
	uint8 segment;
	uint8 flags;
#define SSD_KEY					0x0F
#define	SSD_KEY_NO_SENSE		0x00
#define	SSD_KEY_RECOVERED_ERROR 0x01
#define	SSD_KEY_NOT_READY		0x02
#define	SSD_KEY_MEDIUM_ERROR	0x03
#define	SSD_KEY_HARDWARE_ERROR	0x04
#define	SSD_KEY_ILLEGAL_REQUEST 0x05
#define	SSD_KEY_UNIT_ATTENTION	0x06
#define	SSD_KEY_DATA_PROTECT	0x07
#define	SSD_KEY_BLANK_CHECK		0x08
#define	SSD_KEY_Vendor_Specific 0x09
#define	SSD_KEY_COPY_ABORTED	0x0a
#define	SSD_KEY_ABORTED_COMMAND 0x0b		
#define	SSD_KEY_EQUAL			0x0c
#define	SSD_KEY_VOLUME_OVERFLOW 0x0d
#define	SSD_KEY_MISCOMPARE		0x0e
#define	SSD_KEY_RESERVED		0x0f			
#define SSD_ILI					0x20
#define SSD_EOM					0x40
#define SSD_FILEMARK			0x80
	uint8 info[4];
	uint8 extra_len;
	uint8 cmd_spec_info[4];
	uint8 add_sense_code;
	uint8 add_sense_code_qual;
	uint8 fru;
	uint8 sense_key_spec[3];
#define SSD_SCS_VALID		0x80
#define SSD_FIELDPTR_CMD	0x40
#define SSD_BITPTR_VALID	0x08
#define SSD_BITPTR_VALUE	0x07
#define SSD_MIN_SIZE		18
	uint8 extra_bytes[14];
#define SSD_FULL_SIZE sizeof(struct _scsi_sense_data)
}scsi_sense_data;

#endif /*_SCSI_COMMANDS_H_*/

