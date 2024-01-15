/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _PCICONTROLLERDW_H_
#define _PCICONTROLLERDW_H_

#include <bus/PCI.h>
#include <arch/generic/msi.h>

#include <AutoDeleterOS.h>
#include <lock.h>
#include <util/Vector.h>


#define CHECK_RET(err) {status_t _err = (err); if (_err < B_OK) return _err;}

#define DESIGNWARE_PCI_DRIVER_MODULE_NAME "busses/pci/designware/driver_v1"


enum {
	fdtPciRangeConfig      = 0x00000000,
	fdtPciRangeIoPort      = 0x01000000,
	fdtPciRangeMmio32Bit   = 0x02000000,
	fdtPciRangeMmio64Bit   = 0x03000000,
	fdtPciRangeTypeMask    = 0x03000000,
	fdtPciRangeAliased     = 0x20000000,
	fdtPciRangePrefechable = 0x40000000,
	fdtPciRangeRelocatable = 0x80000000,
};


enum PciBarKind {
	kRegIo,
	kRegMmio32,
	kRegMmio64,
	kRegMmio1MB,
	kRegUnknown,
};


union PciAddress {
	struct {
		uint32 offset: 8;
		uint32 function: 3;
		uint32 device: 5;
		uint32 bus: 8;
		uint32 unused: 8;
	};
	uint32 val;
};

union PciAddressEcam {
	struct {
		uint32 offset: 12;
		uint32 function: 3;
		uint32 device: 5;
		uint32 bus: 8;
		uint32 unused: 4;
	};
	uint32 val;
};

struct RegisterRange {
	phys_addr_t parentBase;
	phys_addr_t childBase;
	uint64 size;
};

struct InterruptMapMask {
	uint32_t childAdr;
	uint32_t childIrq;
};

struct InterruptMap {
	uint32_t childAdr;
	uint32_t childIrq;
	uint32_t parentIrqCtrl;
	uint32_t parentIrq;
};


enum {
	kPciAtuOffset = 0x300000,
};

enum {
	kPciAtuOutbound  = 0,
	kPciAtuInbound   = 1,
};

enum {
	// ctrl1
	kPciAtuTypeMem  = 0,
	kPciAtuTypeIo   = 2,
	kPciAtuTypeCfg0 = 4,
	kPciAtuTypeCfg1 = 5,
	// ctrl2
	kPciAtuBarModeEnable = 1 << 30,
	kPciAtuEnable        = 1 << 31,
};

struct PciAtuRegs {
	uint32 ctrl1;
	uint32 ctrl2;
	uint32 baseLo;
	uint32 baseHi;
	uint32 limit;
	uint32 targetLo;
	uint32 targetHi;
	uint32 unused[57];
};

struct PciDbiRegs {
	uint8 unknown0[0x700];

	uint32 unknown1[3];
	uint32 portAfr;
	uint32 linkControl;
	uint32 unknown2[5];
	uint32 portDebug0;
	uint32 portDebug1;
	uint32 unknown3[55];
	uint32 linkWidthSpeedControl;
	uint32 unknown4[4];
	uint32 msiAddrLo;
	uint32 msiAddrHi;
	struct {
		uint32 enable;
		uint32 mask;
		uint32 status;
	} msiIntr[8];
	uint32 unknown5[13];
	uint32 miscControl1Off;
	uint32 miscPortMultiLaneCtrl;
	uint32 unknown6[15];

	uint32 atuViewport;
	uint32 atuCr1;
	uint32 atuCr2;
	uint32 atuBaseLo;
	uint32 atuBaseHi;
	uint32 atuLimit;
	uint32 atuTargetLo;
	uint32 atuTargetHi;
	uint32 unknown7;
	uint32 atuLimitHi;
	uint32 unknown8[8];

	uint32 msixDoorbell;
	uint32 unknown9[117];

	uint32 plChkRegControlStatus;
	uint32 unknown10;
	uint32 plChkRegErrAddr;
	uint32 unknown11[309];
};


class MsiInterruptCtrlDW: public MSIInterface {
public:
			virtual				~MsiInterruptCtrlDW() = default;

			status_t			Init(PciDbiRegs volatile* dbiRegs, int32 msiIrq);

			status_t			AllocateVectors(uint8 count, uint8& startVector, uint64& address,
									uint16& data) final;
			void				FreeVectors(uint8 count, uint8 startVector) final;


private:
	static	int32				InterruptReceived(void* arg);
	inline	int32				InterruptReceivedInt();

private:
			PciDbiRegs volatile* fDbiRegs {};

			uint32				fAllocatedMsiIrqs[1];
			phys_addr_t			fMsiPhysAddr {};
			long				fMsiStartIrq {};
			uint64				fMsiData {};
};


class DWPCIController {
public:
	static float SupportsDevice(device_node* parent);
	static status_t RegisterDevice(device_node* parent);
	static status_t InitDriver(device_node* node, DWPCIController*& outDriver);
	void UninitDriver();

	status_t ReadConfig(
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 &value);

	status_t WriteConfig(
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 value);

	status_t GetMaxBusDevices(int32& count);

	status_t ReadIrq(
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8& irq);

	status_t WriteIrq(
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8 irq);

	status_t GetRange(uint32 index, pci_resource_range* range);

private:
	status_t ReadResourceInfo();
	inline status_t InitDriverInt(device_node* node);

	inline addr_t ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);

	PciDbiRegs volatile* GetDbuRegs() {return (PciDbiRegs volatile*)fDbiBase;}
	status_t AtuMap(uint32 index, uint32 direction, uint32 type,
		uint64 parentAdr, uint64 childAdr, uint32 size);
	void AtuDump();

private:
	spinlock fLock = B_SPINLOCK_INITIALIZER;

	device_node* fNode {};

	AreaDeleter fConfigArea;
	addr_t fConfigPhysBase {};
	addr_t fConfigBase {};
	size_t fConfigSize {};

	Vector<pci_resource_range> fResourceRanges;
	InterruptMapMask fInterruptMapMask {};
	uint32 fInterruptMapLen {};
	ArrayDeleter<InterruptMap> fInterruptMap;

	AreaDeleter fDbiArea;
	addr_t fDbiPhysBase {};
	addr_t fDbiBase {};
	size_t fDbiSize {};

	MsiInterruptCtrlDW fIrqCtrl;
};


extern device_manager_info* gDeviceManager;

#endif	// _PCICONTROLLERDW_H_
