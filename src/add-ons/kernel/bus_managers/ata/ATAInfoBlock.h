/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATA_INFOBLOCK_H
#define ATA_INFOBLOCK_H


#include <lendian_bitfield.h>


#define ATA_WORD_0_ATA_DEVICE		0
#define ATA_WORD_0_ATAPI_DEVICE		2
#define ATA_WORD_0_CFA_MAGIC		0x848a


typedef struct ata_device_infoblock {
	union {
		struct {
			LBITFIELD8(
				word_0_bit_0_reserved			: 1,
				word_0_bit_1_retired			: 1,
				response_incomplete				: 1,
				word_0_bit_3_5_retired			: 3,
				word_0_bit_6_obsolete			: 1,
				removable_media_device			: 1,
				word_0_bit_8_14_retired			: 7,
				ata_device						: 1		// 0 means ATA
			);
		} ata;
		struct {
			LBITFIELD8(
				packet_length					: 2,	// 0 = 12, 1 = 16 bytes
				response_incomplete				: 1,
				word_0_bit_3_4_reserved			: 2,
				data_request_delay				: 2,	// 0 = 3ms, 2 = 50us
				removable_media_device			: 1,
				command_packet_set				: 5,
				word_0_bit_13_reserved			: 1,
				atapi_device					: 2		// 2 means ATAPI
			);
		} atapi;
		uint16 raw;
	} word_0;

	uint16	word_1_obsolete;
	uint16	specific_configuration;
	uint16	word_3_obsolete;
	uint16	word_4_5_retired[2];
	uint16	word_6_obsolete;
	uint16	word_7_8_reserved_compact_flash_assoc[2];
	uint16	word_9_retired;
	char	serial_number[20];
	uint16	word_20_21_retired[2];
	uint16	word_22_obsolete;
	char	firmware_revision[8];
	char	model_number[40];

	LBITFIELD2(
		max_sectors_per_interrupt				: 8,
		word_47_bit_8_15_80h					: 8		// should be 0x80
	);

	uint16	word_48_reserved;

	LBITFIELD9(
		word_49_bit_0_7_retired					: 8,
		dma_supported							: 1,
		lba_supported							: 1,
		io_ready_disable						: 1,
		io_ready_supported						: 1,
		word_19_bit_12_obsolete					: 1,
		standby_timer_standard					: 1,
		atapi_command_queuing_supported			: 1,
		atapi_interleaved_dma_supported			: 1
	);

	LBITFIELD5(
		standby_timer_value_min					: 1,
		word_50_bit_1_obsolete					: 1,
		word_50_bit_2_13_reserved				: 12,
		word_50_bit_14_one						: 1,
		word_50_bit_15_zero						: 1
	);

	uint16	word_51_52_obsolete[2];

	LBITFIELD4(
		word_53_bit_0_obsolete					: 1,
		word_64_70_valid						: 1,
		word_88_valid							: 1,
		word_53_bit_3_15						: 13
	);

	uint16	word_54_58_obsolete[5];

	LBITFIELD3(
		current_sectors_per_interrupt			: 8,
		multiple_sector_setting_valid			: 1,
		word_59_bit_9_15_reserved				: 7
	);

	uint32	lba_sector_count;
	uint16	word_62_obsolete;

	LBITFIELD8(
		multiword_dma_0_supported				: 1,
		multiword_dma_1_supported				: 1,
		multiword_dma_2_supported				: 1,
		word_63_bit_3_7_resereved				: 5,
		multiword_dma_0_selected				: 1,
		multiword_dma_1_selected				: 1,
		multiword_dma_2_selected				: 1,
		word_63_bit_11_15_reserved				: 5
	);

	LBITFIELD2(
		pio_modes_supported						: 8,
		word_64_bit_8_15_reserved				: 8
	);

	uint16	min_multiword_dma_cycle_time;
	uint16	recommended_multiword_dma_cycle_time;
	uint16	min_pio_cycle_time;
	uint16	min_pio_cycle_time_io_ready;
	uint16	word_69_70_reserved[2];
	uint16	atapi_packet_received_to_bus_release_time_ns;
	uint16	atapi_service_command_to_busy_clear_time_ns;
	uint16	word_71_74_reserved[2];

	LBITFIELD2(
		max_queue_depth_minus_one				: 5,
		word_75_bit_5_15_reserved				: 11
	);

	uint16	word_76_79_reserved[4];

	LBITFIELD15(
		word_80_bit_0_reserved					: 1,
		word_80_bot_1_2_obsolete				: 2,
		supports_ata_3							: 1,
		supports_ata_atapi_4					: 1,
		supports_ata_atapi_5					: 1,
		supports_ata_atapi_6					: 1,
		supports_ata_atapi_7					: 1,
		supports_ata_atapi_8					: 1,
		supports_ata_atapi_9					: 1,
		supports_ata_atapi_10					: 1,
		supports_ata_atapi_11					: 1,
		supports_ata_atapi_12					: 1,
		supports_ata_atapi_13					: 1,
		supports_ata_atapi_14					: 1,
		word_80_bit_15_reserved					: 1
	);

	uint16	minor_version;

	LBITFIELD16(
		smart_supported							: 1,
		security_mode_supported					: 1,
		removable_media_supported				: 1,
		mandatory_power_management_supported	: 1,
		packet_supported						: 1,
		write_cache_supported					: 1,
		look_ahead_supported					: 1,
		release_interrupt_supported				: 1,
		service_interrupt_supported				: 1,
		device_reset_supported					: 1,
		host_protected_area_supported			: 1,
		word_82_bit_11_obsolete					: 1,
		write_buffer_command_supported			: 1,
		read_buffer_command_supported			: 1,
		nop_supported							: 1,
		word_82_bit_15_obsolete					: 1
	);

	LBITFIELD16(
		download_microcode_supported			: 1,
		read_write_dma_queued_supported			: 1,
		compact_flash_assoc_supported			: 1,
		advanced_power_management_supported		: 1,
		removable_media_status_supported		: 1,
		power_up_in_standby_supported			: 1,
		set_features_required_for_spinup		: 1,
		word_83_bit_7_reserved					: 1,
		set_max_security_extension_supported	: 1,
		automatic_acoustic_management_supported	: 1,
		lba48_supported							: 1,
		device_configuration_overlay_supported	: 1,
		mandatory_flush_cache_supported			: 1,
		flush_cache_ext_supported				: 1,
		word_83_bit_14_one						: 1,
		word_83_bit_15_zero						: 1
	);

	LBITFIELD9(
		smart_error_logging_supported			: 1,
		smart_self_test_supported				: 1,
		media_serial_number_supported			: 1,
		media_card_pass_through_supported		: 1,
		word_84_bit_4_reserved					: 1,
		general_purpose_logging_supported		: 1,
		word_84_bit_6_13_reserved				: 8,
		word_84_bit_14_one						: 1,
		word_84_bit_15_zero						: 1
	);

	LBITFIELD16(
		smart_enabled							: 1,
		security_mode_enabled					: 1,
		removable_media_enabled					: 1,
		mandatory_power_management_enabled		: 1,
		packet_enabled							: 1,
		write_cache_enabled						: 1,
		look_ahead_enabled						: 1,
		release_interrupt_enabled				: 1,
		service_interrupt_enabled				: 1,
		device_reset_enabled					: 1,
		host_protected_area_enabled				: 1,
		word_85_bit_11_obsolete					: 1,
		write_buffer_command_enabled			: 1,
		read_buffer_command_enabled				: 1,
		nop_enabled								: 1,
		word_85_bit_15_obsolete					: 1
	);

	LBITFIELD15(
		download_microcode_supported_2			: 1,
		read_write_dma_queued_supported_2		: 1,
		compact_flash_assoc_enabled				: 1,
		advanced_power_management_enabled		: 1,
		removable_media_status_enabled			: 1,
		power_up_in_standby_enabled				: 1,
		set_features_required_for_spinup_2		: 1,
		word_86_bit_7_reserved					: 1,
		set_max_security_extension_enabled		: 1,
		automatic_acoustic_management_enabled	: 1,
		lba48_supported_2						: 1,
		device_configuration_overlay_supported_2: 1,
		mandatory_flush_cache_supported_2		: 1,
		flush_cache_ext_supported_2				: 1,
		word_86_bit_14_15_reserved				: 2
	);

	LBITFIELD9(
		smart_error_logging_supported_2			: 1,
		smart_self_test_supported_2				: 1,
		media_serial_number_valid				: 1,
		media_card_pass_through_enabled			: 1,
		word_87_bit_4_reserved					: 1,
		general_purpose_logging_supported_2		: 1,
		word_87_bit_6_13_reserved				: 8,
		word_87_bit_14_one						: 1,
		word_87_bit_15_zero						: 1
	);

	LBITFIELD16(
		ultra_dma_0_supported					: 1,
		ultra_dma_1_supported					: 1,
		ultra_dma_2_supported					: 1,
		ultra_dma_3_supported					: 1,
		ultra_dma_4_supported					: 1,
		ultra_dma_5_supported					: 1,
		ultra_dma_6_supported					: 1,
		word_88_bit_7_reserved					: 1,
		ultra_dma_0_selected					: 1,
		ultra_dma_1_selected					: 1,
		ultra_dma_2_selected					: 1,
		ultra_dma_3_selected					: 1,
		ultra_dma_4_selected					: 1,
		ultra_dma_5_selected					: 1,
		ultra_dma_6_selected					: 1,
		word_88_bit_15_reserved					: 1
	);

	uint16	security_erase_unit_duration;
	uint16	enhanced_security_erase_duration;
	uint16	current_advanced_power_management_value;
	uint16	master_password_revision_code;

	LBITFIELD5(
		device_0_hardware_reset_result			: 8,
		device_1_hardware_reset_result			: 5,
		cable_id_detected						: 1,
		word_93_bit_14_one						: 1,
		word_93_bit_15_zero						: 1
	);

	LBITFIELD2(
		current_acoustic_management_value		: 8,
		recommended_acoustic_management_value	: 8
	);

	uint16	word_95_99_reserved[5];
	uint64	lba48_sector_count;
	uint16	word_104_105_reserved[2];

	LBITFIELD6(
		logical_sectors_per_physical_sector		: 4,	// 2^x exponent
		word_106_bit_4_11_reserved				: 8,
		logical_sector_not_512_bytes			: 1,
		multiple_logical_per_physical_sectors	: 1,
		word_106_bit_14_one						: 1,
		word_106_bit_15_zero					: 1
	);

	uint16	word_107_116_reserved[10];

	uint32	logical_sector_size;						// in words, see 106

	uint16	word_119_126_reserved[8];

	LBITFIELD2(
		removable_media_status_supported_2		: 2,	// 1 = supported
		word_127_bit_2_15_reserved				: 14
	);

	LBITFIELD9(
		security_supported						: 1,
		security_enabled						: 1,
		security_locked							: 1,
		security_frozen							: 1,
		security_count_expired					: 1,
		ehnaced_security_erase_supported		: 1,
		word_128_bit_6_7_reserved				: 2,
		security_level							: 1,	// 0 = high, 1 = max
		word_128_bit_9_15						: 7
	);

	uint16	word_129_159_vendor_specific[31];

	LBITFIELD5(
		cfa_max_current_milli_ampers			: 12,
		cfa_power_mode_1_disabled				: 1,
		cfa_power_mode_1_required				: 1,
		word_160_bit_14_reserved				: 1,
		word_160_supported						: 1
	);

	uint16	word_161_167_reserved_compact_flash_assoc[7];
	LBITFIELD2(
		device_nominal_form_factor				: 4,
		word_168_bits_4_15_reserved				: 12
	);
	LBITFIELD2(
		data_set_management_support				: 1,
		word_169_bits_1_15_reserved				: 15
	);
	uint16	additional_product_identifier[4];
	uint16	word_174_175_reserved[2];
	uint16	current_media_serial_number[30];
	uint16	word_206_208_reserved[3];

	LBITFIELD3(
		logical_sector_offset					: 14,
		word_209_bit_14_one						: 1,
		word_209_bit_15_zero					: 1
	);

	uint16	word_210_254_reserved[45];

	LBITFIELD2(
		signature								: 8,
		checksum								: 8
	);
} _PACKED ata_device_infoblock;

#endif // ATA_INFOBLOCK_H
