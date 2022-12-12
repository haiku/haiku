/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"
#include <arch/generic/msi.h>


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


class PCIFU740 : public ArchPCIController, public MSIInterface {

			status_t			Init(device_node* pciRootNode);

			PciDbiRegs volatile* GetDbuRegs() {return (PciDbiRegs volatile*)fDbiBase;}

			status_t			InitMSI(int32 irq);
			 status_t			AllocateVectors(uint8 count, uint8& startVector, uint64& address,
									uint16& data) final;
			 void				FreeVectors(uint8 count, uint8 startVector) final;
	static	int32				HandleMSIIrq(void* arg);
	inline	int32				HandleMSIIrqInt();

			void				EnableIoInterrupt(int irq) final;
			void				DisableIoInterrupt(int irq) final;
			void				ConfigureIoInterrupt(int irq, uint32 config) final;
			int32				AssignToCpu(int32 irq, int32 cpu) final;

			addr_t				ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);
			bool 				AllocateBar() { return false; }

private:
			status_t			AtuMap(uint32 index, uint32 direction, uint32 type,
									uint64 parentAdr, uint64 childAdr, uint32 size);
			void				AtuDump();

private:
			AreaDeleter			fDbiArea;
			addr_t				fDbiPhysBase{};
			addr_t				fDbiBase{};
			size_t				fDbiSize{};
};

