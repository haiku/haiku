/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_H__
#define __PCI_H__

#include "pci_controller.h"


#define TRACE_PCI
#ifndef TRACE_PCI
#	define TRACE(x) 
#else
#	define TRACE(x) dprintf x
#endif


#ifdef __cplusplus

struct PCIDev;

struct PCIBus {
	PCIBus *next;
	PCIDev *parent;
	PCIDev *child;
	int domain;
	uint8 bus;
};

struct PCIDev {
	PCIDev *next;
	PCIBus *parent;
	PCIBus *child;
	int domain;
	uint8 bus;
	uint8 dev;
	uint8 func;
	pci_info info;
};


struct domain_data
{
	// These two are set in PCI::AddController:
	pci_controller *	controller;
	void *				controller_cookie;
	
	// All the rest is set in PCI::InitDomainData
	int					max_bus_devices;
};


class PCI {
	public:
							PCI();
							~PCI();
		
		void				InitDomainData();
		void				InitBus();

		status_t			AddController(pci_controller *controller, void *controller_cookie);

		status_t			GetNthPciInfo(long index, pci_info *outInfo);

		status_t			ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
										  uint8 offset, uint8 size, uint32 *value);

		uint32				ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
										  uint8 offset, uint8 size);

		status_t			WritePciConfig(int domain, uint8 bus, uint8 device, uint8 function,
										   uint8 offset, uint8 size, uint32 value);

		status_t			GetVirtBus(uint8 virt_bus, int *domain, uint8 *bus);
		
	private:

		void EnumerateBus(int domain, uint8 bus, uint8 *subordinate_bus = NULL);

		void DiscoverBus(PCIBus *bus);
		void DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func);

		PCIDev *CreateDevice(PCIBus *parent, uint8 dev, uint8 func);
		PCIBus *CreateBus(PCIDev *parent, int domain, uint8 bus);

		status_t GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo);
		void ReadPciBasicInfo(PCIDev *dev);
		void ReadPciHeaderInfo(PCIDev *dev);

		void RefreshDeviceInfo(PCIBus *bus);

		uint32 BarSize(uint32 bits, uint32 mask);
		void GetBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size = 0, uint8 *flags = 0);
		void GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size = 0, uint8 *flags = 0);
		
		
		domain_data *		GetDomainData(int domain);
		
		status_t			AddVirtBus(int domain, uint8 bus, uint8 *virt_bus);

	private:
		PCIBus *			fRootBus;

		enum { 				MAX_PCI_DOMAINS = 8 };
		
		domain_data			fDomainData[MAX_PCI_DOMAINS];
		int					fDomainCount;
};

#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

status_t	pci_init(void);
void		pci_uninit(void);

long		pci_get_nth_pci_info(long index, pci_info *outInfo);

uint32		pci_read_config(uint8 virt_bus, uint8 device, uint8 function, uint8 offset, uint8 size);
void		pci_write_config(uint8 virt_bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value);

#ifdef __cplusplus
}
#endif

#endif	/* __PCI_H__ */
