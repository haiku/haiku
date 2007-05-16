#include "driver.h"

hda_controller cards[MAXCARDS];
uint32 num_cards;

pci_module_info* pci;

const char** publish_devices(void); /* Just to silence compiler */

status_t
init_hardware(void)
{
	pci_info pcii;
	status_t rc;
	long i;

	if ((rc=get_module(B_PCI_MODULE_NAME, (module_info**)&pci)) == B_OK) {
		for (i=0; pci->get_nth_pci_info(i,&pcii) == B_OK; i++) {
			if (pcii.class_base == PCI_multimedia && pcii.class_sub == PCI_hd_audio) {
				put_module(B_PCI_MODULE_NAME);
				pci = NULL;
				return B_OK;
			}
		}
	}

	put_module(B_PCI_MODULE_NAME);
	pci = NULL;

	return ENODEV;
}

status_t
init_driver (void)
{
	char path[B_PATH_NAME_LENGTH];
	pci_info pcii;
	status_t rc;
	long i;

	num_cards = 0;

	if ((rc=get_module(B_PCI_MODULE_NAME, (module_info**)&pci)) == B_OK) {
		for (i=0; pci->get_nth_pci_info(i,&pcii) == B_OK; i++) {
			if (pcii.class_base == PCI_multimedia && pcii.class_sub == PCI_hd_audio) {
				cards[num_cards].pcii = pcii;
				cards[num_cards].opened = 0;
				sprintf(path, DEVFS_PATH_FORMAT, num_cards);
				cards[num_cards++].devfs_path = strdup(path);
				
				dprintf("HDA: Detected controller @ PCI:%d:%d:%d, IRQ:%d, type %04x/%04x\n",
					pcii.bus, pcii.device, pcii.function,
					pcii.u.h0.interrupt_line,
					pcii.vendor_id, pcii.device_id);
			}
		}
	} else {
		return rc;
	}

	if (num_cards == 0) {
		put_module(B_PCI_MODULE_NAME);
		pci = NULL;
		
		return ENODEV;
	}

	return B_OK;
}

void
uninit_driver (void)
{
	long i;
	for (i=0; i < num_cards; i++) {
		free((void*)cards[i].devfs_path);
		cards[i].devfs_path = NULL;
	}
	
	if (pci != NULL) {
		put_module(B_PCI_MODULE_NAME);
		pci = NULL;
	}
}

const char**
publish_devices(void)
{
	static const char* devs[MAXCARDS+1];
	long i;
	
	for (i=0; i < num_cards; i++)
		devs[i] = cards[i].devfs_path;

	devs[i] = NULL;

	return devs;
}

device_hooks*
find_device(const char* name)
{
	return &driver_hooks;
}
