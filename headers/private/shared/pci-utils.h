/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */

#include <stdio.h>

static void
get_class_info(uint8 pci_class_base_id, uint8 pci_class_sub_id, uint8 pci_class_api_id, char *classInfo)
{
	if (pci_class_base_id == 0x80)
		sprintf(classInfo, " (Other)");
	else {
		PCI_CLASSCODETABLE *s = PciClassCodeTable, *foundItem = NULL;
		while (s->BaseDesc) {
			if ((pci_class_base_id == s->BaseClass) 
				&& (pci_class_sub_id == s->SubClass)) {
				foundItem = s;
				if (pci_class_api_id == s->ProgIf)
					break;
			}
			s++;
		}
		if (!s->BaseDesc)
			s = foundItem;
		if (pci_class_sub_id != 0x80)
			sprintf(classInfo, "%s (%s%s%s)", s->BaseDesc, s->SubDesc, 
				(s->ProgDesc && strcmp("", s->ProgDesc)) ? ", " : "", s->ProgDesc);
		else
			sprintf(classInfo, "%s", s->BaseDesc);
	}
}

static void
get_vendor_info(uint16 vendorID, const char **venShort, const char **venFull)
{
	int i;
	for (i = 0; i < (int)PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == vendorID) {
			if (PciVenTable[i].VenShort && PciVenTable[i].VenFull
				&& 0 == strcmp(PciVenTable[i].VenShort, PciVenTable[i].VenFull)) {
				*venShort = PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = NULL;
			} else {
				*venShort = PciVenTable[i].VenShort && PciVenTable[i].VenShort[0] ? PciVenTable[i].VenShort : NULL;
				*venFull = PciVenTable[i].VenFull && PciVenTable[i].VenFull[0] ? PciVenTable[i].VenFull : NULL;
			}
			return;
		}
	}
	*venShort = NULL;
	*venFull = NULL;
}


static void
get_device_info(uint16 vendorID, uint16 deviceID, 
	uint16 subvendorID, uint16 subsystemID, const char **devShort, const char **devFull)
{
	int i;
	*devShort = NULL;
	*devFull = NULL;
	
	// search for the device
	for (i = 0; i < (int)PCI_DEVTABLE_LEN; i++) {
		if (PciDevTable[i].VenId == vendorID && PciDevTable[i].DevId == deviceID ) {
			*devShort = PciDevTable[i].Chip && PciDevTable[i].Chip[0] ? PciDevTable[i].Chip : NULL;
			*devFull = PciDevTable[i].ChipDesc && PciDevTable[i].ChipDesc[0] ? PciDevTable[i].ChipDesc : NULL;
			break;
		}
	}

	// search for the subsystem eventually
	for (; i < (int)PCI_DEVTABLE_LEN; i++) {
		if (PciDevTable[i].VenId != vendorID || PciDevTable[i].DevId != deviceID)
			break;
		if (PciDevTable[i].SubVenId == subvendorID && PciDevTable[i].SubDevId == subsystemID ) {
			*devShort = PciDevTable[i].Chip && PciDevTable[i].Chip[0] ? PciDevTable[i].Chip : NULL;
			*devFull = PciDevTable[i].ChipDesc && PciDevTable[i].ChipDesc[0] ? PciDevTable[i].ChipDesc : NULL;
			break;
		}
	}
}
