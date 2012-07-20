/*
 * Copyright 2003-2008, Marcus Overhagen. All rights reserved.
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef __PCI_H__
#define __PCI_H__

#include <PCI.h>

#ifdef __cplusplus
  #include <VectorMap.h>
#endif

#include "pci_controller.h"

#if defined(__INTEL__) || defined(__x86_64__)
#include "pci_arch_info.h"
#endif

#define TRACE_PCI
#ifndef TRACE_PCI
#	define TRACE(x)
#else
#	define TRACE(x) dprintf x
#endif

#ifdef __cplusplus

struct PCIDev;

struct PCIBus {
	PCIBus *			next;
	PCIDev *			parent;
	PCIDev *			child;
	int					domain;
	uint8				bus;
};

struct PCIDev {
	PCIDev *			next;
	PCIBus *			parent;
	PCIBus *			child;
	int					domain;
	uint8				bus;
	uint8				device;
	uint8				function;
	pci_info			info;
#if defined(__INTEL__) || defined(__x86_64__)
	pci_arch_info		arch_info;
#endif
};


struct domain_data {
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

			void			InitDomainData();
			void			InitBus();

			status_t		AddController(pci_controller *controller,
								void *controller_cookie);

			status_t		GetNthInfo(long index, pci_info *outInfo);

			status_t		ReadConfig(int domain, uint8 bus, uint8 device,
								uint8 function, uint8 offset, uint8 size,
								uint32 *value);
			uint32			ReadConfig(int domain, uint8 bus, uint8 device,
								uint8 function, uint8 offset, uint8 size);
			uint32			ReadConfig(PCIDev *device, uint8 offset,
								uint8 size);

			status_t		WriteConfig(int domain, uint8 bus, uint8 device,
								uint8 function, uint8 offset, uint8 size,
								uint32 value);
			status_t		WriteConfig(PCIDev *device, uint8 offset,
								uint8 size, uint32 value);

			status_t		FindCapability(int domain, uint8 bus, uint8 device,
								uint8 function, uint8 capID, uint8 *offset);
			status_t		FindCapability(PCIDev *device, uint8 capID,
								uint8 *offset);

			status_t		ResolveVirtualBus(uint8 virtualBus, int *domain,
								uint8 *bus);

			PCIDev *		FindDevice(int domain, uint8 bus, uint8 device,
								uint8 function);

			void			ClearDeviceStatus(PCIBus *bus, bool dumpStatus);

			void			RefreshDeviceInfo();

			status_t		UpdateInterruptLine(int domain, uint8 bus,
								uint8 device, uint8 function,
								uint8 newInterruptLineValue);

private:
			void			_EnumerateBus(int domain, uint8 bus,
								uint8 *subordinateBus = NULL);

			void			_FixupDevices(int domain, uint8 bus);

			void			_DiscoverBus(PCIBus *bus);
			void			_DiscoverDevice(PCIBus *bus, uint8 dev,
								uint8 function);

			PCIDev *		_CreateDevice(PCIBus *parent, uint8 dev,
								uint8 function);
			PCIBus *		_CreateBus(PCIDev *parent, int domain, uint8 bus);

			status_t		_GetNthInfo(PCIBus *bus, long *currentIndex,
								long wantIndex, pci_info *outInfo);
			void			_ReadBasicInfo(PCIDev *dev);
			void			_ReadHeaderInfo(PCIDev *dev);

			void			_ConfigureBridges(PCIBus *bus);
			void			_RefreshDeviceInfo(PCIBus *bus);

			uint32			_BarSize(uint32 bits, uint32 mask);
			void			_GetBarInfo(PCIDev *dev, uint8 offset,
								uint32 *address, uint32 *size = 0,
								uint8 *flags = 0);
			void			_GetRomBarInfo(PCIDev *dev, uint8 offset,
								uint32 *address, uint32 *size = 0,
								uint8 *flags = 0);

			domain_data *	_GetDomainData(int domain);

			status_t		_CreateVirtualBus(int domain, uint8 bus,
								uint8 *virtualBus);

			int				_NumFunctions(int domain, uint8 bus, uint8 device);
			PCIDev *		_FindDevice(PCIBus *current, int domain, uint8 bus,
								uint8 device, uint8 function);

private:
	PCIBus *				fRootBus;

	enum { MAX_PCI_DOMAINS = 8 };

	domain_data				fDomainData[MAX_PCI_DOMAINS];
	int						fDomainCount;
	bool					fBusEnumeration;

	typedef VectorMap<uint8, uint16> VirtualBusMap;

	VirtualBusMap			fVirtualBusMap;
	int						fNextVirtualBus;
};

extern PCI *gPCI;

#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif

status_t	pci_init(void);
void		pci_uninit(void);

long		pci_get_nth_pci_info(long index, pci_info *outInfo);

uint32		pci_read_config(uint8 virtualBus, uint8 device, uint8 function,
				uint8 offset, uint8 size);
void		pci_write_config(uint8 virtualBus, uint8 device, uint8 function,
				uint8 offset, uint8 size, uint32 value);

void		__pci_resolve_virtual_bus(uint8 virtualBus, int *domain, uint8 *bus);

#ifdef __cplusplus
}
#endif

#endif	/* __PCI_H__ */
