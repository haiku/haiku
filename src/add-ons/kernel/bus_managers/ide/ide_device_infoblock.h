/*
 * Copyright 2004-2006, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _IDE_DEVICE_INFOBLOCK_H_
#define _IDE_DEVICE_INFOBLOCK_H_

/*
	Definition of response to IDE_CMD_IDENTIFY_DEVICE or 
	IDE_CMD_IDENTIFY_PACKET_DEVICE

	When a new entry is inserted, add its offset in hex
	and its index in decimal as a remark. Without that, you
	have a rough time when you messed up the offsets.
*/


#include <lendian_bitfield.h>


#define IDE_GET_INFO_BLOCK	0x2710
#define IDE_GET_STATUS		0x2711


// must be 512 bytes!!!
typedef struct tagdevice_infoblock {
	union {						// 0 general configuration
		struct {
			LBITFIELD8 (
				_0_res1							: 1,
				_0_ret1							: 1,
				response_incomplete				: 1,
				_0_ret2							: 3,
				removable_controller_or_media	: 1,
				removable_media					: 1,
				_0_ret3							: 7,
				ATA								: 1  // 0 - is ATA!
			);
		} ata;
		struct {
			LBITFIELD8 (
				packet_size						: 2, // 0 - 12 bytes, 1 - 16 bytes
				response_incomplete				: 1,
				_0_res2							: 2,
				drq_speed						: 2, // 0 - 3ms, 1 - IRQ, 2 - 50µs
				removable_media					: 1,
				type							: 5,
				_0_res13						: 1,
				ATAPI							: 2  // 2 - is ATAPI
			);
		} atapi;
	} _0;
	uint16 cylinders;			// 2
	uint16 dummy1;				// 4
	uint16 heads;				// 6
	uint16 dummy2[2];			// 8
	uint16 sectors;				// 0c
	uint16 dummy3[3];			// 0e
	char serial_number[20];		// 14
	uint16 dummy4[3];			// 28
	char firmware_version[8];	// 2e
	char model_number[40];		// 36
	uint16 dummy5[2];			// 5e
	LBITFIELD5 (				// 62 (49) capabilities
		_49_ret1			: 8,
		DMA_supported		: 1,
		LBA_supported		: 1,
		IORDY_can_disable	: 1,
		IORDY_supported		: 1
	);

	uint16 dummy6[1];			// 64
	LBITFIELD2 (				// 66 (51) obsolete: PIO modes?
		_51_obs1			: 8,
		PIO_mode			: 8
	);
	uint16 dummy7[1];			// 68

	LBITFIELD3 (				// 6a (53) validity
		_54_58_valid		: 1,
		_64_70_valid		: 1,
		_88_valid			: 1
	);
	uint16 current_cylinders;	// 6c (54)
	uint16 current_heads;		// 6e
	uint16 current_sectors;		// 70

	uint16 capacity_low;		// 72 (57) ALIGNMENT SPLIT - don't merge
	uint16 capacity_high;

	uint16 dummy8[1];

	uint32 LBA_total_sectors;	// 78 (60)
	uint16 dummy9[1];			// 7c

	LBITFIELD7 ( 				// 7e (63) MDMA modes
		MDMA0_supported		: 1,
		MDMA1_supported		: 1,
		MDMA2_supported		: 1,
		_63_res1			: 5,
		MDMA0_selected		: 1,
		MDMA1_selected		: 1,
		MDMA2_selected		: 1
	);
	uint16 dummy10[11];			// 80
	
	LBITFIELD2 (				// 96 (75)
		queue_depth			: 5,
		_75_res1			: 9
	);
	uint16 dummy11[6];			// 98
	
	LBITFIELD16 (				// a4 (82) supported_command_set
		SMART_supported				: 1,
		security_mode_supported		: 1,
		removable_media_supported	: 1,
		PM_supported				: 1,
		_81_fixed					: 1,	// must be 0
		write_cache_supported		: 1,
		look_ahead_supported		: 1,
		RELEASE_irq_supported		: 1,
		
		SERVICE_irq_supported		: 1,
		DEVICE_RESET_supported		: 1,
		HPA_supported				: 1,
		_81_obs1					: 1,
		WRITE_BUFFER_supported		: 1,
		READ_BUFFER_supported		: 1,
		NOP_supported				: 1,
		_81_obs2					: 1
	);
	LBITFIELD15 (				// a6 (83) supported_command_sets
		DOWNLOAD_MICROCODE_supported		: 1,
		DMA_QUEUED_supported				: 1,
		CFA_supported						: 1,
		APM_supported						: 1,
		RMSN_supported						: 1,
		power_up_in_stand_by_supported		: 1,
		SET_FEATURES_on_power_up_required	: 1,
		reserved_boot_area_supported		: 1,
		SET_MAX_security_supported			: 1,
		auto_acustic_managemene_supported	: 1,
		_48_bit_addresses_supported			: 1,
		device_conf_overlay_supported		: 1,
		FLUSH_CACHE_supported				: 1,
		FLUSH_CACHE_EXT_supported			: 1,
		_83_fixed							: 2	// must be 1 
	);
	
	uint16 dummy12[4];			// a8 (84)
	LBITFIELD15 (				// b0 (88) UDMA modes
		UDMA0_supported		: 1,
		UDMA1_supported		: 1,
		UDMA2_supported		: 1,
		UDMA3_supported		: 1,
		UDMA4_supported		: 1,
		UDMA5_supported		: 1,
		UDMA6_supported		: 1,	// !guessed
		_88_res1			: 1,
		UDMA0_selected		: 1,
		UDMA1_selected		: 1,
		UDMA2_selected		: 1,
		UDMA3_selected		: 1,
		UDMA4_selected		: 1,
		UDMA5_selected		: 1,
		UDMA6_selected		: 1
	);
	
	uint16 dummy89[11];			// b2 (89)
	uint64 LBA48_total_sectors;	// c8 (100)
	uint16 dummy102[22];		// cc (104)
	
	LBITFIELD2 (				// fc (126) 
		last_lun			: 2,
		_126_res2			: 14
	);
	LBITFIELD4 (				// fe (127) RMSN support
		_127_RMSN_support	: 2,// 0 = not supported, 1 = supported, 3, 4 = reserved
		_127_res2			: 6,
		device_write_protect: 2,
		_127_res9			: 6
	);
	uint16 dummy14[128];		// 100 (128)
} ide_device_infoblock;

typedef struct ide_status {
	uint8 _reserved;
	uint8 dma_status;
	uint8 pio_mode;
	uint8 dma_mode;
} ide_status;

#endif	/* _IDE_DEVICE_INFOBLOCK_H_ */
