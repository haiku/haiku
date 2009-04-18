/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __ATA_TYPES_H__
#define __ATA_TYPES_H__

#include <iovec.h>
#include <lendian_bitfield.h>

// ATA task file.
// contains the command block interpreted under different conditions with
// first byte being first command register, second byte second command register
// etc.; for lba48, registers must be written twice, therefore there
// are twice as many bytes as registers - the first eight bytes are those
// that must be written first, the second eight bytes are those that
// must be written second.
union ata_task_file {
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_0_7;
		uint8	cylinder_8_15;
		LBITFIELD8_3(
			head				: 4,
			device				: 1,
			mode				: 3
		);
		uint8	command;
	} chs;
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	lba_0_7;
		uint8	lba_8_15;
		uint8	lba_16_23;
		LBITFIELD8_3(
			lba_24_27			: 4,
			device				: 1,
			mode				: 3
		);
		uint8	command;
	} lba;
	struct {
		LBITFIELD8_3(
			dma					: 1,
			ovl					: 1,
			_0_res2				: 6
		);
		LBITFIELD8_2(
			_1_res0				: 3,
			tag					: 5
		);
		uint8	_2_res;
		uint8	byte_count_0_7;
		uint8	byte_count_8_15;
		LBITFIELD8_6(
			lun					: 3,
			_5_res3				: 1,
			device				: 1,
			_5_one5				: 1,
			_5_res6				: 1,
			_5_one7				: 1
		);
		uint8	command;
	} packet;
	struct {
		LBITFIELD8_5(
			ili					: 1,
			eom					: 1,
			abrt				: 1,
			_0_res3				: 1,
			sense_key			: 4
		);
		LBITFIELD8_4(
			cmd_or_data			: 1,	// 1 - cmd, 0 - data
			input_or_output	 	: 1,	// 0 - input (to device), 1 - output
			release				: 1,
			tag					: 5
		);
		uint8	_2_res;
		uint8	byte_count_0_7;
		uint8	byte_count_8_15;
		LBITFIELD8_5(
			_4_res0				: 4,
			device				: 1,
			_4_obs5				: 1,
			_4_res6				: 1,
			_4_obs7				: 1
		);
		LBITFIELD8_7(
			chk					: 1,
			_7_res1 			: 2,
			drq					: 1,
			serv				: 1,
			dmrd				: 1,
			drdy				: 1,
			bsy					: 1
		);
	} packet_res;
	struct {
		uint8	sector_count;
		LBITFIELD8_4(					// only <tag> is defined for write
			cmd_or_data			: 1,	// 1 - cmd, 0 - data
			input_or_output		: 1,	// 0 - input (to device), 1 - output
			release				: 1,
			tag					: 5
		);
		uint8	lba_0_7;
		uint8	lba_8_15;
		uint8	lba_16_23;
		LBITFIELD8_3(
			lba_24_27			: 4,
			device				: 1,
			mode				: 3
		);
		uint8	command;
	} queued;
	struct {
		// low order bytes
		uint8	features;
		uint8	sector_count_0_7;
		uint8	lba_0_7;
		uint8	lba_8_15;
		uint8	lba_16_23;
		LBITFIELD8_3(
			_5low_res0			: 4,
			device				: 1,
			mode				: 3
		);
		uint8	command;

		// high order bytes
		uint8	_0high_res;
		uint8	sector_count_8_15;
		uint8	lba_24_31;
		uint8	lba_32_39;
		uint8	lba_40_47;
	} lba48;
	struct {
		// low order bytes
		uint8	sector_count_0_7;
		LBITFIELD8_4(
			cmd_or_data			: 1,	// 1 - cmd, 0 - data
			input_or_output	 	: 1,	// 0 - input (to device), 1 - output
			release				: 1,
			tag					: 5
		);
		uint8	lba_0_7;
		uint8	lba_8_15;
		uint8	lba_16_23;
		LBITFIELD8_3(
			_5low_res0			: 4,
			device				: 1,
			mode				: 3
		);
		uint8	command;

		// high order bytes
		uint8	sector_count_8_15;
		uint8	_1high_res;
		uint8	lba_24_31;
		uint8	lba_32_39;
		uint8	lba_40_47;
	} queued48;
	struct {
		uint8	r[7+5];
	} raw;
	struct {
		uint8	features;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_low;
		uint8	cylinder_high;
		uint8	device_head;
		uint8	command;
	} write;
	struct {
		uint8	error;
		uint8	sector_count;
		uint8	sector_number;
		uint8	cylinder_low;
		uint8	cylinder_high;
		uint8	device_head;
		uint8	status;
	} read;
};

typedef union ata_task_file ata_task_file;

// content of "mode" field
enum {
	ATA_MODE_CHS = 5,
	ATA_MODE_LBA = 7
};

// mask for ata_task_file fields to be written
enum {
	ATA_MASK_FEATURES	 				= 0x01,
	ATA_MASK_SECTOR_COUNT				= 0x02,

	// CHS
	ATA_MASK_SECTOR_NUMBER				= 0x04,
	ATA_MASK_CYLINDER_LOW				= 0x08,
	ATA_MASK_CYLINDER_HIGH				= 0x10,

	// LBA
	ATA_MASK_LBA_LOW					= 0x04,
	ATA_MASK_LBA_MID					= 0x08,
	ATA_MASK_LBA_HIGH					= 0x10,

	// packet
	ATA_MASK_BYTE_COUNT					= 0x18,
	
	// packet and dma queued result
	ATA_MASK_ERROR						= 0x01,
	ATA_MASK_INTERRUPT_REASON			= 0x02,

	ATA_MASK_DEVICE_HEAD				= 0x20,
	ATA_MASK_COMMAND					= 0x40,
	
	ATA_MASK_STATUS						= 0x40,

	// for 48 bits, the following flags tell which registers to load twice
	ATA_MASK_FEATURES_48				= 0x80 | ATA_MASK_FEATURES,
	ATA_MASK_SECTOR_COUNT_48			= 0x100 | ATA_MASK_SECTOR_COUNT,
	ATA_MASK_LBA_LOW_48					= 0x200 | ATA_MASK_LBA_LOW,
	ATA_MASK_LBA_MID_48					= 0x400 | ATA_MASK_LBA_MID,
	ATA_MASK_LBA_HIGH_48				= 0x800 | ATA_MASK_LBA_HIGH,
	
	ATA_MASK_HOB						= 0xf80
};

// status register
enum {
	ATA_STATUS_ERROR					= 0x01,		// error
	ATA_STATUS_INDEX					= 0x02,		// obsolete
	ATA_STATUS_CORR						= 0x04,		// obsolete
	ATA_STATUS_DATA_REQUEST				= 0x08,		// data request
	ATA_STATUS_DSC						= 0x10,		// reserved
	ATA_STATUS_SERVICE					= 0x10,		// ready to service device
	ATA_STATUS_DWF						= 0x20,		// reserved
	ATA_STATUS_DMA						= 0x20,		// reserved
	ATA_STATUS_DMA_READY				= 0x20,		// packet: DMA ready
	ATA_STATUS_DEVICE_FAULT				= 0x20,		// device fault
	ATA_STATUS_DEVICE_READY				= 0x40,		// device ready
	ATA_STATUS_BUSY						= 0x80		// busy
};

// device control register
enum {
												// bit 0 must be zero
	ATA_DEVICE_CONTROL_DISABLE_INTS		= 0x02,	// disable INTRQ
	ATA_DEVICE_CONTROL_SOFT_RESET		= 0x04,	// software device reset
	ATA_DEVICE_CONTROL_BIT3				= 0x08,	// don't know, but must be set
												// bits inbetween are reserved
	ATA_DEVICE_CONTROL_HIGH_ORDER_BYTE	= 0x80	// read high order byte
												// (for 48-bit lba)
};

// error register - most bits are command specific
enum {
	// always used
	ATA_ERROR_ABORTED					= 0x04,		// command aborted

	// used for Ultra DMA modes
	ATA_ERROR_INTERFACE_CRC				= 0x80,		// interface CRC error

	// used by reading data transfers
	ATA_ERROR_UNCORRECTABLE				= 0x40,		// uncorrectable data error
	// used by writing data transfers
	ATA_ERROR_WRITE_PROTECTED			= 0x40,		// media write protect

	// used by all data transfer commands
	ATA_ERROR_MEDIUM_CHANGED			= 0x20,		// medium changed
	ATA_ERROR_INVALID_ADDRESS			= 0x10,		// invalid CHS address
	ATA_ERROR_MEDIA_CHANGE_REQUESTED	= 0x08,		// media change requested
	ATA_ERROR_NO_MEDIA					= 0x02,		// no media

	ATA_ERROR_ALL						= 0xfe
};

typedef struct ata_channel_info *ata_channel_cookie;

#endif	/* __ATA_TYPES_H__ */
