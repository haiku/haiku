/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#define TRACE(a...) dprintf("ahci: " a)
#define FLOW(a...)	dprintf("ahci: " a)

#if 0
	uint16 vendor_id;
	uint16 device_id;
	uint8 base_class;
	uint8 sub_class;
	uint8 class_api;
	status_t res;

	if (base_class != PCI_mass_storage || sub_class != PCI_sata || class_api != PCI_sata_ahci)
		return 0.0f;

	TRACE("controller found! vendor 0x%04x, device 0x%04x\n", vendor_id, device_id);
	
	res = pci->find_pci_capability(device, PCI_cap_id_sata, &cap_ofs);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", cap_ofs);
		satacr0 = pci->read_pci_config(device, cap_ofs, 4);
		satacr1 = pci->read_pci_config(device, cap_ofs + 4, 4);
		TRACE("satacr0 = 0x%08x, satacr1 = 0x%08x\n", satacr0, satacr1);
	}
#endif
