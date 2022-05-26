/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _VIRTIO_DEFS_H_
#define _VIRTIO_DEFS_H_


#include <SupportDefs.h>


enum {
	kVirtioRegsSize = 0x1000,
	kVirtioSignature = 0x74726976,
	kVirtioVendorId = 0x554d4551,
};

enum {
	kVirtioDevNet     =  1,
	kVirtioDevBlock   =  2,
	kVirtioDevConsole =  3,
	kVirtioDev9p      =  9,
	kVirtioDevInput   = 18,
};

enum {
	kVirtioConfigSAcknowledge = 1 << 0,
	kVirtioConfigSDriver      = 1 << 1,
	kVirtioConfigSDriverOk    = 1 << 2,
	kVirtioConfigSFeaturesOk  = 1 << 3,
};

// VirtioRegs.interruptStatus, interruptAck
enum {
	kVirtioIntQueue  = 1 << 0,
	kVirtioIntConfig = 1 << 1,
};

enum {
	kVringDescFlagsNext     = 1 << 0,
	kVringDescFlagsWrite    = 1 << 1,
	kVringDescFlagsIndirect = 1 << 2,
};

struct VirtioRegs {
	uint32 signature;
	uint32 version;
	uint32 deviceId;
	uint32 vendorId;
	uint32 deviceFeatures;
	uint32 unknown1[3];
	uint32 driverFeatures;
	uint32 unknown2[1];
	uint32 guestPageSize; /* version 1 only */
	uint32 unknown3[1];
	uint32 queueSel;
	uint32 queueNumMax;
	uint32 queueNum;
	uint32 queueAlign;    /* version 1 only */
	uint32 queuePfn;      /* version 1 only */
	uint32 queueReady;
	uint32 unknown4[2];
	uint32 queueNotify;
	uint32 unknown5[3];
	uint32 interruptStatus;
	uint32 interruptAck;
	uint32 unknown6[2];
	uint32 status;
	uint32 unknown7[3];
	uint32 queueDescLow;
	uint32 queueDescHi;
	uint32 unknown8[2];
	uint32 queueAvailLow;
	uint32 queueAvailHi;
	uint32 unknown9[2];
	uint32 queueUsedLow;
	uint32 queueUsedHi;
	uint32 unknown10[21];
	uint32 configGeneration;
	uint8 config[3840];
};

struct VirtioDesc {
	uint64 addr;
	uint32 len;
	uint16 flags;
	uint16 next;
};

// filled by driver
struct VirtioAvail {
	uint16 flags;
	uint16 idx;
	uint16 ring[0];
};

struct VirtioUsedItem
{
	uint32 id;
	uint32 len;
};

// filled by device
struct VirtioUsed {
	uint16 flags;
	uint16 idx;
	VirtioUsedItem ring[0];
};


// Input

// VirtioInputConfig::select
enum {
	kVirtioInputCfgUnset    = 0x00,
	kVirtioInputCfgIdName   = 0x01,
	kVirtioInputCfgIdSerial = 0x02,
	kVirtioInputCfgIdDevids = 0x03,
	kVirtioInputCfgPropBits = 0x10,
	kVirtioInputCfgEvBits   = 0x11, // subsel: kVirtioInputEv*
	kVirtioInputCfgAbsInfo  = 0x12, // subsel: kVirtioInputAbs*
};

enum {
	kVirtioInputEvSyn = 0,
	kVirtioInputEvKey = 1,
	kVirtioInputEvRel = 2,
	kVirtioInputEvAbs = 3,
	kVirtioInputEvRep = 4,
};

enum {
	kVirtioInputBtnLeft     = 0x110,
	kVirtioInputBtnRight    = 0x111,
	kVirtioInputBtnMiddle   = 0x112,
	kVirtioInputBtnGearDown = 0x150,
	kVirtioInputBtnGearUp   = 0x151,
};

enum {
	kVirtioInputRelX     = 0,
	kVirtioInputRelY     = 1,
	kVirtioInputRelZ     = 2,
	kVirtioInputRelWheel = 8,
};

enum {
	kVirtioInputAbsX = 0,
	kVirtioInputAbsY = 1,
	kVirtioInputAbsZ = 2,
};


struct VirtioInputAbsinfo {
	int32 min;
	int32 max;
	int32 fuzz;
	int32 flat;
	int32 res;
};

struct VirtioInputDevids {
	uint16 bustype;
	uint16 vendor;
	uint16 product;
	uint16 version;
};

struct VirtioInputConfig {
	uint8 select; // in
	uint8 subsel; // in
	uint8 size;   // out, size of reply
	uint8 reserved[5];
	union {
		char  string[128];
		uint8 bitmap[128];
		VirtioInputAbsinfo abs;
		VirtioInputDevids ids;
	};
};

struct VirtioInputPacket {
	uint16 type;
	uint16 code;
	int32 value;
};


// Block

enum {
	kVirtioBlockTypeIn       = 0,
	kVirtioBlockTypeOut      = 1,
	kVirtioBlockTypeFlush    = 4,
	kVirtioBlockTypeFlushOut = 5,
};

enum {
	kVirtioBlockStatusOk          = 0,
	kVirtioBlockStatusIoError     = 1,
	kVirtioBlockStatusUnsupported = 2,
};

enum {
	kVirtioBlockSectorSize = 512,
};

struct VirtioBlockRequest {
	uint32 type;
	uint32 ioprio;
	uint64 sectorNum;
};


#endif	// _VIRTIO_DEFS_H_
