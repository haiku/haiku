/*
 *	Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 *	Copyright 2010, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */
#ifndef _PCI_x86_MSI_H
#define _PCI_x86_MSI_H

#include <OS.h>
#include <SupportDefs.h>


// Message Signaled Interrupts


// MSI
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


// MSI-X
typedef struct msix_info {
	bool	msix_capable;
	uint8	capability_offset;
	uint8	message_count;
	uint8	table_bar;
	uint32	table_offset;
	area_id	table_area_id;
	addr_t 	table_address;
	uint8	pba_bar;
	uint32	pba_offset;
	area_id	pba_area_id;
	addr_t	pba_address;
	uint8	configured_count;
	uint8	start_vector;
	uint16	control_value;
	uint16	data_value;
	uint64	address_value;
} msix_info;


// HyperTransport MSI mapping
typedef struct ht_mapping_info {
	bool	ht_mapping_capable;
	uint8	capability_offset;
	uint16	control_value;
	uint64	address_value;
} ht_mapping_info;


#endif // _PCI_x86_MSI_H
