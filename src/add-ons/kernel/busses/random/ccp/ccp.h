/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _CCP_H
#define _CCP_H


#include <stdlib.h>

#include <KernelExport.h>


//#define TRACE_CCP_RNG
#ifndef DRIVER_NAME
#	define DRIVER_NAME "ccp_rng"
#endif
#ifdef TRACE_CCP_RNG
#	define TRACE(x...) dprintf("\33[33m" DRIVER_NAME ":\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33m" DRIVER_NAME ":\33[0m " x)
#define ERROR(x...)			dprintf("\33[33m" DRIVER_NAME ":\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define CCP_ACPI_DEVICE_MODULE_NAME "busses/random/ccp_rng/acpi/driver_v1"
#define CCP_PCI_DEVICE_MODULE_NAME "busses/random/ccp_rng/pci/driver_v1"
#define CCP_DEVICE_MODULE_NAME "busses/random/ccp_rng/device/v1"


#define read32(address) \
	(*((volatile uint32*)(address)))



extern device_manager_info* gDeviceManager;
extern driver_module_info gCcpAcpiDevice;
extern driver_module_info gCcpPciDevice;


typedef struct {
	phys_addr_t base_addr;
	uint64 map_size;

	device_node* node;
	device_node* driver_node;

	area_id registersArea;
	addr_t registers;

	timer extractTimer;
	void* dpcHandle;
} ccp_device_info;


#endif // _CCP_H
