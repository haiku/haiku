#ifndef _PCI_P_H
#define _PCI_P_H

#define CONFIG_ADDRESS 0xcf8
#define CONFIG_DATA 0xcfc

struct pci_config_address {
	unsigned reg : 8,
		function : 3,
		unit : 5,
		bus : 8,
		reserved : 7,
		enable : 1;
};

void dump_pci_config(struct pci_cfg *cfg);
int pci_probe(uint8 bus, uint8 unit, uint8 function, struct pci_cfg *cfg);

#endif


