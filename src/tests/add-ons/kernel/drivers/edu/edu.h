/*
 * Copyright 2004-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamza Dridi, <dridiha@gmail.com>
 */

#ifndef EDU_H
#define EDU_H

#include <PCI.h>
#include <device_manager.h>

#include "IOScheduler.h"
#include "bus/PCI.h"
#include "dma_resources.h"

#define PCI_EDU_VENDOR_ID 0x1234
#define PCI_EDU_DEVICE_ID 0x11e8

#define EDU_LOW_ADDRESS 0x00
#define EDU_HIGH_ADDRESS 0x0fffffff

#define EDU_DMA_ADDR 0x40000
#define EDU_DMA_BUFFER_SIZE 4096
#define EDU_DISABLE_INTERRUPT 0x400
#define EDU_DMA_START 0x1
#define EDU_DMA_RAM_TO_EDU 0x0
#define EDU_DMA_EDU_TO_RAM 0x2
#define EDU_DMA_INTERRUPT 0x4
#define EDU_FACTORIAL_REG 0x08
#define EDU_STATUS_REG 0x20
#define EDU_INTERRUPT_STATUS_REG 0x24
#define EDU_INTERRUPT_ACK_REG 0x64
#define EDU_DMA_SRC_REG 0x80
#define EDU_DMA_DST_REG 0x88
#define EDU_DMA_COUNT_REG 0x90
#define EDU_DMA_CMD_REG 0x98

#define EDU_FACTORIAL_INTERRUPT 0x80

#define EDU_IOCTL_IDENT 0x00
#define EDU_IOCTL_LIVENESS 0x01
#define EDU_IOCTL_FACTORIAL 0x02

typedef struct edu_driver_data {
        pci_info* pci_device_info;
        device_node* node;
        pci_device_module_info* pci;
        pci_device* pci_dev;
} edu_driver_data;

class EduDeviceData {
public:
                                EduDeviceData(pci_device_module_info* pci, pci_info* pci_info,
                                    pci_device* pci_dev);

                                ~EduDeviceData();

            status_t            Init();
            void                EduWriteRegister(uint8 offset, uint32 value);
            uint32              EduReadRegister(uint8 offset);
            uint32              EduComputeFactorial(uint32 value);
			void				EduDoDma(phys_addr_t physaddr, uint32 length, off_t offset,
                                    bool isWrite);

            IOScheduler*        Scheduler() const { return fScheduler; };
            sem_id              Notify() const { return fNotify; };

private:

            area_id             fRegisterArea;
            uint8*              fRegisters;
            sem_id              fNotify;
            DMAResource*        fDMAResource;
            IOScheduler*        fScheduler;
            uchar               fIRQ;

            pci_device_module_info* fPci;
            struct pci_info*    fPciInfo;
            pci_device*         fPciDev;
};

#endif // !EDU_H
