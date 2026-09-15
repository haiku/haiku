/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha98@gmail.com>
 */

#ifndef __UFSHCI_SPEC__
#define __UFSHCI_SPEC__

#include "SupportDefs.h"

/* Utp Transfer Request List */
#define UFS_MAX_UTRL_ENTRIES 0x20
#define UFS_UTRLD_SCSI_COMMAND_TYPE 0x0FFFFFFF
#define UFS_UTRLD_MAX_PRDT_ENTRY_LENGTH (256 * 1024)
#define UFS_UTRLD_MAX_PRDT_ENTRIES 128
#define UFS_UTRLD_MAX_PRDT_BLOCKS 64
#define UFS_UTRLD_OVERALL_STATUS 0xF
#define UFS_UTRLD_INTERRUPT (1 << 24)
#define UFS_UTRLD_TRANSFER_TO_DEVICE (1 << 25)
#define UFS_UTRLD_TRANSFER_FROM_DEVICE (2 << 25)
#define UFS_UTRLD_RESPONSE_LENGTH 0x80
#define UFS_UTRLD_COMMAND_LENGTH 0x80

/* Utp Command UPIU */
#define UFS_UCD_COMMAND_UPIU 0x1
#define UFS_UCD_COMMAND_READ_FLAG (1 << 6)
#define UFS_UCD_COMMAND_WRITE_FLAG (1 << 5)

/* UfsHciController Attributes */
#define UFSHCI_MAX_TARGET_COUNT 1
#define UFSHCI_MAX_LUN_COUNT 128
#define UFSHCI_DMA_ALIGNMENT 4

struct UfsHciRegisters {
	volatile uint32 capabilities;
	volatile uint32 reserved_1; // mcq capability
	volatile uint32 version;
	volatile uint32 reserved_2;
	volatile uint32 hcddid; // host controller identification Descriptor: DeviceId and DeviceClass
	volatile uint32 hcpmid; // host controller identification Descriptor: ProductId and manufacturerId
	volatile uint32 reserved_3[2];

	volatile uint32 interrupt_status; // 0x20
	volatile uint32 interrupt_enable; // 0x24
	volatile uint32 reserved_4[2]; // 0x28
	volatile uint32 host_controller_status; // 0x30
	volatile uint32 host_controller_enable; // 0x34
	volatile uint32 error_code_phy; // 0x38
	volatile uint32 error_code_link; // 0x3C
	volatile uint32 error_code_network; // 0x40
	volatile uint32 error_code_transport; // 0x44
	volatile uint32 error_code_dme; // 0x48
	volatile uint32 interrupt_aggregation_control_register; // 0x4C

	volatile uint32 utrl_base_address_low; // 0x50
	volatile uint32 utrl_base_address_high; // 0x54
	volatile uint32 utrl_doorbell; // 0x58
	volatile uint32 utrl_clear; // 0x5C
	volatile uint32 utrl_run_stop; // 0x60

	volatile uint32 dummy; // 0x64
	volatile uint32 reserved_5[2]; // 0x68

	volatile uint32 utmrl_base_address_low; // 0x70
	volatile uint32 utmrl_base_address_high; // 0x74
	volatile uint32 utmrl_doorbell; // 0x78
	volatile uint32 utmrl_clear; // 0x7C
	volatile uint32 utmrl_run_stop; // 0x80
	volatile uint32 dummy_2; // 0x84
	volatile uint32 reserved_6[2]; // 0x88

	volatile uint32 uic_command; // 0x90
	volatile uint32 uic_command_argument_1;
	volatile uint32 uic_command_argument_2;
	volatile uint32 uic_command_argument_3;
	volatile uint32 reserved_7[4];

	uint32 reserved_8[16];
	uint32 vendor_registers[4];

} __attribute__((packed));

typedef struct UfsHciRegisters UfsHciRegisters;
/* Size of UtpCommandDescriptor: 32 Bytes*/
struct UtpTransferRequestDescriptor {
	/* Header: 16 Bytes */
	uint32 reserved[4];
	uint32 utp_cmd_descriptor_low_addr;
	uint32 utp_cmd_descriptor_high_addr;
	uint16 utp_response_upiu_length;
	uint16 utp_response_upiu_offset;
	uint16 prdt_length;
	uint16 prdt_offset;
} __attribute__((packed));

/* Size of Physical Regional Descriptor Table Entry is 16 bytes */
struct UfsPrdtEntry {
	uint32 data_base_address;
	uint32 data_base_address_upper;
	uint32 reserved;
	uint32 data_byte_count;
} __attribute__((packed));

/* Big Endian format, Size 12 Bytes */
struct UtpUpiuHeader {
	uint8 transaction_type;
	uint8 flags;
	uint8 lun;
	uint8 task_tag;
	uint8 command_type;
	uint8 query_function;
	uint8 response;
	uint8 status;
	uint8 total_ehs_length;
	uint8 device_information;
	uint16 data_segment_length;
} __attribute__((packed));

/*
 * Size of the UtpCommandUpiu: 128 Bytes (0x80)
 * header: 12 Bytes
 * expected_data_transfer_length: 4 bytes
 * cdb: 16 Bytes
 * padding; 96 Bytes
 */
struct UtpCommandUpiu {
	struct UtpUpiuHeader header;
	uint32 expected_data_transfer_length;
	uint8 cdb[16];
	uint8 reserved_pad[96];
} __attribute__((packed));

/*
 * Size of the UtpResponseUpiu: 128 Bytes (0x80)
 * header: 12 Bytes
 * expected_data_transfer_length: 4 Bytes
 * cdb: 16 Bytes
 * sense_data_length: 2 Bytes
 * sense data & padding: 94 Bytes
 */
struct UtpResponseUpiu {
	struct UtpUpiuHeader header;
	uint32 residual_transfer_count;
	uint32 reserved[4];
	uint16 sense_data_length;
	uint8 sense_data_and_pad[94];
} __attribute__((packed));

/* Size of UtpCommandDescriptor is 0x900 bytes
 * UtpCommandUpiu: 128 Bytes (0x80)
 * UtpResponseUpiu: 128 Bytes (0x80)
 * prdt: 0x800 Bytes: 16 Bytes ( UfsPrdtEntry ) * UFS_MAX_PRDT_ENTRIES: 16 * 128 = 0x800 Bytes
 */
struct UtpCommandDescriptor {
	struct UtpCommandUpiu command;
	struct UtpResponseUpiu response;
	struct UfsPrdtEntry prdt[UFS_UTRLD_MAX_PRDT_ENTRIES];
} __attribute__((packed));


struct UtpTmRequestUpiu {
	struct UtpUpiuHeader header;
	uint32 input_param_1;
	uint32 input_param_2;
	uint32 input_param_3;
	uint32 reserved[2];
	// header E2ECRC;

} __attribute__((packed));

struct UtpTmResponseUpiu {
	struct UtpUpiuHeader header;
	uint32 output_param1;
	uint32 output_param2;
	uint32 reserved[3];
	// header E2ECRC;

} __attribute__((packed));

struct UtpTmRequestDescriptor {
	uint32 header[4];
	struct UtpTmRequestUpiu request;
	struct UtpTmResponseUpiu response;
} __attribute__((packed));

#endif // !__UFSHCI_SPEC__
