/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon kernel driver
		
	AGP fix. Some motherboard BIOSes enable FastWrite even
	though the graphics card doesn't support it. Here, we'll
	fix that (hopefully it is generic enough).
*/


#include "radeon_driver.h"


// missing PCI definitions
#define  PCI_status_cap_list	0x10	/* Support Capability List */

#define  PCI_header_type_normal		0
#define  PCI_header_type_bridge 	1
#define  PCI_header_type_cardbus 	2

#define PCI_capability_list		0x34	/* Offset of first capability list entry */
#define PCI_cb_capability_list	0x14

#define PCI_cap_list_id		0	/* Capability ID */
#define PCI_cap_id_pm		0x01	/* Power Management */
#define PCI_cap_id_agp		0x02	/* Accelerated Graphics Port */
#define PCI_cap_id_vpd		0x03	/* Vital Product Data */
#define PCI_cap_id_slotid	0x04	/* Slot Identification */
#define PCI_cap_id_msi		0x05	/* Message Signalled Interrupts */
#define PCI_cap_id_chswp	0x06	/* CompactPCI HotSwap */
#define PCI_cap_list_next	1	/* Next capability in the list */
#define PCI_cap_flags		2	/* Capability defined flags (16 bits) */
#define PCI_cap_sizeof		4


#define PCI_agp_status			4	/* Status register */
#define PCI_agp_status_rq_mask	0xff000000	/* Maximum number of requests - 1 */
#define PCI_agp_status_sba		0x0200	/* Sideband addressing supported */
#define PCI_agp_status_64bit	0x0020	/* 64-bit addressing supported */
#define PCI_agp_status_fw		0x0010	/* FW transfers supported */
#define PCI_agp_status_rate4	0x0004	/* 4x transfer rate supported */
#define PCI_agp_status_rate2	0x0002	/* 2x transfer rate supported */
#define PCI_agp_status_rate1	0x0001	/* 1x transfer rate supported */

#define PCI_agp_command			8	/* Control register */
#define PCI_agp_command_rq_mask 0xff000000  /* Master: Maximum number of requests */
#define PCI_agp_command_sba		0x0200	/* Sideband addressing enabled */
#define PCI_agp_command_agp		0x0100	/* Allow processing of AGP transactions */
#define PCI_agp_command_64bit	0x0020 	/* Allow processing of 64-bit addresses */
#define PCI_agp_command_fw		0x0010 	/* Force FW transfers */
#define PCI_agp_command_rate4	0x0004	/* Use 4x rate */
#define PCI_agp_command_rate2	0x0002	/* Use 2x rate */
#define PCI_agp_command_rate1	0x0001	/* Use 1x rate */



// helper macros for easier PCI access
#define get_pci(o, s) (*pci_bus->read_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s))
#define set_pci(o, s, v) (*pci_bus->write_pci_config)(pcii->bus, pcii->device, pcii->function, (o), (s), (v))


// show AGP capabilities
static void show_agp_status( uint32 status )
{
	SHOW_FLOW( 3, "Status (%08lx): %s%s%s%s%s%s", status,
		(status & PCI_agp_status_sba) != 0 ? "Sideband addressing " : "",
		(status & PCI_agp_status_64bit) != 0 ? "64-bit " : "",
		(status & PCI_agp_status_fw) != 0 ? "FastWrite " : "",
		(status & PCI_agp_status_rate4) != 0 ? "4x " : "",
		(status & PCI_agp_status_rate2) != 0 ? "2x " : "",
		(status & PCI_agp_status_rate1) != 0 ? "1x " : "" );
}		


// show AGP settings
static void show_agp_command( uint32 command )
{
	SHOW_FLOW( 3, "Command (%08lx): %s%s%s%s%s%s%s", command,
		(command & PCI_agp_command_sba) != 0 ? "Sideband addressing " : "",
		(command & PCI_agp_command_agp) != 0 ? "AGP-Enabled " : "AGP-Disabled ",
		(command & PCI_agp_command_64bit) != 0 ? "64-bit " : "",
		(command & PCI_agp_command_fw) != 0 ? "FastWrite " : "",
		(command & PCI_agp_command_rate4) != 0 ? "4x " : "",
		(command & PCI_agp_command_rate2) != 0 ? "2x " : "",
		(command & PCI_agp_command_rate1) != 0 ? "1x " : "" );
}


// find PCI capability
int find_capability( pci_info *pcii, uint8 capability )
{
	int try_count;
	uint16 status;
	uint8 pos;

	// check whether PCI capabilities are supported at all	
	status = get_pci( PCI_status, 2 );
	
	if( (status & PCI_status_cap_list) == 0 )
		return B_NAME_NOT_FOUND;
		
	SHOW_FLOW0( 3, "Device supports capabilities" );
	
	// get offset of first capability in list	
	switch( pcii->header_type & PCI_header_type_mask ) {
	case PCI_header_type_normal:
	case PCI_header_type_bridge:
		pos = get_pci( PCI_capability_list, 1 );
		break;
	case PCI_header_type_cardbus:
		pos = get_pci( PCI_cb_capability_list, 1 );
		break;
	default:
		SHOW_FLOW( 3, "Unknown type (%x)", pcii->header_type & PCI_header_type_mask );
		return B_ERROR;
	}
	
	// search for whished capability in linked list
	for( try_count = 48; try_count > 0 && pos >= 0x40; --try_count ) {
		uint8 id;
		
		pos &= ~3;
		
		id = get_pci( pos + PCI_cap_list_id, 1 );
		if( id == 0xff )
			return B_NAME_NOT_FOUND;
			
		if( id == capability ) {
			SHOW_FLOW( 3, "Found capability %d", capability );
			return pos;
		}
		
		SHOW_FLOW( 3, "Ignored capability %d", id );
			
		pos = get_pci( pos + PCI_cap_list_next, 1 );
	}
	
	return B_NAME_NOT_FOUND;
}


// fix invalid AGP settings
void Radeon_Fix_AGP()
{
	long pci_index;
	pci_info pci_data, *pcii;
	
	uint32 common_caps = 
		PCI_agp_status_sba | PCI_agp_status_64bit | PCI_agp_status_fw |
		PCI_agp_status_rate4 | PCI_agp_status_rate2 | PCI_agp_status_rate1;
	uint32 read_queue_depth = PCI_agp_status_rq_mask;
	
	SHOW_FLOW0( 4, "Composing common feature list" );
	
	// only required to make get_pci/set_pci working
	pcii = &pci_data;

	// find common feature set
	for( pci_index = 0;
		(*pci_bus->get_nth_pci_info)(pci_index, &pci_data) == B_NO_ERROR;
		++pci_index ) 
	{
		int offset;

		SHOW_FLOW( 3, "Checking bus %d, device %d, function %d (vendor_id=%04x, device_id=%04x):", 
			pcii->bus, pcii->device, pcii->function,
			pcii->vendor_id, pcii->device_id );
		
		offset = find_capability( pcii, PCI_cap_id_agp );
		
		if( offset > 0 ) {
			uint32 agp_status, agp_command;
			
			agp_status = get_pci( offset + PCI_agp_status, 4 );
			agp_command = get_pci( offset + PCI_agp_command, 4 );
			
			SHOW_FLOW( 3, "bus %d, device %d, function %d (vendor_id=%04x, device_id=%04x):", 
				pcii->bus, pcii->device, pcii->function,
				pcii->vendor_id, pcii->device_id );
			show_agp_status( agp_status );
			show_agp_command( agp_command );
			
			common_caps &= agp_status;
			read_queue_depth = min( read_queue_depth, agp_status & PCI_agp_status_rq_mask );
		}
	}

	SHOW_FLOW0( 3, "Combined:" );
	show_agp_command( common_caps );

	// choose features that all devices support	
	for( pci_index = 0;
		(*pci_bus->get_nth_pci_info)(pci_index, &pci_data) == B_NO_ERROR;
		++pci_index ) 
	{
		int offset;
		
		offset = find_capability( pcii, PCI_cap_id_agp );
		
		if( offset > 0 ) {
			SHOW_FLOW( 3, "Modifying bus %d, device %d, function %d (vendor_id=%04x, device_id=%04x):", 
				pcii->bus, pcii->device, pcii->function,
				pcii->vendor_id, pcii->device_id );

			set_pci( offset + PCI_agp_command, 4, common_caps | read_queue_depth );
		}
	}
}
