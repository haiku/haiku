/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"


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


class PCIFU740 : public ArchPCIController {

			status_t			Init(device_node* pciRootNode);

			status_t			InitMSI(int32 irq);
			int32				AllocateMSIIrq();
			void				FreeMSIIrq(int32 irq);
			int32				HandleMSIIrq(void* arg);


			addr_t				ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);
			void				InitDeviceMSI(uint8 bus, uint8 device, uint8 function);
			bool 				AllocateBar() { return false; }

private:
			status_t			AtuMap(uint32 index, uint32 direction, uint32 type,
									uint64 parentAdr, uint64 childAdr, uint32 size);
			void				AtuDump();
};
