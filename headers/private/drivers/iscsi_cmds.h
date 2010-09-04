/*
 * Copyright 2010, Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ISCSI_CMDS_H
#define ISCSI_CMDS_H


#include <ByteOrder.h>


// TODO Add Little Endian support by introducing more macros
#if __BYTE_ORDER != __BIG_ENDIAN
#error Only Big Endian systems supported yet.
#endif


// iSCSI Basic Header Segment (BHS) (RFC 3720 10.2.1)

#define ISCSI_BHS_BYTE0 			\
	bool reserved : 1;				\
	bool immediateDelivery : 1;		\
	uint8 opcode : 6;

// TODO This macro is LE-incompatible
#define ISCSI_BHS_START 			\
	ISCSI_BHS_BYTE0					\
	bool final : 1;

#define ISCSI_BHS_LENGTHS 			\
	uint8 totalAHSLength; 			\
	uint32 dataSegmentLength : 24;

#define ISCSI_BHS_TASK_TAG 			\
	uint32 initiatorTaskTag;

#define ISCSI_BHS_TAGS				\
	ISCSI_BHS_TASK_TAG				\
	uint32 targetTransferTag;

struct iscsi_basic_header_segment {
	ISCSI_BHS_START
	uint32 opcodeSpecific : 23;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TASK_TAG
	uint8 opcodeSpecific2[28];
} _PACKED;

// initiator opcodes
#define ISCSI_OPCODE_NOP_OUT			0x00
#define ISCSI_OPCODE_SCSI_COMMAND		0x01
#define ISCSI_OPCODE_LOGIN_REQUEST		0x03
#define ISCSI_OPCODE_TEXT_REQUEST		0x04
#define ISCSI_OPCODE_LOGOUT_REQUEST		0x06
// target opcodes
#define ISCSI_OPCODE_NOP_IN				0x20
#define ISCSI_OPCODE_SCSI_RESPONSE		0x21
#define ISCSI_OPCODE_LOGIN_RESPONSE 	0x23
#define ISCSI_OPCODE_TEXT_RESPONSE		0x24
#define ISCSI_OPCODE_SCSI_DATA_IN		0x25
#define ISCSI_OPCODE_LOGOUT_RESPONSE	0x26

// SCSI Command (RFC 3720 10.3)
struct iscsi_scsi_command {
	iscsi_scsi_command()
		:
		reserved(0),
		opcode(ISCSI_OPCODE_SCSI_COMMAND),
		reserved2(0),
		reserved3(0)
	{
	}
	ISCSI_BHS_START
	bool r : 1;
	bool w : 1;
	uint8 reserved2 : 2;
	uint8 attr : 3;
	uint16 reserved3;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TASK_TAG
	uint32 expectedDataTransferLength;
	uint32 cmdSN;
	uint32 expStatSN;
	uint8 cdb[16];
} _PACKED;

// SCSI Response (RFC 3720 10.4)
struct iscsi_scsi_response {
	ISCSI_BHS_START
	uint8 reserved2 : 2;
	bool o : 1;
	bool u : 1;
	bool O : 1;
	bool U : 1;
	bool reserved3 : 1;
	uint8 response;
	uint8 status;
	ISCSI_BHS_LENGTHS
	uint32 reserved4[2];
	ISCSI_BHS_TASK_TAG
	uint32 snackTag;
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint32 expDataSN;
	uint32 bidirectionalReadResidualCount;
	uint32 residualCount;
} _PACKED;

// SCSI Data-In (RFC 3270 10.7)
struct iscsi_scsi_data_in {
	ISCSI_BHS_START
	bool acknowledge : 1;
	uint8 reserved2 : 3;
	bool O : 1;
	bool U : 1;
	bool S : 1;
	uint8 reserved3;
	uint8 status;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TAGS
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint32 dataSN;
	uint32 bufferOffset;
	uint32 residualCount;
} _PACKED;

// Text Request (RFC 3720 10.10)
struct iscsi_text_request {
	iscsi_text_request()
		:
		reserved(0),
		opcode(ISCSI_OPCODE_TEXT_REQUEST),
		reserved2(2)
	{
		reserved3[0] = 0;
		reserved3[1] = 0;
		reserved3[2] = 0;
		reserved3[3] = 0;
	}
	ISCSI_BHS_START
	bool c : 1;				// continue
	uint32 reserved2 : 22;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TAGS
	uint32 cmdSN;
	uint32 expStatSN;
	uint32 reserved3[4];
} _PACKED;

// Text Response (RFC 3720 10.11)
struct iscsi_text_response {
	ISCSI_BHS_START
	bool c : 1; // continue
	uint32 reserved2 : 22;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TAGS
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint32 reserved3[3];
} _PACKED;

struct iscsi_isid {
	uint8 t : 2;
	uint8 a : 6;
	uint16 b;
	uint8 c;
	uint16 d;
} _PACKED;

// Login Request (RFC 3720 10.12)
struct iscsi_login_request {
	iscsi_login_request()
		:
		reserved(false),
		immediateDelivery(1),
		opcode(ISCSI_OPCODE_LOGIN_REQUEST),
		reserved2(0),
		reserved3(0)
	{
		memset(reserved4, 0, sizeof(reserved4));
	}

	ISCSI_BHS_BYTE0
	bool transit : 1;
	bool c : 1;					// continue
	uint8 reserved2 : 2;
	uint8 currentStage : 2;
	uint8 nextStage : 2;
	uint8 versionMax;
	uint8 versionMin;
	ISCSI_BHS_LENGTHS
	iscsi_isid isid;
	uint16 tsih;
	ISCSI_BHS_TASK_TAG
	uint16 cid;
	uint16 reserved3;
	uint32 cmdSN;
	uint32 expStatSN;
	uint32 reserved4[4];
} _PACKED;

#define ISCSI_SESSION_STAGE_SECURITY_NEGOTIATION			0
#define ISCSI_SESSION_STAGE_LOGIN_OPERATIONAL_NEGOTIATION	1
#define ISCSI_SESSION_STAGE_FULL_FEATURE_PHASE				3

#define ISCSI_VERSION 0x00

#define ISCSI_ISID_OUI		0
#define ISCSI_ISID_EN		1
#define ISCSI_ISID_RANDOM	2

// Login Response (RFC 3720 10.13)
struct iscsi_login_response {
	ISCSI_BHS_BYTE0
	bool transit : 1;
	bool c : 1;					// continue
	uint8 reserved2 : 2;
	uint8 currentStage : 2;
	uint8 nextStage : 2;
	uint8 versionMax;
	uint8 versionActive;
	ISCSI_BHS_LENGTHS
	iscsi_isid isid;
	uint16 tsih;
	ISCSI_BHS_TASK_TAG
	uint32 reserved3;
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint8 statusClass;
	uint8 statusDetail;
	uint16 reserved4;
	uint32 reserved5[2];
} _PACKED;

// Logout Request (RFC 3720 10.14)
struct iscsi_logout_request {
	iscsi_logout_request()
		:
		reserved(0),
		opcode(ISCSI_OPCODE_LOGOUT_REQUEST),
		final(true),
		reserved2(0),
		reserved4(0)
	{
		reserved3[0] = 0;
		reserved3[1] = 0;
		reserved5[0] = 0;
		reserved5[1] = 0;
		reserved5[2] = 0;
		reserved5[3] = 0;
	}
	ISCSI_BHS_START
	uint8 reasonCode : 7;
	uint16 reserved2;
	ISCSI_BHS_LENGTHS
	uint32 reserved3[2];
	ISCSI_BHS_TASK_TAG
	uint16 cid;
	uint16 reserved4;
	uint32 cmdSN;
	uint32 expStatSN;
	uint32 reserved5[4];
} _PACKED;

#define ISCSI_LOGOUT_REASON_CLOSE_SESSION		0
#define ISCSI_LOGOUT_REASON_CLOSE_CONNECTION	1
#define ISCSI_LOGOUT_REASON_REMOVE_CONNECTION	2

// Logout Response (RFC 3720 10.15)
struct iscsi_logout_response {
	ISCSI_BHS_START
	uint8 reserved2 : 7;
	uint8 response;
	uint8 reserved3;
	ISCSI_BHS_LENGTHS
	uint32 reserved4[2];
	ISCSI_BHS_TASK_TAG
	uint32 reserved5;
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint32 reserved6;
	uint16 time2Wait;
	uint16 time2Remain;
	uint32 reserved7;
} _PACKED;

// NOP-Out (RFC 3270, 10.18)
struct iscsi_nop_out {
	iscsi_nop_out()
		:
		reserved(0),
		opcode(ISCSI_OPCODE_NOP_OUT),
		final(true),
		reserved2(0)
	{
		reserved3[0] = 0;
		reserved3[1] = 0;
		reserved3[2] = 0;
		reserved3[3] = 0;
	}
	ISCSI_BHS_START
	uint32 reserved2 : 23;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TAGS
	uint32 cmdSN;
	uint32 expStatSN;
	uint32 reserved3[4];
} _PACKED;

// NOP-In (RFC 3270, 10.19)
struct iscsi_nop_in {
	ISCSI_BHS_START
	uint32 reserved2 : 23;
	ISCSI_BHS_LENGTHS
	uint64 lun;
	ISCSI_BHS_TAGS
	uint32 statSN;
	uint32 expCmdSN;
	uint32 maxCmdSN;
	uint32 reserved3[3];
} _PACKED;


#endif
