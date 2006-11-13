/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <drivers/module.h>
#include <drivers/PCI.h>
#include <drivers/bus/PCI.h>

#include "dm_wrapper.h"
#include "pcihdr.h"
#include "pci-utils.h"

extern const char *__progname;

#define DUMP_MODE	0
#define USER_MODE	1
int gMode = USER_MODE;

#define BUS_ISA		1
#define BUS_PCI		2


static void
usage()
{
	fprintf(stderr, "usage: %s [-d]\n", __progname);
	fprintf(stderr, "Displays devices in a user friendly way\n");
	fprintf(stderr, "-d : dumps the tree\n");
	exit(0);
}


static void
put_level(int32 level)
{
	while (level-- > 0)
		printf("   ");
}


static void
dump_attribute(struct dev_attr *attr, int32 level)
{
	if (attr == NULL)
		return;

	put_level(level);
	printf("\"%s\" : ", attr->name);
	switch (attr->type) {
		case B_STRING_TYPE:
			printf("string : \"%s\"", attr->value.string);
			break;
		case B_UINT8_TYPE:
			printf("uint8 : %u (%#x)", attr->value.ui8, attr->value.ui8);
			break;
		case B_UINT16_TYPE:
			printf("uint16 : %u (%#x)", attr->value.ui16, attr->value.ui16);
			break;
		case B_UINT32_TYPE:
			printf("uint32 : %lu (%#lx)", attr->value.ui32, attr->value.ui32);
			break;
		case B_UINT64_TYPE:
			printf("uint64 : %Lu (%#Lx)", attr->value.ui64, attr->value.ui64);
			break;
		default:
			printf("raw data");
	}
	printf("\n");	
}


static void
dump_device(uint32 *node, uint8 level)
{
	char data[256];
	struct dev_attr attr;
	attr.cookie = 0;
	attr.node_cookie = *node;
	attr.value.raw.data = data;
	attr.value.raw.length = sizeof(data);

	while (dm_get_next_attr(&attr) == B_OK) {
		dump_attribute(&attr, level);
	}
}


static void
dump_nodes(uint32 *node, uint8 level)
{
	status_t err;
	uint32 child = *node;
	dump_device(node, level);
	
	printf("node %lu\n", *node);

	if (get_child(&child) != B_OK)
		return;

	do {
		dump_nodes(&child, level+1);
	} while ((err = get_next_child(&child)) == B_OK);

}


static void
display_device(uint32 *node, uint8 level, int parent_bus, int *bus)
{
	char data[256];
	struct dev_attr attr;

	uint8 is_bus = 0;
	char connection[255] = "";

	uint8 pci_class_base_id = 0;
	uint8 pci_class_sub_id = 0;
	uint8 pci_class_api_id = 0;
	uint16 pci_vendor_id = 0;
	uint16 pci_device_id = 0;
	uint16 pci_subsystem_vendor_id = 0;
	uint16 pci_subsystem_id = 0;
	
	const char *venShort;
	const char *venFull;
	const char *devShort;
	const char *devFull;
	char classInfo[255];
		
	attr.cookie = 0;
	attr.node_cookie = *node;
	attr.value.raw.data = data;
	attr.value.raw.length = sizeof(data);

	while (dm_get_next_attr(&attr) == B_OK) {
		if (!strcmp(attr.name, PNP_BUS_IS_BUS) 
			&& attr.type == B_UINT8_TYPE) {
			is_bus = attr.value.ui8;
		} else if (!strcmp(attr.name, PNP_DRIVER_CONNECTION)
			&& attr.type == B_STRING_TYPE) {
			strlcpy(connection, attr.value.string, 255);
		}

		switch (parent_bus) {
			case BUS_ISA:
				break;
			case BUS_PCI:
				if (!strcmp(attr.name, PCI_DEVICE_BASE_CLASS_ID_ITEM)
					&& attr.type == B_UINT8_TYPE) 
					pci_class_base_id = attr.value.ui8;
				else if (!strcmp(attr.name, PCI_DEVICE_SUB_CLASS_ID_ITEM)
					&& attr.type == B_UINT8_TYPE)
					pci_class_sub_id = attr.value.ui8;
				else if (!strcmp(attr.name, PCI_DEVICE_API_ID_ITEM)
					&& attr.type == B_UINT8_TYPE)
					pci_class_api_id = attr.value.ui8;
				else if (!strcmp(attr.name, PCI_DEVICE_VENDOR_ID_ITEM)
					&& attr.type == B_UINT16_TYPE)
					pci_vendor_id = attr.value.ui16;
				else if (!strcmp(attr.name, PCI_DEVICE_DEVICE_ID_ITEM)
					&& attr.type == B_UINT16_TYPE)
					pci_device_id = attr.value.ui16;
				break;
		};
	}

	if (is_bus) {
		if (!strcmp(connection, "ISA"))
			*bus = BUS_ISA;
		else if (!strcmp(connection, "PCI"))
			*bus = BUS_PCI;
	}

	switch (parent_bus) {
		case BUS_ISA:
			break;
		case BUS_PCI:
			get_class_info(pci_class_base_id, pci_class_sub_id, pci_class_api_id, classInfo);
			printf("device %s [%x|%x|%x]\n", classInfo, pci_class_base_id, pci_class_sub_id, pci_class_api_id);
			
			get_vendor_info(pci_vendor_id, &venShort, &venFull);
			if (!venShort && !venFull) {
				printf("vendor %04x: Unknown\n", pci_vendor_id);
			} else if (venShort && venFull) {
				printf("vendor %04x: %s - %s\n", pci_vendor_id, venShort, venFull);
			} else {
				printf("vendor %04x: %s\n", pci_vendor_id, venShort ? venShort : venFull);
			}
		
			get_device_info(pci_vendor_id, pci_device_id, pci_subsystem_vendor_id, pci_subsystem_id, 
				&devShort, &devFull);
			if (!devShort && !devFull) {
				printf("device %04x: Unknown\n", pci_device_id);
			} else if (devShort && devFull) {
				printf("device %04x: %s - %s\n", pci_device_id, devShort, devFull);
			} else {
				printf("device %04x: %s\n", pci_device_id, devShort ? devShort : devFull);
			}
			printf("\n");
			
			break;
	}
}


static void
display_nodes(uint32 *node, uint8 level, int parent_bus)
{
	status_t err;
	uint32 child = *node;
	int bus = 0;
	display_device(node, level, parent_bus, &bus);

	if (get_child(&child) != B_OK)
		return;
		
	do {
		display_nodes(&child, level+1, bus);
	} while ((err = get_next_child(&child)) == B_OK);
}


int
main(int argc, char **argv)
{
	status_t error;
	uint32 root;

	if ((error = init_dm_wrapper()) < 0) {
		printf("Error initializing device manager (%s)\n", strerror(error));
		return error;
	}
	
	if (argc > 2)
                usage();

	if (argc == 2) {
		if (!strcmp(argv[1], "-d")) {
			gMode = DUMP_MODE;
		} else {
			usage();
		}
	}

	if (gMode == DUMP_MODE) {
		get_root(&root);
		dump_nodes(&root, 0);
	} else {
		get_root(&root);
		display_nodes(&root, 0, 0);
	}

	uninit_dm_wrapper();

	return 0;
}
