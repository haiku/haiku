/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <KernelExport.h>
#include <stdlib.h>
#include <string.h>
#include "dvb_interface.h"
#include "cx23882.h"
#include "cx23882_i2c.h"
#include "cx22702.h"
#include "dtt7592.h"
#include "driver.h"
#include "config.h"
#include "util.h"

#define TRACE_INTERFACE
#ifdef TRACE_INTERFACE
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif


#if 0
static void
dump_eeprom(cx23882_device *device)
{
	uint8 d[256+8];
	uint8 adr;
	uint8 *p;
	int i;
	status_t res;

	adr = 0;
	res = i2c_xfer(device->i2c_bus, I2C_ADDR_EEPROM, &adr, 1, d, sizeof(d));
	if (res != B_OK) {
		TRACE("i2c_read failed: %08lx\n", res);
		return;		
	}
	for (p = d, i = 0; i < ((int)sizeof(d) / 8); i++, p+= 8)
		TRACE("EEPROM %02x: %02x %02x %02x %02x %02x %02x %02x %02x\n", i * 8, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		
}
#endif


status_t
interface_attach(void **cookie, const pci_info *info)
{
	cx23882_device *device;
	uint32 val;
	int i;
	
	TRACE("interface_attach\n");
	
	device = malloc(sizeof(cx23882_device));
	if (!device)
		return B_NO_MEMORY;
	*cookie = device;
	
	// initialize cookie
	memset(device, 0, sizeof(*device));
	device->regs_area = -1;
	device->dma_buf1_area = -1;
	device->dma_buf2_area = -1;
	device->capture_sem = -1;
	
	device->pci_info = info;

	// enable busmaster and memory mapped access, disable io port access
	val = gPci->read_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, PCI_command, 2);
	val = PCI_PCICMD_BME | PCI_PCICMD_MSE | (val & ~PCI_PCICMD_IOS);
	gPci->write_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, PCI_command, 2, val);

	// adjust PCI latency timer
	val = gPci->read_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, PCI_latency, 1);
	TRACE("PCI latency is %02lx, changing to %02x\n", val, PCI_LATENCY);
	gPci->write_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, PCI_latency, 1, PCI_LATENCY);

	// get IRQ
	device->irq = gPci->read_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, PCI_interrupt_line, 1);
	if (device->irq == 0 || device->irq == 0xff) {
		dprintf("cx23882: Error, no IRQ assigned\n");
		goto err;
	}
	TRACE("IRQ %d\n", device->irq);
	
	// map registers into memory
	val = gPci->read_pci_config(device->pci_info->bus, device->pci_info->device, device->pci_info->function, 0x10, 4);
	val &= PCI_address_memory_32_mask;
	if (val == 0) {
		dprintf("cx23882: Error, no memory space assigned\n");
		goto err;
	}
	TRACE("hardware register address %p\n", (void *) val);
	device->regs_area = map_mem(&device->regs, (void *)val, 16777216 /* 16 MB */, 0, "cx23882 registers");
	if (device->regs_area < B_OK) {
		dprintf("cx23882: Error, can't map hardware registers\n");
		goto err;
	}
	TRACE("mapped registers to %p\n", device->regs);

	device->capture_sem = create_sem(0, "cx23882 capture");

	cx23882_reset(device);
		
	if (i2c_init(device) < B_OK) {
		dprintf("cx23882: Error, can't init I2C\n");
	}
	
		
	if (cx23882_init(device) < B_OK) {
		dprintf("cx23882: Error, can't init hardware\n");
	}
	
	
	for (i = 0; i < 20; i++)
		if (cx22702_init(device->i2c_bus) == B_OK)
			break;
	if (i == 20) {
		TRACE("cx22702_init failed\n");
		goto err;
	}

	// setup interrupt handler
	if (install_io_interrupt_handler(device->irq, cx23882_int, device, 0) < B_OK) {
		dprintf("cx23882: Error, can't install interrupt handler\n");
		goto err;
	}
	
//	dump_eeprom(device);
//	dtt7582_test(device->i2c_bus);
	
	return B_OK;
err:
	free(cookie);
	return B_ERROR;
}


void
interface_detach(void *cookie)
{
	cx23882_device *device = cookie;

	i2c_terminate(device);

	if (cx23882_terminate(device) < B_OK) {
	}
	
  	remove_io_interrupt_handler(device->irq, cx23882_int, device);

	delete_area(device->regs_area);
	
	delete_sem(device->capture_sem);
	
	TRACE("interface_detach\n");
}


static void
interface_get_interface_info(dvb_interface_info_t *info)
{
	memset(info, 0, sizeof(*info));
	info->version = 1;
	info->flags = 0;
	info->type = DVB_TYPE_DVB_T;
	strcpy(info->name, "CX23882");
	strcpy(info->info, "Hauppauge WinTV-NOVA-T model 928 driver, Copyright (c) 2005 Marcus Overhagen");
}


status_t
interface_ioctl(void *cookie, uint32 op, void *arg, size_t len)
{
	cx23882_device *device = cookie;
	status_t res;
	
	switch (op) {
		case DVB_GET_INTERFACE_INFO:
		{
			dvb_interface_info_t info;
			interface_get_interface_info(&info);
			if (user_memcpy(arg, &info, sizeof(info)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case DVB_GET_FREQUENCY_INFO:
		{
			dvb_frequency_info_t info;
			if ((res = cx22702_get_frequency_info(device->i2c_bus, &info)) < B_OK)
				return res;
			if (user_memcpy(arg, &info, sizeof(info)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}
		
		case DVB_START_CAPTURE:
		{
			return cx23882_start_capture(device);
		}
		
		case DVB_STOP_CAPTURE:
		{
			return cx23882_stop_capture(device);
		}
		
		case DVB_SET_TUNING_PARAMETERS:
		{
			dvb_tuning_parameters_t params;
			if (user_memcpy(&params, arg, sizeof(params)) < B_OK)
				return B_BAD_ADDRESS;
			if ((res = cx22702_set_tuning_parameters(device->i2c_bus, &params.u.dvb_t)) < B_OK)
				return res;
			break;
		}

		case DVB_GET_TUNING_PARAMETERS:
		{
			dvb_tuning_parameters_t params;
			if ((res = cx22702_get_tuning_parameters(device->i2c_bus, &params.u.dvb_t)) < B_OK)
				return res;
			if (user_memcpy(arg, &params, sizeof(params)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}
		
		case DVB_GET_STATUS:
		{
			dvb_status_t status;
			if ((res = cx22702_get_status(device->i2c_bus, &status)) < B_OK)
				return res;
			if (user_memcpy(arg, &status, sizeof(status)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case DVB_GET_SS:
		{
			uint32 value;
			if ((res = cx22702_get_ss(device->i2c_bus, &value)) < B_OK)
				return res;
			if (user_memcpy(arg, &value, sizeof(value)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case DVB_GET_BER:
		{
			uint32 value;
			if ((res = cx22702_get_ber(device->i2c_bus, &value)) < B_OK)
				return res;
			if (user_memcpy(arg, &value, sizeof(value)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case DVB_GET_SNR:
		{
			uint32 value;
			if ((res = cx22702_get_snr(device->i2c_bus, &value)) < B_OK)
				return res;
			if (user_memcpy(arg, &value, sizeof(value)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}

		case DVB_GET_UPC:
		{
			uint32 value;
			if ((res = cx22702_get_upc(device->i2c_bus, &value)) < B_OK)
				return res;
			if (user_memcpy(arg, &value, sizeof(value)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}
		
		case DVB_CAPTURE:
		{
			dvb_capture_t cap_data;
			// wait for data ready interrupt, with 100 ms timeout (in case tuning failed, bad reception, etc)
			if ((res = acquire_sem_etc(device->capture_sem, 1, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 100000)) < B_OK)
				return res;
			cap_data.data = device->capture_data;
			cap_data.size = device->capture_size;
			cap_data.end_time = device->capture_end_time;
			if (user_memcpy(arg, &cap_data, sizeof(cap_data)) < B_OK)
				return B_BAD_ADDRESS;
			break;
		}
		
		default:
		{
			TRACE("interface_ioctl\n");
			return B_BAD_VALUE;
		}
	}

	return B_OK;
}
