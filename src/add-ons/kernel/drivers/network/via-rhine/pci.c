/*
 * VIA VT86C100A Rhine-II and VIA VT3043 Rhine Based Card Driver By Richard Houle
 * for the BeOS Release 5
 */
 
/*
 * Standard Libraries
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

/*
 * Driver-Related Libraries
 */
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>

/*
 * VIA Rhine Libraries
 */
#include "via-rhine.h"

/*
 * PCI Information Table (VT6102/VT6103/VT6105/VT6105M added by Karina Goddard, March 04, VT6103 and VT6105 seem to work, but buggy)
 */
struct viarhine_pci_id_info pci_tbl[] =
{
	{"VIA VT86C100A Rhine-II", 0x1106, 0x6100, 0xffff, PCI_command_io | PCI_command_master, 128},
	{"VIA VT3042 Rhine",       0x1106, 0x3043, 0xffff, PCI_command_io | PCI_command_master, 128},
	{"VIA VT6102 Rhine-II/VT6103 Tahoe 10/100M Fast Ethernet Adapter", 0x1106, 0x3065, 0xffff, PCI_command_io | PCI_command_master, 256},
	{"VIA VT6105 Rhine-III Management Adapter", 0x1106, 0x3106, 0xffff, PCI_command_io | PCI_command_master, 256},
	{"VIA VT6105M Rhine-III Management Adapter", 0x1106, 0x3053, 0xffff, PCI_command_io | PCI_command_master, 256},

{0}

};

/*
 * Chip Information Table
 */
struct viarhine_chip_info cap_tbl[] =
{
	{128, CanHaveMII},
	{128, CanHaveMII}
};

/*
 * VIA Rhine PCI Functions
 */
int32 pci_getlist(pci_info *info[], int32 maxEntries)
{
	status_t  status;
	int32     i, j;
	int32     entries;
	pci_info *item;

	item = (pci_info*)malloc(sizeof(pci_info));

	for (i = 0, entries = 0; entries < maxEntries; i++)
	{
		if ((status = pModuleInfo->get_nth_pci_info(i, item)) != B_OK)
			break;
	
		j = 0;
		while (pci_tbl[j].name != NULL)
		{
			if ((pci_tbl[j].vendor_id == item->vendor_id) &&
			    (pci_tbl[j].device_id == item->device_id))
			{
				// Check if the Device Really has an IRQ
				if ((item->u.h0.interrupt_line == 0) ||
				    (item->u.h0.interrupt_line == 0xFF))
				{
#ifdef __VIARHINE_DEBUG__
					debug_printf("pci_getlist: Found with Invalid IRQ - Check IRQ Assignement\n");
#endif
					goto next_entry;
				}

#ifdef __VIARHINE_DEBUG__
				debug_printf("pci_getlist: Found at IRQ %u\n", item->u.h0.interrupt_line);
#endif

				pModuleInfo->write_pci_config(
						item->bus,
						item->device, 
						item->function,
						PCI_command,
						2,
						pci_tbl[j].flags | pModuleInfo->read_pci_config(
								item->bus,
								item->device,
								item->function,
								PCI_command,
								2));

				if (pci_tbl[j].flags & PCI_command_master)
				{
					if (item->latency < MIN_PCI_LATENCY)
					{
#ifdef __VIARHINE_DEBUG__
						debug_printf("pci_getlist: latency too slow\n");
#endif
						pModuleInfo->write_pci_config(
							item->bus,
							item->device,
							item->function,
							PCI_latency,
							1,
							MIN_PCI_LATENCY);
					}
				}

				info[entries++] = item;
				item            = (pci_info *)malloc(sizeof(pci_info));
				goto next_entry;
			}
			
			j++;
		}
next_entry:
	}
	
	info[entries] = NULL;
	free(item);
	return entries;
}

status_t pci_freelist(pci_info *info[])
{
	int32      i;
	pci_info  *item;

	for (i = 0; (item = info[i]); i++)
		free(item);

	return B_OK;
}
