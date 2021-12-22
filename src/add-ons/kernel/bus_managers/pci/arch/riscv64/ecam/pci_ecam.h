/*
 * Copyright 2009-2020, Haiku Inc., All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "arch_pci_controller.h"


class PCIEcam : public ArchPCIController {

			status_t			Init(device_node* pciRootNode);

			status_t			InitMSI(int32 irq);
			int32				AllocateMSIIrq();
			void				FreeMSIIrq(int32 irq);
			int32				HandleMSIIrq(void* arg);

			addr_t				ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);
			void				InitDeviceMSI(uint8 bus, uint8 device, uint8 function);
			bool 				AllocateBar() { return true; }
};
