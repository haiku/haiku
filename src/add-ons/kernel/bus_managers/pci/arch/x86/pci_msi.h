/*
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */
#ifndef _PCI_x86_MSI_H
#define _PCI_x86_MSI_H

#include <SupportDefs.h>

class PCIDev;

// Message Signaled Interrupts
typedef struct msi_info {
	bool	msi_capable;
	uint8	capability_offset;
	uint8	message_count;
	uint8	configured_count;
	uint8	start_vector;
	uint16	control_value;
	uint16	data_value;
	uint64	address_value;
} msi_info;


uint8		pci_get_msi_count(uint8 virtualBus, uint8 _device, uint8 function);
status_t	pci_configure_msi(uint8 virtualBus, uint8 _device, uint8 function,
				uint8 count, uint8 *startVector);
status_t	pci_unconfigure_msi(uint8 virtualBus, uint8 device, uint8 function);
status_t	pci_enable_msi(uint8 virtualBus, uint8 device, uint8 function);
status_t	pci_disable_msi(uint8 virtualBus, uint8 device, uint8 function);
void		pci_read_msi_info(PCIDev *device);

#endif // _PCI_x86_MSI_H
