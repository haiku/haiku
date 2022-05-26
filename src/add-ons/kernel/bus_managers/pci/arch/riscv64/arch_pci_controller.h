/*
 * Copyright 2021, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	x512 <danger_mail@list.ru>
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _ARCH_PCI_CONTROLLER_H_
#define _ARCH_PCI_CONTROLLER_H_


#include "pci_controller_private.h"
#include "pci_controller.h"
#include "pci_private.h"
#include "pci.h"

#include <AutoDeleterOS.h>
#include <AutoDeleterDrivers.h>
#include <drivers/bus/FDT.h>
#include <util/AutoLock.h>


enum {
    kRegIo,
    kRegMmio32,
    kRegMmio64,
};

struct RegisterRange {
    phys_addr_t parentBase;
    phys_addr_t childBase;
    size_t size;
    phys_addr_t free;
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



class ArchPCIController {
public:
								ArchPCIController();
virtual							~ArchPCIController();

// Implementation Specific
virtual	status_t				Init(device_node* pciRootNode) = 0;

virtual status_t				InitMSI(int32 irq) = 0;
virtual int32					AllocateMSIIrq() = 0;
virtual void					FreeMSIIrq(int32 irq) = 0;
virtual	void					InitDeviceMSI(uint8 bus, uint8 device, uint8 function) = 0;
virtual int32					HandleMSIIrq(void* arg) = 0;

virtual	addr_t					ConfigAddress(uint8 bus, uint8 device, uint8 function,
									uint16 offset) = 0;
virtual bool					AllocateBar() = 0;

// Generic Helpers
		uint32_t				EncodePciAddress(uint8 bus, uint8 device, uint8 function);
		void					DecodePciAddress(uint32_t adr, uint8& bus, uint8& device,
									uint8& function);

		addr_t					GetIoRegs() { return fIoBase; };
	volatile PciDbiRegs*				GetDbuRegs();

		RegisterRange*			GetRegisterRange(int range);
		void					SetRegisterRange(int kind, phys_addr_t parentBase,
									phys_addr_t childBase, size_t size);

		phys_addr_t				AllocRegister(int kind, size_t size);
		void					AllocRegsForDevice(uint8 bus, uint8 device, uint8 function);

		InterruptMap*			LookupInterruptMap(uint32_t childAdr, uint32_t childIrq);
		void					AllocRegs();

		status_t				ReadConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
									uint16 offset, uint8 size, uint32 *value);
		status_t				WriteConfig(void *cookie, uint8 bus, uint8 device, uint8 function,
									uint16 offset, uint8 size, uint32 value);

protected:
			uint32				fAllocatedMsiIrqs[1];
			phys_addr_t			fMsiPhysAddr;
			long				fMsiStartIrq;
			uint64				fMsiData;

			RegisterRange		fRegisterRanges[3];
			InterruptMapMask	fInterruptMapMask;

			uint32				fHostCtrlType;

			AreaDeleter			fConfigArea;
			addr_t				fConfigPhysBase;
			addr_t				fConfigBase;
			size_t				fConfigSize;

			AreaDeleter			fDbiArea;
			addr_t				fDbiPhysBase;
			addr_t				fDbiBase;
			size_t				fDbiSize;

			AreaDeleter			fIoArea;
			addr_t				fIoBase;

			ArrayDeleter<InterruptMap> fInterruptMap;
			uint32_t				fInterruptMapLen;
};


#endif /* _ARCH_PCI_CONTROLLER_H_ */
