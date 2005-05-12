/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_H__
#define __PCI_H__


#include <SupportDefs.h>


#define TRACE_PCI
#ifndef TRACE_PCI
#	define TRACE(x) 
#else
#	define TRACE(x) dprintf x
#endif


#ifdef __cplusplus

struct PCIBus;
struct PCIDev;

struct PCIBus {
	PCIDev *parent;
	uint8 bus;
	PCIDev *child;
};

struct PCIDev {
	PCIDev *next;
	PCIBus *parent;
	PCIBus *child;
	uint8 bus;
	uint8 dev;
	uint8 func;
	pci_info info;
};

class PCI {
	public:
		PCI();
		~PCI();

		status_t GetNthPciInfo(long index, pci_info *outInfo);

	private:	
		void DiscoverBus(PCIBus *bus);
		void DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func);

		PCIDev *CreateDevice(PCIBus *parent, uint8 dev, uint8 func);
		PCIBus *CreateBus(PCIDev *parent, uint8 bus);

		status_t GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo);
		void ReadPciBasicInfo(PCIDev *dev);
		void ReadPciHeaderInfo(PCIDev *dev);

		void RefreshDeviceInfo(PCIBus *bus);

		uint32 BarSize(uint32 bits, uint32 mask);
		void GetBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size = 0, uint8 *flags = 0);
		void GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size = 0, uint8 *flags = 0);

	private:
		PCIBus fRootBus;
};

#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

status_t pci_init(void);
void pci_uninit(void);

long pci_get_nth_pci_info(long index, pci_info *outInfo);

#ifdef __cplusplus
}
#endif

#endif	/* __PCI_H__ */
