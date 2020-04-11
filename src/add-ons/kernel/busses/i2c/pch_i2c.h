/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _PCH_I2C_H
#define _PCH_I2C_H


#include "pch_i2c_hardware.h"

extern "C" {
#	include "acpi.h"
}

#include <i2c.h>
#include <lock.h>


//#define TRACE_PCH_I2C
#ifdef TRACE_PCH_I2C
#	define TRACE(x...) dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#define ERROR(x...)			dprintf("\33[33mpch_i2c_pci:\33[0m " x)
#define CALLED(x...)		TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define PCH_I2C_ACPI_DEVICE_MODULE_NAME "busses/i2c/pch_i2c/acpi/driver_v1"
#define PCH_I2C_PCI_DEVICE_MODULE_NAME "busses/i2c/pch_i2c/pci/driver_v1"
#define PCH_I2C_SIM_MODULE_NAME "busses/i2c/pch_i2c/device/v1"


#define write32(address, data) \
	(*((volatile uint32*)(address)) = (data))
#define read32(address) \
	(*((volatile uint32*)(address)))



extern device_manager_info* gDeviceManager;
extern i2c_for_controller_interface* gI2c;
extern acpi_module_info* gACPI;
extern driver_module_info gPchI2cAcpiDevice;
extern driver_module_info gPchI2cPciDevice;


acpi_status pch_i2c_scan_bus_callback(acpi_handle object, uint32 nestingLevel,
	void *context, void** returnValue);


struct pch_i2c_crs {
	uint16	i2c_addr;
	uint8	irq;
    uint8	irq_triggering;
	uint8	irq_polarity;
	uint8	irq_sharable;

	uint32	addr_bas;
	uint32	addr_len;
};


typedef enum {
	PCH_I2C_IRQ_LEGACY,
	PCH_I2C_IRQ_MSI,
	PCH_I2C_IRQ_MSI_X_SHARED
} pch_i2c_irq_type;


typedef struct {
	phys_addr_t base_addr;
	size_t map_size;
	uint8 irq;
	i2c_bus sim;

	device_node* node;
	device_node* driver_node;

	area_id registersArea;
	addr_t registers;
	uint32 capabilities;

	uint16 ss_hcnt;
	uint16 ss_lcnt;
	uint16 fs_hcnt;
	uint16 fs_lcnt;
	uint16 hs_hcnt;
	uint16 hs_lcnt;
	uint32 sda_hold_time;

	uint8 tx_fifo_depth;
	uint8 rx_fifo_depth;

	uint32 masterConfig;

	// transfer
	int32	busy;
	bool	readwait;
	bool	writewait;
	i2c_op	op;
	void*	buffer;
	size_t	length;
	uint32	flags;
	int32	error;

	mutex	lock;
	status_t (*scan_bus)(i2c_bus_cookie cookie);
} pch_i2c_sim_info;


#endif // _PCH_I2C_H
