/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __LIBNVME_SPEC_H__
#define __LIBNVME_SPEC_H__

#include <stdint.h>
#include <stddef.h>

/*
 * Use to mark a command to apply to all namespaces,
 * or to retrieve global log pages.
 */
#define NVME_GLOBAL_NS_TAG		((uint32_t)0xFFFFFFFF)

#define NVME_MAX_IO_QUEUES		(65535)

#define NVME_ADMIN_QUEUE_MIN_ENTRIES	2
#define NVME_ADMIN_QUEUE_MAX_ENTRIES	4096

#define NVME_IO_QUEUE_MIN_ENTRIES	2
#define NVME_IO_QUEUE_MAX_ENTRIES	65536

#define NVME_MAX_NS			1024

/*
 * Number of characters in the serial number.
 */
#define NVME_SERIAL_NUMBER_CHARACTERS	20

/*
 * Number of characters in the model number.
 */
#define NVME_MODEL_NUMBER_CHARACTERS	40

/*
 * Indicates the maximum number of range sets that may be specified
 * in the dataset mangement command.
 */
#define NVME_DATASET_MANAGEMENT_MAX_RANGES   256

/*
 * Compile time assert check.
 */
#if __GNUC__ > 3
#ifdef __cplusplus
#define nvme_static_assert(cond, msg)	static_assert(cond, msg)
#else
#define nvme_static_assert(cond, msg)	_Static_assert(cond, msg)
#endif
#else
#define nvme_static_assert(cond, msg)
#endif

union nvme_cap_register {

	uint64_t raw;

	struct {
		/* Maximum queue entries supported */
		uint32_t mqes		: 16;

		/* Contiguous queues required */
		uint32_t cqr		: 1;

		/* Arbitration mechanism supported */
		uint32_t ams		: 2;

		uint32_t reserved1	: 5;

		/*
		 * Worst case time in 500 millisecond units
		 * to wait for the controller to become ready
		 * (CSTS.RDY set to '1') after a power-on or reset
		 */
		uint32_t to		: 8;

		/* Doorbell stride */
		uint32_t dstrd		: 4;

		/* NVM subsystem reset supported */
		uint32_t nssrs		: 1;

		/* Command sets supported */
		uint32_t css_nvm	: 1;

		uint32_t css_reserved	: 3;
		uint32_t reserved2	: 7;

		/* Memory page size minimum */
		uint32_t mpsmin		: 4;

		/* Memory page size maximum */
		uint32_t mpsmax		: 4;

		uint32_t reserved3	: 8;
	} bits;

};
nvme_static_assert(sizeof(union nvme_cap_register) == 8, "Incorrect size");

union nvme_cc_register {

	uint32_t raw;

	struct {
		/* Enable */
		uint32_t en		: 1;

		uint32_t reserved1	: 3;

		/* I/O command set selected */
		uint32_t css		: 3;

		/* Memory page size */
		uint32_t mps		: 4;

		/* Arbitration mechanism selected */
		uint32_t ams		: 3;

		/* Shutdown notification */
		uint32_t shn		: 2;

		/* I/O submission queue entry size */
		uint32_t iosqes		: 4;

		/* I/O completion queue entry size */
		uint32_t iocqes		: 4;

		uint32_t reserved2	: 8;
	} bits;

};
nvme_static_assert(sizeof(union nvme_cc_register) == 4, "Incorrect size");

enum nvme_shn_value {
	NVME_SHN_NORMAL	 = 0x1,
	NVME_SHN_ABRUPT	 = 0x2,
};

union nvme_csts_register {

	uint32_t raw;

	struct {
		/* Ready */
		uint32_t rdy		: 1;

		/* Controller fatal status */
		uint32_t cfs		: 1;

		/* Shutdown status */
		uint32_t shst		: 2;

		uint32_t reserved1	: 28;
	} bits;
};
nvme_static_assert(sizeof(union nvme_csts_register) == 4, "Incorrect size");

enum nvme_shst_value {
	NVME_SHST_NORMAL      = 0x0,
	NVME_SHST_OCCURRING   = 0x1,
	NVME_SHST_COMPLETE    = 0x2,
};

union nvme_aqa_register {

	uint32_t raw;

	struct {
		/* Admin submission queue size */
		uint32_t asqs		: 12;

		uint32_t reserved1	: 4;

		/* Admin completion queue size */
		uint32_t acqs		: 12;

		uint32_t reserved2	: 4;
	} bits;

};
nvme_static_assert(sizeof(union nvme_aqa_register) == 4, "Incorrect size");

union nvme_vs_register {

	uint32_t raw;

	struct {
		/* Indicates the tertiary version */
		uint32_t ter		: 8;

		/* Indicates the minor version */
		uint32_t mnr		: 8;

		/* Indicates the major version */
		uint32_t mjr		: 16;
	} bits;

};
nvme_static_assert(sizeof(union nvme_vs_register) == 4, "Incorrect size");

/*
 * Generate raw version.
 */
#define NVME_VERSION(mjr, mnr, ter) \
	(((uint32_t)(mjr) << 16) |  \
	 ((uint32_t)(mnr) << 8) |   \
	 (uint32_t)(ter))

/*
 * Test that the shifts are correct
 */
nvme_static_assert(NVME_VERSION(1, 0, 0) == 0x00010000, "version macro error");
nvme_static_assert(NVME_VERSION(1, 2, 1) == 0x00010201, "version macro error");

union nvme_cmbloc_register {

	uint32_t raw;

	struct {
		/*
		 * Indicator of BAR which contains controller
		 * memory buffer(CMB).
		 */
		uint32_t bir		: 3;

		uint32_t reserved1	: 9;

		/* Offset of CMB in multiples of the size unit */
		uint32_t ofst		: 20;
	} bits;
};
nvme_static_assert(sizeof(union nvme_cmbloc_register) == 4, "Incorrect size");

union nvme_cmbsz_register {

	uint32_t raw;

	struct {
		/* Support submission queues in CMB */
		uint32_t sqs		: 1;

		/* Support completion queues in CMB */
		uint32_t cqs		: 1;

		/* Support PRP and SGLs lists in CMB */
		uint32_t lists		: 1;

		/* Support read data and metadata in CMB */
		uint32_t rds		: 1;

		/* Support write data and metadata in CMB */
		uint32_t wds		: 1;

		uint32_t reserved1	: 3;

		/* Indicates the granularity of the size unit */
		uint32_t szu		: 4;

		/* Size of CMB in multiples of the size unit */
		uint32_t sz		: 20;
	} bits;
};
nvme_static_assert(sizeof(union nvme_cmbsz_register) == 4, "Incorrect size");

struct nvme_registers {

	/* Controller capabilities */
	union nvme_cap_register		cap;

	/* Version of NVMe specification */
	union nvme_vs_register		vs;

	/* Interrupt mask set   */
	uint32_t			intms;

	/* Interrupt mask clear */
	uint32_t			intmc;

	/* Controller configuration */
	union nvme_cc_register		cc;

	uint32_t			reserved1;

	/* Controller status   */
	union nvme_csts_register	csts;

	/* NVM subsystem reset */
	uint32_t			nssr;

	/* Admin queue attributes */
	union nvme_aqa_register		aqa;

	/* Admin submission queue base addr */
	uint64_t 			asq;

	/* Admin completion queue base addr */
	uint64_t			acq;

	/* Controller memory buffer location */
	union nvme_cmbloc_register	cmbloc;

	/* Controller memory buffer size */
	union nvme_cmbsz_register	cmbsz;

	uint32_t			reserved3[0x3f0];

	struct {
		/* Submission queue tail doorbell */
		uint32_t		sq_tdbl;

		/* Completion queue head doorbell */
		uint32_t		cq_hdbl;
	} doorbell[1];
};

/*
 * Return the offset of a field in a structure.
 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER)  __builtin_offsetof (TYPE, MEMBER)
#endif

/*
 * NVMe controller register space offsets.
 */
nvme_static_assert(0x00 == offsetof(struct nvme_registers, cap),
		   "Incorrect register offset");
nvme_static_assert(0x08 == offsetof(struct nvme_registers, vs),
		   "Incorrect register offset");
nvme_static_assert(0x0C == offsetof(struct nvme_registers, intms),
		   "Incorrect register offset");
nvme_static_assert(0x10 == offsetof(struct nvme_registers, intmc),
		   "Incorrect register offset");
nvme_static_assert(0x14 == offsetof(struct nvme_registers, cc),
		   "Incorrect register offset");
nvme_static_assert(0x1C == offsetof(struct nvme_registers, csts),
		   "Incorrect register offset");
nvme_static_assert(0x20 == offsetof(struct nvme_registers, nssr),
		   "Incorrect register offset");
nvme_static_assert(0x24 == offsetof(struct nvme_registers, aqa),
		   "Incorrect register offset");
nvme_static_assert(0x28 == offsetof(struct nvme_registers, asq),
		   "Incorrect register offset");
nvme_static_assert(0x30 == offsetof(struct nvme_registers, acq),
		   "Incorrect register offset");
nvme_static_assert(0x38 == offsetof(struct nvme_registers, cmbloc),
		   "Incorrect register offset");
nvme_static_assert(0x3C == offsetof(struct nvme_registers, cmbsz),
		   "Incorrect register offset");

enum nvme_sgl_descriptor_type {

	NVME_SGL_TYPE_DATA_BLOCK	= 0x0,
	NVME_SGL_TYPE_BIT_BUCKET	= 0x1,
	NVME_SGL_TYPE_SEGMENT		= 0x2,
	NVME_SGL_TYPE_LAST_SEGMENT	= 0x3,
	NVME_SGL_TYPE_KEYED_DATA_BLOCK	= 0x4,

	/* 0x5 - 0xE reserved */

	NVME_SGL_TYPE_VENDOR_SPECIFIC	= 0xF

};

enum nvme_sgl_descriptor_subtype {
	NVME_SGL_SUBTYPE_ADDRESS	= 0x0,
	NVME_SGL_SUBTYPE_OFFSET		= 0x1,
};

/*
 * Scatter Gather List descriptor
 */
struct __attribute__((packed)) nvme_sgl_descriptor {

	uint64_t address;

	union {
		struct {
			uint8_t reserved[7];
			uint8_t subtype	: 4;
			uint8_t type	: 4;
		} generic;

		struct {
			uint32_t length;
			uint8_t reserved[3];
			uint8_t subtype	: 4;
			uint8_t type	: 4;
		} unkeyed;

		struct {
			uint64_t length   : 24;
			uint64_t key	  : 32;
			uint64_t subtype  : 4;
			uint64_t type	  : 4;
		} keyed;
	};

};
nvme_static_assert(sizeof(struct nvme_sgl_descriptor) == 16,
		   "Incorrect size");

enum nvme_psdt_value {
	NVME_PSDT_PRP		   = 0x0,
	NVME_PSDT_SGL_MPTR_CONTIG  = 0x1,
	NVME_PSDT_SGL_MPTR_SGL	   = 0x2,
	NVME_PSDT_RESERVED	   = 0x3
};

/*
 * Submission queue priority values for Create I/O Submission Queue Command.
 * Only valid for weighted round robin arbitration method.
 */
enum nvme_qprio {
	NVME_QPRIO_URGENT    = 0x0,
	NVME_QPRIO_HIGH	     = 0x1,
	NVME_QPRIO_MEDIUM    = 0x2,
	NVME_QPRIO_LOW	     = 0x3
};

/*
 * Optional Arbitration Mechanism Supported by the controller.
 * Two bits for CAP.AMS (18:17) field are set to '1' when the controller
 * supports. There is no bit for AMS_RR where all controllers support and
 * set to 0x0 by default.
 */
enum nvme_cap_ams {

	/*
	 * Weighted round robin
	 */
	NVME_CAP_AMS_WRR     = 0x1,

	/*
	 * Vendor specific.
	 */
	NVME_CAP_AMS_VS	     = 0x2,

};

/*
 * Arbitration Mechanism Selected to the controller.
 * Value 0x2 to 0x6 is reserved.
 */
enum nvme_cc_ams {

	/*
	 * Default round robin.
	 */
	NVME_CC_AMS_RR	    = 0x0,

	/*
	 * Weighted round robin.
	 */
	NVME_CC_AMS_WRR	    = 0x1,

	/*
	 * Vendor specific.
	 */
	NVME_CC_AMS_VS	    = 0x7,

};

struct nvme_cmd {

	/* dword 0 */
	uint16_t		opc	:  8;	/* opcode               */
	uint16_t		fuse	:  2;	/* fused operation      */
	uint16_t		rsvd1	:  4;
	uint16_t		psdt	:  2;
	uint16_t		cid;		/* command identifier   */

	/* dword 1 */
	uint32_t		nsid;		/* namespace identifier */

	/* dword 2-3 */
	uint32_t		rsvd2;
	uint32_t		rsvd3;

	/* dword 4-5 */
	uint64_t		mptr;		/* metadata pointer     */

	/* dword 6-9: data pointer */
	union {
		struct {
			uint64_t prp1;	 /* prp entry 1 */
			uint64_t prp2;	 /* prp entry 2 */
		} prp;

		struct nvme_sgl_descriptor sgl1;
	} dptr;

	/* dword 10-15 */
	uint32_t		cdw10;		/* command-specific     */
	uint32_t		cdw11;		/* command-specific     */
	uint32_t		cdw12;		/* command-specific     */
	uint32_t		cdw13;		/* command-specific     */
	uint32_t		cdw14;		/* command-specific     */
	uint32_t		cdw15;		/* command-specific     */

};
nvme_static_assert(sizeof(struct nvme_cmd) == 64, "Incorrect size");

struct nvme_status {
	uint16_t		p	:  1;	/* phase tag            */
	uint16_t		sc	:  8;	/* status code          */
	uint16_t		sct	:  3;	/* status code type     */
	uint16_t		rsvd2	:  2;
	uint16_t		m	:  1;	/* more                 */
	uint16_t		dnr	:  1;	/* do not retry         */
};
nvme_static_assert(sizeof(struct nvme_status) == 2, "Incorrect size");

/*
 * Completion queue entry
 */
struct nvme_cpl {

	/* dword 0 */
	uint32_t		cdw0;	/* command-specific              */

	/* dword 1 */
	uint32_t		rsvd1;

	/* dword 2 */
	uint16_t		sqhd;	/* submission queue head pointer */
	uint16_t		sqid;	/* submission queue identifier   */

	/* dword 3 */
	uint16_t		cid;	/* command identifier            */

	struct nvme_status	status;

};
nvme_static_assert(sizeof(struct nvme_cpl) == 16, "Incorrect size");

/*
 * Dataset Management range
 */
struct nvme_dsm_range {
	uint32_t 		attributes;
	uint32_t 		length;
	uint64_t 		starting_lba;
};
nvme_static_assert(sizeof(struct nvme_dsm_range) == 16, "Incorrect size");

/*
 * Status code types
 */
enum nvme_status_code_type {

	NVME_SCT_GENERIC		= 0x0,
	NVME_SCT_COMMAND_SPECIFIC	= 0x1,
	NVME_SCT_MEDIA_ERROR	        = 0x2,

	/* 0x3-0x6 - reserved */
	NVME_SCT_VENDOR_SPECIFIC	= 0x7,

};

/*
 * Generic command status codes
 */
enum nvme_generic_command_status_code {

	NVME_SC_SUCCESS				= 0x00,
	NVME_SC_INVALID_OPCODE			= 0x01,
	NVME_SC_INVALID_FIELD			= 0x02,
	NVME_SC_COMMAND_ID_CONFLICT		= 0x03,
	NVME_SC_DATA_TRANSFER_ERROR		= 0x04,
	NVME_SC_ABORTED_POWER_LOSS		= 0x05,
	NVME_SC_INTERNAL_DEVICE_ERROR		= 0x06,
	NVME_SC_ABORTED_BY_REQUEST		= 0x07,
	NVME_SC_ABORTED_SQ_DELETION		= 0x08,
	NVME_SC_ABORTED_FAILED_FUSED		= 0x09,
	NVME_SC_ABORTED_MISSING_FUSED		= 0x0a,
	NVME_SC_INVALID_NAMESPACE_OR_FORMAT	= 0x0b,
	NVME_SC_COMMAND_SEQUENCE_ERROR		= 0x0c,
	NVME_SC_INVALID_SGL_SEG_DESCRIPTOR	= 0x0d,
	NVME_SC_INVALID_NUM_SGL_DESCIRPTORS	= 0x0e,
	NVME_SC_DATA_SGL_LENGTH_INVALID		= 0x0f,
	NVME_SC_METADATA_SGL_LENGTH_INVALID	= 0x10,
	NVME_SC_SGL_DESCRIPTOR_TYPE_INVALID	= 0x11,
	NVME_SC_INVALID_CONTROLLER_MEM_BUF	= 0x12,
	NVME_SC_INVALID_PRP_OFFSET		= 0x13,
	NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED	= 0x14,
	NVME_SC_INVALID_SGL_OFFSET		= 0x16,
	NVME_SC_INVALID_SGL_SUBTYPE		= 0x17,
	NVME_SC_HOSTID_INCONSISTENT_FORMAT	= 0x18,
	NVME_SC_KEEP_ALIVE_EXPIRED		= 0x19,
	NVME_SC_KEEP_ALIVE_INVALID		= 0x1a,

	NVME_SC_LBA_OUT_OF_RANGE		= 0x80,
	NVME_SC_CAPACITY_EXCEEDED		= 0x81,
	NVME_SC_NAMESPACE_NOT_READY		= 0x82,
	NVME_SC_RESERVATION_CONFLICT            = 0x83,
	NVME_SC_FORMAT_IN_PROGRESS              = 0x84,

};

/*
 * Command specific status codes
 */
enum nvme_command_specific_status_code {

	NVME_SC_COMPLETION_QUEUE_INVALID	   = 0x00,
	NVME_SC_INVALID_QUEUE_IDENTIFIER	   = 0x01,
	NVME_SC_MAXIMUM_QUEUE_SIZE_EXCEEDED	   = 0x02,
	NVME_SC_ABORT_COMMAND_LIMIT_EXCEEDED	   = 0x03,

	/* 0x04 - reserved */

	NVME_SC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED = 0x05,
	NVME_SC_INVALID_FIRMWARE_SLOT		   = 0x06,
	NVME_SC_INVALID_FIRMWARE_IMAGE		   = 0x07,
	NVME_SC_INVALID_INTERRUPT_VECTOR	   = 0x08,
	NVME_SC_INVALID_LOG_PAGE		   = 0x09,
	NVME_SC_INVALID_FORMAT			   = 0x0a,
	NVME_SC_FIRMWARE_REQ_CONVENTIONAL_RESET    = 0x0b,
	NVME_SC_INVALID_QUEUE_DELETION             = 0x0c,
	NVME_SC_FEATURE_ID_NOT_SAVEABLE            = 0x0d,
	NVME_SC_FEATURE_NOT_CHANGEABLE             = 0x0e,
	NVME_SC_FEATURE_NOT_NAMESPACE_SPECIFIC     = 0x0f,
	NVME_SC_FIRMWARE_REQ_NVM_RESET             = 0x10,
	NVME_SC_FIRMWARE_REQ_RESET                 = 0x11,
	NVME_SC_FIRMWARE_REQ_MAX_TIME_VIOLATION    = 0x12,
	NVME_SC_FIRMWARE_ACTIVATION_PROHIBITED     = 0x13,
	NVME_SC_OVERLAPPING_RANGE                  = 0x14,
	NVME_SC_NAMESPACE_INSUFFICIENT_CAPACITY    = 0x15,
	NVME_SC_NAMESPACE_ID_UNAVAILABLE           = 0x16,

	/* 0x17 - reserved */

	NVME_SC_NAMESPACE_ALREADY_ATTACHED         = 0x18,
	NVME_SC_NAMESPACE_IS_PRIVATE               = 0x19,
	NVME_SC_NAMESPACE_NOT_ATTACHED             = 0x1a,
	NVME_SC_THINPROVISIONING_NOT_SUPPORTED     = 0x1b,
	NVME_SC_CONTROLLER_LIST_INVALID            = 0x1c,

	NVME_SC_CONFLICTING_ATTRIBUTES		   = 0x80,
	NVME_SC_INVALID_PROTECTION_INFO		   = 0x81,
	NVME_SC_ATTEMPTED_WRITE_TO_RO_PAGE	   = 0x82,

};

/*
 * Media error status codes
 */
enum nvme_media_error_status_code {
	NVME_SC_WRITE_FAULTS			= 0x80,
	NVME_SC_UNRECOVERED_READ_ERROR		= 0x81,
	NVME_SC_GUARD_CHECK_ERROR		= 0x82,
	NVME_SC_APPLICATION_TAG_CHECK_ERROR	= 0x83,
	NVME_SC_REFERENCE_TAG_CHECK_ERROR	= 0x84,
	NVME_SC_COMPARE_FAILURE			= 0x85,
	NVME_SC_ACCESS_DENIED			= 0x86,
	NVME_SC_DEALLOCATED_OR_UNWRITTEN_BLOCK  = 0x87,
};

/*
 * Admin opcodes
 */
enum nvme_admin_opcode {

	NVME_OPC_DELETE_IO_SQ			= 0x00,
	NVME_OPC_CREATE_IO_SQ			= 0x01,
	NVME_OPC_GET_LOG_PAGE			= 0x02,

	/* 0x03 - reserved */

	NVME_OPC_DELETE_IO_CQ			= 0x04,
	NVME_OPC_CREATE_IO_CQ			= 0x05,
	NVME_OPC_IDENTIFY			= 0x06,

	/* 0x07 - reserved */

	NVME_OPC_ABORT				= 0x08,
	NVME_OPC_SET_FEATURES			= 0x09,
	NVME_OPC_GET_FEATURES			= 0x0a,

	/* 0x0b - reserved */

	NVME_OPC_ASYNC_EVENT_REQUEST		= 0x0c,
	NVME_OPC_NS_MANAGEMENT			= 0x0d,

	/* 0x0e-0x0f - reserved */

	NVME_OPC_FIRMWARE_COMMIT		= 0x10,
	NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD	= 0x11,

	NVME_OPC_NS_ATTACHMENT			= 0x15,

	NVME_OPC_KEEP_ALIVE			= 0x18,

	NVME_OPC_FORMAT_NVM			= 0x80,
	NVME_OPC_SECURITY_SEND			= 0x81,
	NVME_OPC_SECURITY_RECEIVE		= 0x82,

};

/*
 * NVM command set opcodes
 */
enum nvme_nvm_opcode {

	NVME_OPC_FLUSH				= 0x00,
	NVME_OPC_WRITE				= 0x01,
	NVME_OPC_READ				= 0x02,

	/* 0x03 - reserved */

	NVME_OPC_WRITE_UNCORRECTABLE		= 0x04,
	NVME_OPC_COMPARE			= 0x05,

	/* 0x06-0x07 - reserved */

	NVME_OPC_WRITE_ZEROES			= 0x08,
	NVME_OPC_DATASET_MANAGEMENT		= 0x09,

	NVME_OPC_RESERVATION_REGISTER		= 0x0d,
	NVME_OPC_RESERVATION_REPORT		= 0x0e,

	NVME_OPC_RESERVATION_ACQUIRE		= 0x11,
	NVME_OPC_RESERVATION_RELEASE		= 0x15,

};

/*
 * Data transfer (bits 1:0) of an NVMe opcode.
 */
enum nvme_data_transfer {

	/*
	 * Opcode does not transfer data.
	 */
	NVME_DATA_NONE				= 0,

	/*
	 * Opcode transfers data from host to controller (e.g. Write).
	 */
	NVME_DATA_HOST_TO_CONTROLLER		= 1,

	/*
	 * Opcode transfers data from controller to host (e.g. Read).
	 */
	NVME_DATA_CONTROLLER_TO_HOST		= 2,

	/*
	 * Opcode transfers data both directions.
	 */
	NVME_DATA_BIDIRECTIONAL			= 3

};

/*
 * Extract the Data Transfer bits from an NVMe opcode.
 *
 * This determines whether a command requires a data buffer and
 * which direction (host to controller or controller to host) it is
 * transferred.
 */
static inline enum nvme_data_transfer nvme_opc_get_data_transfer(uint8_t opc)
{
	return (enum nvme_data_transfer)(opc & 3);
}

/*
 * Features.
 */
enum nvme_feat {

	/* 0x00 - reserved */

	NVME_FEAT_ARBITRATION				= 0x01,
	NVME_FEAT_POWER_MANAGEMENT			= 0x02,
	NVME_FEAT_LBA_RANGE_TYPE			= 0x03,
	NVME_FEAT_TEMPERATURE_THRESHOLD			= 0x04,
	NVME_FEAT_ERROR_RECOVERY			= 0x05,
	NVME_FEAT_VOLATILE_WRITE_CACHE			= 0x06,
	NVME_FEAT_NUMBER_OF_QUEUES			= 0x07,
	NVME_FEAT_INTERRUPT_COALESCING			= 0x08,
	NVME_FEAT_INTERRUPT_VECTOR_CONFIGURATION	= 0x09,
	NVME_FEAT_WRITE_ATOMICITY			= 0x0A,
	NVME_FEAT_ASYNC_EVENT_CONFIGURATION		= 0x0B,
	NVME_FEAT_AUTONOMOUS_POWER_STATE_TRANSITION	= 0x0C,
	NVME_FEAT_HOST_MEM_BUFFER		        = 0x0D,
	NVME_FEAT_KEEP_ALIVE_TIMER			= 0x0F,

	/* 0x0C-0x7F - reserved */

	NVME_FEAT_SOFTWARE_PROGRESS_MARKER		= 0x80,

	/* 0x81-0xBF - command set specific */

	NVME_FEAT_HOST_IDENTIFIER			= 0x81,
	NVME_FEAT_HOST_RESERVE_MASK			= 0x82,
	NVME_FEAT_HOST_RESERVE_PERSIST			= 0x83,

	/* 0xC0-0xFF - vendor specific */

};

/*
 * Get features selection.
 */
enum nvme_feat_sel {
	NVME_FEAT_CURRENT	= 0x0,
	NVME_FEAT_DEFAULT	= 0x1,
	NVME_FEAT_SAVED   	= 0x2,
	NVME_FEAT_SUPPORTED	= 0x3,
};

enum nvme_dsm_attribute {
	NVME_DSM_ATTR_INTEGRAL_READ		= 0x1,
	NVME_DSM_ATTR_INTEGRAL_WRITE		= 0x2,
	NVME_DSM_ATTR_DEALLOCATE		= 0x4,
};

struct nvme_power_state {

	/*
	 * bits 15:00: maximum power.
	 */
	uint16_t mp;

	uint8_t reserved1;

	/*
	 * bit 24: max power scale.
	 */
	uint8_t mps		: 1;

	/*
	 * bit 25: non-operational state.
	 */
	uint8_t nops		: 1;
	uint8_t reserved2	: 6;

	/*
	 * bits 63:32: entry latency in microseconds.
	 */
	uint32_t enlat;

	/*
	 * bits 95:64: exit latency in microseconds.
	 */
	uint32_t exlat;

	/*
	 * bits 100:96: relative read throughput.
	 */
	uint8_t rrt		: 5;
	uint8_t reserved3	: 3;

	/*
	 * bits 108:104: relative read latency.
	 */
	uint8_t rrl		: 5;
	uint8_t reserved4	: 3;

	/*
	 * bits 116:112: relative write throughput.
	 */
	uint8_t rwt		: 5;
	uint8_t reserved5	: 3;

	/*
	 * bits 124:120: relative write latency.
	 */
	uint8_t rwl		: 5;
	uint8_t reserved6	: 3;

	uint8_t reserved7[16];

};
nvme_static_assert(sizeof(struct nvme_power_state) == 32, "Incorrect size");

/*
 * Identify command CNS value
 */
enum nvme_identify_cns {

	/*
	 * Identify namespace indicated in CDW1.NSID.
	 */
	NVME_IDENTIFY_NS			= 0x00,

	/*
	 * Identify controller.
	 */
	NVME_IDENTIFY_CTRLR			= 0x01,

	/*
	 * List active NSIDs greater than CDW1.NSID.
	 */
	NVME_IDENTIFY_ACTIVE_NS_LIST		= 0x02,

	/*
	 * List allocated NSIDs greater than CDW1.NSID.
	 */
	NVME_IDENTIFY_ALLOCATED_NS_LIST		= 0x10,

	/*
	 * Identify namespace if CDW1.NSID is allocated.
	 */
	NVME_IDENTIFY_NS_ALLOCATED		= 0x11,

	/*
	 * Get list of controllers starting at CDW10.CNTID
	 * that are attached to CDW1.NSID.
	 */
	NVME_IDENTIFY_NS_ATTACHED_CTRLR_LIST	= 0x12,

	/*
	 * Get list of controllers starting at CDW10.CNTID.
	 */
	NVME_IDENTIFY_CTRLR_LIST	      	= 0x13,
};

/*
 * NVMe over Fabrics controller model.
 */
enum nvmf_ctrlr_model {

	/*
	 * NVM subsystem uses dynamic controller model.
	 */
	NVMF_CTRLR_MODEL_DYNAMIC		= 0,

	/*
	 * NVM subsystem uses static controller model.
	 */
	NVMF_CTRLR_MODEL_STATIC			= 1,

};

struct __attribute__((packed)) nvme_ctrlr_data {

	/* Bytes 0-255: controller capabilities and features */

	/*
	 * PCI vendor id.
	 */
	uint16_t		vid;

	/*
	 * PCI subsystem vendor id.
	 */
	uint16_t		ssvid;

	/*
	 * Serial number.
	 */
	int8_t			sn[NVME_SERIAL_NUMBER_CHARACTERS];

	/*
	 * Model number.
	 */
	int8_t			mn[NVME_MODEL_NUMBER_CHARACTERS];

	/*
	 * Firmware revision.
	 */
	uint8_t			fr[8];

	/*
	 * Recommended arbitration burst.
	 */
	uint8_t			rab;

	/*
	 * IEEE oui identifier.
	 */
	uint8_t			ieee[3];

	/*
	 * Controller multi-path I/O and namespace sharing capabilities.
	 */
	struct {
		uint8_t 	multi_port	: 1;
		uint8_t 	multi_host	: 1;
		uint8_t 	sr_iov		: 1;
		uint8_t 	reserved	: 5;
	} cmic;

	/*
	 * Maximum data transfer size.
	 */
	uint8_t			mdts;

	/*
	 * Controller ID.
	 */
	uint16_t		cntlid;

	/*
	 * Version.
	 */
	union nvme_vs_register	ver;

	/*
	 * RTD3 resume latency.
	 */
	uint32_t		rtd3r;

	/*
	 * RTD3 entry latency.
	 */
	uint32_t		rtd3e;

	/*
	 * Optional asynchronous events supported.
	 */
	uint32_t		oaes;

	/*
	 * Controller attributes.
	 */
	struct {
		uint32_t	host_id_exhid_supported: 1;
		uint32_t	reserved: 31;
	} ctratt;

	uint8_t			reserved1[156];

	/*
	 * Bytes 256-511: admin command set attributes.
	 */

	/*
	 * Optional admin command support.
	 */
	struct {
		/*
		 * Supports security send/receive commands.
		 */
		uint16_t	security  : 1;

		/*
		 * Supports format nvm command.
		 */
		uint16_t	format    : 1;

		/*
		 * Supports firmware activate/download commands.
		 */
		uint16_t	firmware  : 1;

		/*
		 * Supports ns manage/ns attach commands.
		 */
		uint16_t	ns_manage : 1;

		uint16_t	oacs_rsvd : 12;
	} oacs;

	/*
	 * Abort command limit.
	 */
	uint8_t			acl;

	/*
	 * Asynchronous event request limit.
	 */
	uint8_t			aerl;

	/*
	 * Firmware updates.
	 */
	struct {
		/*
		 * First slot is read-only.
		 */
		uint8_t		slot1_ro  : 1;

		/*
		 * Number of firmware slots.
		 */
		uint8_t		num_slots : 3;

		/*
		 * Support activation without reset.
		 */
		uint8_t		activation_without_reset : 1;

		uint8_t		frmw_rsvd : 3;
	} frmw;

	/*
	 * Log page attributes.
	 */
	struct {
		/*
		 * Per namespace smart/health log page.
		 */
		uint8_t		ns_smart : 1;
		/*
		 * Command effects log page.
		 */
		uint8_t		celp     : 1;
		/*
		 * Extended data for get log page.
		 */
		uint8_t		edlp     : 1;
		uint8_t		lpa_rsvd : 5;
	} lpa;

	/*
	 * Error log page entries.
	 */
	uint8_t			elpe;

	/*
	 * Number of power states supported.
	 */
	uint8_t			npss;

	/*
	 * Admin vendor specific command configuration.
	 */
	struct {
		/*
		 * Admin vendor specific commands use disk format.
		 */
		uint8_t		spec_format : 1;
		uint8_t		avscc_rsvd  : 7;
	} avscc;

	/*
	 * Autonomous power state transition attributes.
	 */
	struct {
		uint8_t		supported  : 1;
		uint8_t		apsta_rsvd : 7;
	} apsta;

	/*
	 * Warning composite temperature threshold.
	 */
	uint16_t		wctemp;

	/*
	 * Critical composite temperature threshold.
	 */
	uint16_t		cctemp;

	/*
	 * Maximum time for firmware activation.
	 */
	uint16_t		mtfa;

	/*
	 * Host memory buffer preferred size.
	 */
	uint32_t		hmpre;

	/*
	 * Host memory buffer minimum size.
	 */
	uint32_t		hmmin;

	/*
	 * Total NVM capacity.
	 */
	uint64_t		tnvmcap[2];

	/*
	 * Unallocated NVM capacity.
	 */
	uint64_t		unvmcap[2];

	/*
	 * Replay protected memory block support.
	 */
	struct {
		uint8_t		num_rpmb_units	: 3;
		uint8_t		auth_method	: 3;
		uint8_t		reserved1	: 2;

		uint8_t		reserved2;

		uint8_t		total_size;
		uint8_t		access_size;
	} rpmbs;

	uint8_t			reserved2[4];

	uint16_t		kas;

	uint8_t			reserved3[190];

	/*
	 * Bytes 512-703: nvm command set attributes.
	 */

	/*
	 * Submission queue entry size.
	 */
	struct {
		uint8_t		min : 4;
		uint8_t		max : 4;
	} sqes;

	/*
	 * Completion queue entry size.
	 */
	struct {
		uint8_t		min : 4;
		uint8_t		max : 4;
	} cqes;

	uint16_t		maxcmd;

	/*
	 * Number of namespaces.
	 */
	uint32_t		nn;

	/*
	 * Optional nvm command support.
	 */
	struct {
		uint16_t	compare           : 1;
		uint16_t	write_unc         : 1;
		uint16_t	dsm               : 1;
		uint16_t	write_zeroes      : 1;
		uint16_t	set_features_save : 1;
		uint16_t	reservations      : 1;
		uint16_t	reserved          : 10;
	} oncs;

	/*
	 * Fused operation support.
	 */
	uint16_t		fuses;

	/*
	 * Format nvm attributes.
	 */
	struct {
		uint8_t		format_all_ns          : 1;
		uint8_t		erase_all_ns           : 1;
		uint8_t		crypto_erase_supported : 1;
		uint8_t		reserved               : 5;
	} fna;

	/*
	 * Volatile write cache.
	 */
	struct {
		uint8_t		present  : 1;
		uint8_t		reserved : 7;
	} vwc;

	/*
	 * Atomic write unit normal.
	 */
	uint16_t		awun;

	/*
	 * Atomic write unit power fail.
	 */
	uint16_t		awupf;

	/*
	 * NVM vendor specific command configuration.
	 */
	uint8_t			nvscc;

	uint8_t			reserved531;

	/*
	 * Atomic compare & write unit.
	 */
	uint16_t		acwu;

	uint16_t		reserved534;

	/*
	 * SGL support.
	 */
	struct {
		uint32_t	supported : 1;
		uint32_t	reserved0 : 1;
		uint32_t	keyed_sgl : 1;
		uint32_t	reserved1 : 13;
		uint32_t	bit_bucket_descriptor : 1;
		uint32_t	metadata_pointer      : 1;
		uint32_t	oversized_sgl         : 1;
		uint32_t	metadata_address      : 1;
		uint32_t	sgl_offset : 1;
		uint32_t	reserved2  : 11;
	} sgls;

	uint8_t			reserved4[228];

	uint8_t			subnqn[256];

	uint8_t			reserved5[768];

	/*
	 * NVMe over Fabrics-specific fields.
	 */
	struct {
		/*
		 * I/O queue command capsule supported size (16-byte units).
		 */
		uint32_t	ioccsz;

		/*
		 * I/O queue response capsule supported size (16-byte units).
		 */
		uint32_t	iorcsz;

		/*
		 * In-capsule data offset (16-byte units).
		 */
		uint16_t	icdoff;

		/*
		 * Controller attributes: model.
		 */
		struct {
			uint8_t	ctrlr_model : 1;
			uint8_t reserved    : 7;
		} ctrattr;

		/*
		 * Maximum SGL block descriptors (0 = no limit).
		 */
		uint8_t		msdbd;

		uint8_t		reserved[244];
	} nvmf_specific;

	/*
	 * Bytes 2048-3071: power state descriptors.
	 */
	struct nvme_power_state	psd[32];

	/*
	 * Bytes 3072-4095: vendor specific.
	 */
	uint8_t			vs[1024];

};
nvme_static_assert(sizeof(struct nvme_ctrlr_data) == 4096, "Incorrect size");

struct nvme_ns_data {

	/*
	 * Namespace size (number of sectors).
	 */
	uint64_t		nsze;

	/*
	 * Namespace capacity.
	 */
	uint64_t		ncap;

	/*
	 * Namespace utilization.
	 */
	uint64_t		nuse;

	/*
	 * Namespace features.
	 */
	struct {
		/*
		 * Thin provisioning.
		 */
		uint8_t		thin_prov : 1;
		uint8_t		reserved1 : 7;
	} nsfeat;

	/*
	 * Number of lba formats.
	 */
	uint8_t			nlbaf;

	/*
	 * Formatted lba size.
	 */
	struct {
		uint8_t		format    : 4;
		uint8_t		extended  : 1;
		uint8_t		reserved2 : 3;
	} flbas;

	/*
	 * Metadata capabilities.
	 */
	struct {
		/*
		 * Metadata can be transferred as part of data prp list.
		 */
		uint8_t		extended  : 1;

		/*
		 * Metadata can be transferred with separate metadata pointer.
		 */
		uint8_t		pointer   : 1;

		uint8_t		reserved3 : 6;
	} mc;

	/*
	 * End-to-end data protection capabilities.
	 */
	struct {
		/*
		 * Protection information type 1.
		 */
		uint8_t		pit1     : 1;

		/*
		 * Protection information type 2.
		 */
		uint8_t		pit2     : 1;

		/*
		 * Protection information type 3.
		 */
		uint8_t		pit3     : 1;

		/*
		 * First eight bytes of metadata.
		 */
		uint8_t		md_start : 1;

		/*
		 * Last eight bytes of metadata.
		 */
		uint8_t		md_end   : 1;
	} dpc;

	/*
	 * End-to-end data protection type settings.
	 */
	struct {
		/*
		 * Protection information type.
		 */
		uint8_t		pit       : 3;

		/*
		 * 1 == protection info transferred at start of metadata.
		 * 0 == protection info transferred at end of metadata.
		 */
		uint8_t		md_start  : 1;

		uint8_t		reserved4 : 4;
	} dps;

	/*
	 * Namespace multi-path I/O and namespace sharing capabilities.
	 */
	struct {
		uint8_t		can_share : 1;
		uint8_t		reserved  : 7;
	} nmic;

	/*
	 * Reservation capabilities.
	 */
	union {
		struct {
			/*
			 * Supports persist through power loss.
			 */
			uint8_t		persist : 1;

			/*
			 * Supports write exclusive.
			 */
			uint8_t		write_exclusive : 1;

			/*
			 * Supports exclusive access.
			 */
			uint8_t		exclusive_access : 1;

			/*
			 * Supports write exclusive - registrants only.
			 */
			uint8_t		write_exclusive_reg_only : 1;

			/*
			 * Supports exclusive access - registrants only.
			 */
			uint8_t		exclusive_access_reg_only : 1;

			/*
			 * Supports write exclusive - all registrants.
			 */
			uint8_t		write_exclusive_all_reg : 1;

			/*
			 * Supports exclusive access - all registrants.
			 */
			uint8_t		exclusive_access_all_reg : 1;

			uint8_t		reserved : 1;
		} rescap;
		uint8_t		raw;
	} nsrescap;

	/*
	 * Format progress indicator.
	 */
	struct {
		uint8_t		percentage_remaining : 7;
		uint8_t		fpi_supported        : 1;
	} fpi;

	uint8_t			reserved33;

	/*
	 * Namespace atomic write unit normal.
	 */
	uint16_t		nawun;

	/*
	 * Namespace atomic write unit power fail.
	 */
	uint16_t		nawupf;

	/*
	 * Namespace atomic compare & write unit.
	 */
	uint16_t		nacwu;

	/*
	 * Namespace atomic boundary size normal.
	 */
	uint16_t		nabsn;

	/*
	 * Namespace atomic boundary offset.
	 */
	uint16_t		nabo;

	/*
	 * Namespace atomic boundary size power fail.
	 */
	uint16_t		nabspf;

	uint16_t		reserved46;

	/*
	 * NVM capacity.
	 */
	uint64_t		nvmcap[2];

	uint8_t			reserved64[40];

	/*
	 * Namespace globally unique identifier.
	 */
	uint8_t			nguid[16];

	/*
	 * IEEE extended unique identifier.
	 */
	uint64_t		eui64;

	/*
	 * LBA format support.
	 */
	struct {
		/*
		 * Metadata size.
		 */
		uint32_t	ms	  : 16;

		/*
		 * LBA data size.
		 */
		uint32_t	lbads	  : 8;

		/*
		 * Relative performance.
		 */
		uint32_t	rp	  : 2;

		uint32_t	reserved6 : 6;
	} lbaf[16];

	uint8_t			reserved6[192];

	uint8_t			vendor_specific[3712];
};
nvme_static_assert(sizeof(struct nvme_ns_data) == 4096, "Incorrect size");

/*
 * Reservation Type Encoding
 */
enum nvme_reservation_type {

	/* 0x00 - reserved */

	/*
	 * Write Exclusive Reservation.
	 */
	NVME_RESERVE_WRITE_EXCLUSIVE		= 0x1,

	/*
	 * Exclusive Access Reservation.
	 */
	NVME_RESERVE_EXCLUSIVE_ACCESS		= 0x2,

	/*
	 * Write Exclusive - Registrants Only Reservation.
	 */
	NVME_RESERVE_WRITE_EXCLUSIVE_REG_ONLY	= 0x3,

	/*
	 * Exclusive Access - Registrants Only Reservation.
	 */
	NVME_RESERVE_EXCLUSIVE_ACCESS_REG_ONLY	= 0x4,

	/*
	 * Write Exclusive - All Registrants Reservation.
	 */
	NVME_RESERVE_WRITE_EXCLUSIVE_ALL_REGS	= 0x5,

	/*
	 * Exclusive Access - All Registrants Reservation.
	 */
	NVME_RESERVE_EXCLUSIVE_ACCESS_ALL_REGS	= 0x6,

	/* 0x7-0xFF - Reserved */
};

struct nvme_reservation_acquire_data {

	/*
	 * Current reservation key.
	 */
	uint64_t		crkey;

	/*
	 * Preempt reservation key.
	 */
	uint64_t		prkey;

};
nvme_static_assert(sizeof(struct nvme_reservation_acquire_data) == 16,
		   "Incorrect size");

/*
 * Reservation Acquire action
 */
enum nvme_reservation_acquire_action {
	NVME_RESERVE_ACQUIRE		= 0x0,
	NVME_RESERVE_PREEMPT		= 0x1,
	NVME_RESERVE_PREEMPT_ABORT	= 0x2,
};

struct __attribute__((packed)) nvme_reservation_status_data {

	/*
	 * Reservation action generation counter.
	 */
	uint32_t		generation;

	/*
	 * Reservation type.
	 */
	uint8_t			type;

	/*
	 * Number of registered controllers.
	 */
	uint16_t		nr_regctl;
	uint16_t		reserved1;

	/*
	 * Persist through power loss state.
	 */
	uint8_t			ptpl_state;
	uint8_t			reserved[14];

};
nvme_static_assert(sizeof(struct nvme_reservation_status_data) == 24,
		   "Incorrect size");

struct __attribute__((packed)) nvme_reservation_ctrlr_data {

	uint16_t		ctrlr_id;

	/*
	 * Reservation status.
	 */
	struct {
		uint8_t		status    : 1;
		uint8_t		reserved1 : 7;
	} rcsts;
	uint8_t			reserved2[5];

	/*
	 * Host identifier.
	 */
	uint64_t		host_id;

	/*
	 * Reservation key.
	 */
	uint64_t		key;
};
nvme_static_assert(sizeof(struct nvme_reservation_ctrlr_data) == 24,
		   "Incorrect size");

/*
 * Change persist through power loss state for
 * Reservation Register command
 */
enum nvme_reservation_register_cptpl {
	NVME_RESERVE_PTPL_NO_CHANGES		= 0x0,
	NVME_RESERVE_PTPL_CLEAR_POWER_ON	= 0x2,
	NVME_RESERVE_PTPL_PERSIST_POWER_LOSS	= 0x3,
};

/*
 * Registration action for Reservation Register command
 */
enum nvme_reservation_register_action {
	NVME_RESERVE_REGISTER_KEY		= 0x0,
	NVME_RESERVE_UNREGISTER_KEY	        = 0x1,
	NVME_RESERVE_REPLACE_KEY		= 0x2,
};

struct nvme_reservation_register_data {

	/*
	 * Current reservation key.
	 */
	uint64_t		crkey;

	/*
	 * New reservation key.
	 */
	uint64_t		nrkey;

};
nvme_static_assert(sizeof(struct nvme_reservation_register_data) == 16,
		   "Incorrect size");

struct nvme_reservation_key_data {

	/*
	 * Current reservation key.
	 */
	uint64_t		crkey;

};
nvme_static_assert(sizeof(struct nvme_reservation_key_data) == 8,
		   "Incorrect size");

/*
 * Reservation Release action
 */
enum nvme_reservation_release_action {
	NVME_RESERVE_RELEASE		= 0x0,
	NVME_RESERVE_CLEAR	        = 0x1,
};

/*
 * Log page identifiers for NVME_OPC_GET_LOG_PAGE
 */
enum nvme_log_page {

	/* 0x00 - reserved */

	/*
	 * Error information (mandatory).
	 */
	NVME_LOG_ERROR			        = 0x01,

	/*
	 * SMART / health information (mandatory).
	 */
	NVME_LOG_HEALTH_INFORMATION	        = 0x02,

	/*
	 * Firmware slot information (mandatory).
	 */
	NVME_LOG_FIRMWARE_SLOT		        = 0x03,

	/*
	 * Changed namespace list (optional).
	 */
	NVME_LOG_CHANGED_NS_LIST	        = 0x04,

	/*
	 * Command effects log (optional).
	 */
	NVME_LOG_COMMAND_EFFECTS_LOG	        = 0x05,

	/* 0x06-0x6F - reserved */

	/*
	 * Discovery(refer to the NVMe over Fabrics specification).
	 */
	NVME_LOG_DISCOVERY		        = 0x70,

	/* 0x71-0x7f - reserved for NVMe over Fabrics */

	/*
	 * Reservation notification (optional).
	 */
	NVME_LOG_RESERVATION_NOTIFICATION	= 0x80,

	/* 0x81-0xBF - I/O command set specific */

	/* 0xC0-0xFF - vendor specific */
};

/*
 * Error information log page (\ref NVME_LOG_ERROR)
 */
struct nvme_error_information_entry {
	uint64_t		error_count;
	uint16_t		sqid;
	uint16_t		cid;
	struct nvme_status	status;
	uint16_t		error_location;
	uint64_t		lba;
	uint32_t		nsid;
	uint8_t			vendor_specific;
	uint8_t			reserved[35];
};
nvme_static_assert(sizeof(struct nvme_error_information_entry) == 64,
		   "Incorrect size");

union nvme_critical_warning_state {

	uint8_t		raw;

	struct {
		uint8_t	available_spare		: 1;
		uint8_t	temperature		: 1;
		uint8_t	device_reliability	: 1;
		uint8_t	read_only		: 1;
		uint8_t	volatile_memory_backup	: 1;
		uint8_t	reserved		: 3;
	} bits;

};
nvme_static_assert(sizeof(union nvme_critical_warning_state) == 1,
		   "Incorrect size");

/*
 * SMART / health information page (\ref NVME_LOG_HEALTH_INFORMATION)
 */
struct __attribute__((packed)) nvme_health_information_page {

	union nvme_critical_warning_state critical_warning;

	uint16_t		temperature;
	uint8_t			available_spare;
	uint8_t			available_spare_threshold;
	uint8_t			percentage_used;

	uint8_t			reserved[26];

	/*
	 * Note that the following are 128-bit values, but are
	 * defined as an array of 2 64-bit values.
	 */

	/*
	 * Data Units Read is always in 512-byte units.
	 */
	uint64_t		data_units_read[2];

	/*
	 * Data Units Written is always in 512-byte units.
	 */
	uint64_t		data_units_written[2];

	/*
	 * For NVM command set, this includes Compare commands.
	 */
	uint64_t		host_read_commands[2];
	uint64_t		host_write_commands[2];

	/*
	 * Controller Busy Time is reported in minutes.
	 */
	uint64_t		controller_busy_time[2];
	uint64_t		power_cycles[2];
	uint64_t		power_on_hours[2];
	uint64_t		unsafe_shutdowns[2];
	uint64_t		media_errors[2];
	uint64_t		num_error_info_log_entries[2];

	uint8_t			reserved2[320];
};
nvme_static_assert(sizeof(struct nvme_health_information_page) == 512,
		   "Incorrect size");

/*
 * Firmware slot information page (\ref NVME_LOG_FIRMWARE_SLOT)
 */
struct nvme_firmware_page {

	struct {
		/*
		 * Slot for current FW.
		 */
		uint8_t	slot		: 3;
		uint8_t	reserved	: 5;
	} afi;

	uint8_t			reserved[7];

	/*
	 * Revisions for 7 slots.
	 */
	uint64_t		revision[7];

	uint8_t			reserved2[448];

};
nvme_static_assert(sizeof(struct nvme_firmware_page) == 512,
		   "Incorrect size");

/*
 * Namespace attachment Type Encoding
 */
enum nvme_ns_attach_type {

	/*
	 * Controller attach.
	 */
	NVME_NS_CTRLR_ATTACH	= 0x0,

	/*
	 * Controller detach.
	 */
	NVME_NS_CTRLR_DETACH	= 0x1,

	/* 0x2-0xF - Reserved */

};

/*
 * Namespace management Type Encoding
 */
enum nvme_ns_management_type {

	/*
	 * Create.
	 */
	NVME_NS_MANAGEMENT_CREATE	= 0x0,

	/*
	 * Delete.
	 */
	NVME_NS_MANAGEMENT_DELETE	= 0x1,

	/* 0x2-0xF - Reserved */

};

struct nvme_ns_list {
	uint32_t ns_list[NVME_MAX_NS];
};
nvme_static_assert(sizeof(struct nvme_ns_list) == 4096, "Incorrect size");

struct nvme_ctrlr_list {
	uint16_t ctrlr_count;
	uint16_t ctrlr_list[2047];
};
nvme_static_assert(sizeof(struct nvme_ctrlr_list) == 4096, "Incorrect size");

enum nvme_secure_erase_setting {
	NVME_FMT_NVM_SES_NO_SECURE_ERASE	= 0x0,
	NVME_FMT_NVM_SES_USER_DATA_ERASE	= 0x1,
	NVME_FMT_NVM_SES_CRYPTO_ERASE	        = 0x2,
};

enum nvme_pi_location {
	NVME_FMT_NVM_PROTECTION_AT_TAIL	        = 0x0,
	NVME_FMT_NVM_PROTECTION_AT_HEAD	        = 0x1,
};

enum nvme_pi_type {
	NVME_FMT_NVM_PROTECTION_DISABLE		= 0x0,
	NVME_FMT_NVM_PROTECTION_TYPE1		= 0x1,
	NVME_FMT_NVM_PROTECTION_TYPE2		= 0x2,
	NVME_FMT_NVM_PROTECTION_TYPE3		= 0x3,
};

enum nvme_metadata_setting {
	NVME_FMT_NVM_METADATA_TRANSFER_AS_BUFFER	= 0x0,
	NVME_FMT_NVM_METADATA_TRANSFER_AS_LBA	        = 0x1,
};

struct nvme_format {
	uint32_t	lbaf		: 4;
	uint32_t	ms		: 1;
	uint32_t	pi		: 3;
	uint32_t	pil		: 1;
	uint32_t	ses		: 3;
	uint32_t	reserved	: 20;
};
nvme_static_assert(sizeof(struct nvme_format) == 4, "Incorrect size");

struct nvme_protection_info {
	uint16_t	guard;
	uint16_t	app_tag;
	uint32_t	ref_tag;
};
nvme_static_assert(sizeof(struct nvme_protection_info) == 8, "Incorrect size");

/*
 * Parameters for NVME_OPC_FIRMWARE_COMMIT cdw10: commit action.
 */
enum nvme_fw_commit_action {

	/*
	 * Downloaded image replaces the image specified by
	 * the Firmware Slot field. This image is not activated.
	 */
	NVME_FW_COMMIT_REPLACE_IMG		= 0x0,

	/*
	 * Downloaded image replaces the image specified by
	 * the Firmware Slot field. This image is activated at the next reset.
	 */
	NVME_FW_COMMIT_REPLACE_AND_ENABLE_IMG	= 0x1,

	/*
	 * The image specified by the Firmware Slot field is
	 * activated at the next reset.
	 */
	NVME_FW_COMMIT_ENABLE_IMG		= 0x2,

	/*
	 * The image specified by the Firmware Slot field is
	 * requested to be activated immediately without reset.
	 */
	NVME_FW_COMMIT_RUN_IMG			= 0x3,

};

/*
 * Parameters for NVME_OPC_FIRMWARE_COMMIT cdw10.
 */
struct nvme_fw_commit {

	/*
	 * Firmware Slot. Specifies the firmware slot that shall be used for the
	 * Commit Action. The controller shall choose the firmware slot (slot 1 - 7)
	 * to use for the operation if the value specified is 0h.
	 */
	uint32_t	fs		: 3;

	/*
	 * Commit Action. Specifies the action that is taken on the image downloaded
	 * with the Firmware Image Download command or on a previously downloaded and
	 * placed image.
	 */
	uint32_t	ca		: 3;

	uint32_t	reserved	: 26;

};
nvme_static_assert(sizeof(struct nvme_fw_commit) == 4, "Incorrect size");

#define nvme_cpl_is_error(cpl)					\
	((cpl)->status.sc != 0 || (cpl)->status.sct != 0)

/*
 * Enable protection information checking of the
 * Logical Block Reference Tag field.
 */
#define NVME_IO_FLAGS_PRCHK_REFTAG      (1U << 26)

/*
 * Enable protection information checking of the
 * Application Tag field.
 */
#define NVME_IO_FLAGS_PRCHK_APPTAG      (1U << 27)

/*
 * Enable protection information checking of the Guard field.
 */
#define NVME_IO_FLAGS_PRCHK_GUARD       (1U << 28)

/*
 * Strip or insert (when set) the protection information.
 */
#define NVME_IO_FLAGS_PRACT             (1U << 29)

/*
 * Bypass device cache.
 */
#define NVME_IO_FLAGS_FORCE_UNIT_ACCESS (1U << 30)

/*
 * Limit retries on error.
 */
#define NVME_IO_FLAGS_LIMITED_RETRY     (1U << 31)

#endif /* define __LIBNVME_SPEC_H__ */

