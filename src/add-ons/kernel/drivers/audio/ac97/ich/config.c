/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002-2005 Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <KernelExport.h>
#include <config_manager.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>

#include "debug.h"
#include "config.h"
#include "ich.h"

extern pci_module_info *pci;

device_config c;
device_config *config = &c;

struct device_item {
	uint16			vendor_id;
	uint16			device_id;
	uint8			type;
	const char *	name;
} device_list[] = {
	{ 0x8086, 0x7195, TYPE_DEFAULT, "Intel 82443MX" },
	{ 0x8086, 0x2415, TYPE_DEFAULT, "Intel 82801AA (ICH)" },
	{ 0x8086, 0x2425, TYPE_DEFAULT, "Intel 82801AB (ICH0)" },
	{ 0x8086, 0x2445, TYPE_DEFAULT, "Intel 82801BA (ICH2), Intel 82801BAM (ICH2-M)" },
	{ 0x8086, 0x2485, TYPE_DEFAULT, "Intel 82801CA (ICH3-S), Intel 82801CAM (ICH3-M)" },
	{ 0x8086, 0x24C5, TYPE_ICH4,	"Intel 82801DB (ICH4)" },
	{ 0x8086, 0x24D5, TYPE_ICH4,	"Intel 82801EB (ICH5), Intel 82801ER (ICH5R)" },
	{ 0x8086, 0x266E, TYPE_ICH4,	"Intel 82801FB/FR/FW/FRW (ICH6)" },
	{ 0x8086, 0x27DE, TYPE_ICH4,	"Intel unknown (ICH7)" },
	{ 0x8086, 0x2698, TYPE_ICH4,	"Intel unknown (ESB2)" },
	{ 0x8086, 0x25A6, TYPE_ICH4,	"Intel unknown (ESB5)" },
	{ 0x1039, 0x7012, TYPE_SIS7012,	"SiS SI7012" },
	{ 0x10DE, 0x01B1, TYPE_DEFAULT,	"NVIDIA nForce (MCP)" },
	{ 0x10DE, 0x006A, TYPE_DEFAULT, "NVIDIA nForce 2 (MCP2)" },
	{ 0x10DE, 0x00DA, TYPE_DEFAULT,	"NVIDIA nForce 3 (MCP3)" },
	{ 0x10DE, 0x003A, TYPE_DEFAULT,	"NVIDIA unknown (MCP04)" },
	{ 0x10DE, 0x0059, TYPE_DEFAULT,	"NVIDIA unknown (CK804)" },
	{ 0x10DE, 0x008A, TYPE_DEFAULT,	"NVIDIA unknown (CK8)" },
	{ 0x10DE, 0x00EA, TYPE_DEFAULT,	"NVIDIA unknown (CK8S)" },
	{ 0x1022, 0x746D, TYPE_DEFAULT, "AMD AMD8111" },
	{ 0x1022, 0x7445, TYPE_DEFAULT, "AMD AMD768" },
//	{ 0x10B9, 0x5455, TYPE_DEFAULT, "Ali 5455" }, not yet supported
	{ },
};


/** 
 * search for the ICH AC97 controller, and initialize the global config 
 */
status_t
probe_device(void)
{
	struct pci_info info; 
	struct pci_info *pciinfo = &info;
	int index;
	int i;
	status_t result;
	uint32 value;

	// initialize whole config to 0
	memset(config, 0, sizeof(*config));

	result = B_OK;

	for (index = 0; B_OK == pci->get_nth_pci_info(index, pciinfo); index++) { 
		LOG(("Checking PCI device, vendor 0x%04x, id 0x%04x, bus 0x%02x, dev 0x%02x, func 0x%02x, rev 0x%02x, api 0x%02x, sub 0x%02x, base 0x%02x\n",
			pciinfo->vendor_id, pciinfo->device_id, pciinfo->bus, pciinfo->device, pciinfo->function,
			pciinfo->revision, pciinfo->class_api, pciinfo->class_sub, pciinfo->class_base));
		for (i = 0; device_list[i].vendor_id; i++) {
			if (device_list[i].vendor_id == pciinfo->vendor_id && device_list[i].device_id == pciinfo->device_id) {
				config->name = device_list[i].name;
				config->type = device_list[i].type;
				goto probe_ok;
			}
		}
	}
	LOG(("No compatible hardware found\n"));
	result = B_ERROR;
	goto probe_done;

probe_ok:
	LOG(("found %s\n",config->name));
	LOG(("revision = %d\n",pciinfo->revision));

	#if DEBUG
		LOG(("bus = %d, device = %d, function = %d\n", pciinfo->bus, pciinfo->device, pciinfo->function));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_vendor_id, 2);
		LOG(("vendor id = 0x%04x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_device_id, 2);
		LOG(("device id = 0x%04x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_revision, 1);
		LOG(("revision = 0x%02x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_subsystem_vendor_id, 2);
		LOG(("subsystem vendor = 0x%04x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_subsystem_id, 2);
		LOG(("subsystem device = 0x%04x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x04, 2);
		LOG(("PCICMD = %#04x\n",value));
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x06, 2);
		LOG(("PCISTS = %#04x\n",value));
	#endif

	/* for ICH4 enable memory mapped IO and busmaster access,
	 * for old ICHs enable programmed IO and busmaster access
	 */
	value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_PCICMD, 2);
	if (config->type & TYPE_ICH4)
		value |= PCI_PCICMD_MSE | PCI_PCICMD_BME;
	else
		value |= PCI_PCICMD_IOS | PCI_PCICMD_BME;
	pci->write_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_PCICMD, 2, value);
		
	#if DEBUG
		value = pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_PCICMD, 2);
		LOG(("PCICMD = %#04x\n",value));
	#endif

	/* read memory-io and port-io bars
	 */
	config->nambar	= pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x10, 4);
	config->nabmbar	= pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x14, 4);
	config->mmbar	= pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x18, 4);
	config->mbbar	= pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x1C, 4);
	config->irq		= pci->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x3C, 1);

	config->nambar  &= PCI_address_io_mask;
	config->nabmbar &= PCI_address_io_mask;
	config->mmbar   &= PCI_address_memory_32_mask;
	config->mbbar   &= PCI_address_memory_32_mask;       

	if (config->irq == 0 || config->irq == 0xff) {
		PRINT(("WARNING: no interrupt configured\n"));
		/* we can continue without an interrupt, as another 
		 * workaround to handle this is also implemented
		 * force irq to be 0, not 0xff if no irq assigned
		 */
		config->irq = 0;
	}

	/* the ICH4 uses memory mapped IO */
	if ((config->type & TYPE_ICH4) && ((config->mmbar == 0) || (config->mbbar == 0))) {
		PRINT(("ERROR: memory mapped IO not configured\n"));
		result = B_ERROR;
	}
	/* all other ICHs use programmed IO */
	if (!(config->type & TYPE_ICH4) && ((config->nambar == 0) || (config->nabmbar == 0))) {
		PRINT(("ERROR: IO space not configured\n"));
		result = B_ERROR;
	}

	LOG(("nambar  = 0x%08x\n", config->nambar));
	LOG(("nabmbar = 0x%08x\n", config->nabmbar));
	LOG(("mmbar   = 0x%08x\n", config->mmbar));
	LOG(("mbbar   = 0x%08x\n", config->mbbar));
	LOG(("irq     = %d\n",    config->irq));

probe_done:
	return result;
}
