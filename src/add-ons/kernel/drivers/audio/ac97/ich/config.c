/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
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

//#define DEBUG 2

#include "debug.h"
#include "config.h"
#include "ich.h"

device_config c;
device_config *config = &c;

status_t find_pci_pin_irq(uint8 pin, uint8 *irq);

/* 
 * search for the ICH AC97 controller, and initialize the global config 
 * XXX multiple controllers not supported
 */

status_t probe_device(void)
{
	pci_module_info *pcimodule;
	config_manager_for_driver_module_info *configmodule;
	struct device_info info; 
	uint64 cookie; 
	status_t result;
	struct device_info *dinfo; 
	struct pci_info *pciinfo; 
	uint32 value;

	if (get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME,(module_info **)&configmodule) < 0) {
		dprintf(DRIVER_NAME " ERROR: couldn't load config manager module\n");
		return B_ERROR; 
	}

	if (get_module(B_PCI_MODULE_NAME,(module_info **)&pcimodule) < 0) {
		dprintf(DRIVER_NAME " ERROR: couldn't load pci module\n");
		put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
		return B_ERROR; 
	}

	config->name = 0;
	config->nambar = 0;
	config->nabmbar = 0;
	config->mmbar = 0;
	config->mbbar = 0;
	config->irq = 0;
	config->sample_size = 2;
	config->swap_reg = false;
	config->type = 0;
	config->log_mmbar = 0;
	config->log_mbbar = 0;
	config->area_mmbar = -1;
	config->area_mbbar = -1;

	result = B_ERROR;
	cookie = 0;
	while (configmodule->get_next_device_info(B_PCI_BUS, &cookie, &info, sizeof(info)) == B_OK) { 

		if (info.config_status != B_OK) 
			continue; 

		dinfo = (struct device_info *) malloc(info.size);
		if (dinfo == 0)
			break;
			
		if (configmodule->get_device_info_for(cookie, dinfo, info.size) != B_OK) {
			free(dinfo);
			break;
		}

		pciinfo = (struct pci_info *)((char *)dinfo + info.bus_dependent_info_offset); 
			
		if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x7195) {
			config->name = "Intel 82443MX"; 
		} else if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x2415) { /* verified */
			config->name = "Intel 82801AA (ICH)";
		} else if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x2425) { /* verified */
			config->name = "Intel 82801AB (ICH0)";
		} else if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x2445) { /* verified */
			config->name = "Intel 82801BA (ICH2), Intel 82801BAM (ICH2-M)";
		} else if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x2485) { /* verified */
			config->name = "Intel 82801CA (ICH3-S), Intel 82801CAM (ICH3-M)";
		} else if (pciinfo->vendor_id == 0x8086 && pciinfo->device_id == 0x24C5) { /* verified */
			config->name = "Intel 82801DB (ICH4)";
			config->type = TYPE_ICH4;
		} else if (pciinfo->vendor_id == 0x1039 && pciinfo->device_id == 0x7012) { /* verified */
			config->name = "SiS SI7012";
			config->swap_reg = true;
			config->sample_size = 1;
		} else if (pciinfo->vendor_id == 0x10DE && pciinfo->device_id == 0x01B1) {
			config->name = "Nvidia nForce AC97";
		} else if (pciinfo->vendor_id == 0x1022 && pciinfo->device_id == 0x764d) {
			config->name = "AMD AMD8111";
		} else if (pciinfo->vendor_id == 0x1022 && pciinfo->device_id == 0x7445) {
			config->name = "AMD AMD768";
		} else {
			free(dinfo);
			continue;
		}
		TRACE(("found %s\n",config->name));
		TRACE(("revision = %d\n",pciinfo->revision));

		#if DEBUG
			TRACE(("bus = %#x, device = %#x, function = %#x\n",pciinfo->bus, pciinfo->device, pciinfo->function));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x00, 2);
			TRACE(("VID = %#04x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x02, 2);
			TRACE(("DID = %#04x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x08, 1);
			TRACE(("RID = %#02x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x04, 2);
			TRACE(("PCICMD = %#04x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x06, 2);
			TRACE(("PCISTS = %#04x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x10, 4);
			TRACE(("NAMBAR = %#08x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x14, 4);
			TRACE(("NABMBAR = %#08x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x3c, 1);
			TRACE(("INTR_LN = %#02x\n",value));
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x3d, 1);
			TRACE(("INTR_PN = %#02x\n",value));
		#endif

		/*
		 * for ICH4 enable memory mapped IO and busmaster access,
		 * for old ICHs enable programmed IO and busmaster access
		 */
		value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_PCICMD, 2);
		if (config->type & TYPE_ICH4)
			value |= PCI_PCICMD_MSE | PCI_PCICMD_BME;
		else
			value |= PCI_PCICMD_IOS | PCI_PCICMD_BME;
		pcimodule->write_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x04, 2, value);
		
		#if DEBUG
			value = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, PCI_PCICMD, 2);
			TRACE(("PCICMD = %#04x\n",value));
		#endif
		
		config->irq = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x3C, 1);
		if (config->irq == 0 || config->irq == 0xff) {
			// workaround: even if no irq is configured, we may be able to find the correct one
			uint8 pin;
			uint8 irq;
			pin = pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x3d, 1);
			TRACE(("IRQ not assigned to pin %d\n",pin));
			TRACE(("Searching for IRQ...\n"));
			if (B_OK == find_pci_pin_irq(pin, &irq)) {
				TRACE(("Assigning IRQ %d to pin %d\n",irq,pin));
				config->irq = irq;
			} else {
				config->irq = 0; // always 0, not 0xff if no irq assigned
			}
		}
		
		if (config->type & TYPE_ICH4) {
			// memory mapped access
			config->mmbar = 0xfffffffe & pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x18, 4);
			config->mbbar = 0xfffffffe & pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x1C, 4);
		} else {
			// pio access
			config->nambar = 0xfffffffe & pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x10, 4);
			config->nabmbar = 0xfffffffe & pcimodule->read_pci_config(pciinfo->bus, pciinfo->device, pciinfo->function, 0x14, 4);
		}

		TRACE(("irq     = %d\n",config->irq));
		TRACE(("nambar  = %#08x\n",config->nambar));
		TRACE(("nabmbar = %#08x\n",config->nabmbar));
		TRACE(("mmbar   = %#08x\n",config->mmbar));
		TRACE(("mbbar   = %#08x\n",config->mbbar));

		free(dinfo);
		result = B_OK;
	}

	if (result != B_OK)
		TRACE(("probe_device() hardware not found\n"));
//config->irq = 0;
	if (config->irq == 0) {
		dprintf(DRIVER_NAME ": WARNING: no interrupt configured\n");
		/*
		 * we can continue without an interrupt, as another 
		 * workaround to handle this is also implemented
		 */
	}
	/* the ICH4 uses memory mapped IO */
	if ((config->type & TYPE_ICH4) != 0 && ((config->mmbar == 0) || (config->mbbar == 0))) {
		dprintf(DRIVER_NAME " ERROR: memory mapped IO not configured\n");
		result = B_ERROR;
	}
	/* all other ICHs use programmed IO */
	if ((config->type & TYPE_ICH4) == 0 && ((config->nambar == 0) || (config->nabmbar == 0))) {
		dprintf(DRIVER_NAME " ERROR: IO space not configured\n");
		result = B_ERROR;
	}

	put_module(B_PCI_MODULE_NAME);
	put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME);
	return result;
}


/*
 * This is another ugly workaround. If no irq has been assigned
 * to our card, we try to find another card that uses the same
 * interrupt pin, but has an irq assigned, and use it.
 */
status_t find_pci_pin_irq(uint8 pin, uint8 *irq)
{
	pci_module_info *module;
	struct pci_info info; 
	status_t result;
	long index;

	if (get_module(B_PCI_MODULE_NAME,(module_info **)&module) < 0) {
		dprintf(DRIVER_NAME " ERROR: couldn't load pci module\n");
		return B_ERROR; 
	}

	result = B_ERROR;
	for (index = 0; B_OK == module->get_nth_pci_info(index, &info); index++) {
		uint8 pciirq = module->read_pci_config(info.bus, info.device, info.function, PCI_interrupt_line, 1);
		uint8 pcipin = module->read_pci_config(info.bus, info.device, info.function, PCI_interrupt_pin, 1);
		TRACE(("pin %d, irq %d\n",pcipin,pciirq));
		if (pcipin == pin && pciirq != 0 && pciirq != 0xff) {
			*irq = pciirq;
			result = B_OK;
			break;
		}
	}

	#if DEBUG 
		if (result != B_OK) {
			TRACE(("Couldn't find IRQ for pin %d\n",pin));
		}
	#endif

	put_module(B_PCI_MODULE_NAME);
	return result;
}

