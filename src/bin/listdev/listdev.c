/*
 * Copyright 2006-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <drivers/module.h>
#include <drivers/PCI.h>
#include <drivers/bus/PCI.h>
#include <drivers/bus/SCSI.h>

#include "dm_wrapper.h"
#include "pcihdr.h"
#include "pci-utils.h"


extern const char *__progname;

#define DUMP_MODE	0
#define USER_MODE	1
int gMode = USER_MODE;

#define BUS_ISA		1
#define BUS_PCI		2
#define BUS_SCSI 	3


static const char *
get_scsi_device_type(uint8 type)
{
	switch (type) {
		case 0x0: return "Direct Access";
		case 0x1: return "Sequential Access";
		case 0x2: return "Printer";
		case 0x3: return "Processor";
		case 0x4: return "WORM";
		case 0x5: return "CDROM";
		case 0x6: return "Scanner";
		case 0x7: return "Optical memory";
		case 0x8: return "Medium changer";
		case 0x9: return "Communication";
		case 0xc: return "Storage array controller";
		case 0xd: return "Enclosure services";
		case 0xe: return "Simplified Direct Access";
		default: return "";
	}
}


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
dump_attribute(struct device_attr_info *attr, int32 level)
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
dump_device(device_node_cookie *node, uint8 level)
{
	char data[256];
	struct device_attr_info attr;
	attr.cookie = 0;
	attr.node_cookie = *node;
	attr.value.raw.data = data;
	attr.value.raw.length = sizeof(data);

	put_level(level);
	printf("(%d)\n", level);
	while (dm_get_next_attr(&attr) == B_OK) {
		dump_attribute(&attr, level);
	}
}


static void
dump_nodes(device_node_cookie *node, uint8 level)
{
	status_t err;
	device_node_cookie child = *node;
	dump_device(node, level);

	if (get_child(&child) != B_OK)
		return;

	do {
		dump_nodes(&child, level + 1);
	} while ((err = get_next_child(&child)) == B_OK);

}


static int32
display_device(device_node_cookie *node, uint8 level)
{
	uint8 new_level = level;

	char data[256];
	struct device_attr_info attr;

	// BUS attributes
	char device_bus[64];
	uint8 scsi_path_id = 255;
	int bus = 0;

	// PCI attributes
	uint8 pci_class_base_id = 0;
	uint8 pci_class_sub_id = 0;
	uint8 pci_class_api_id = 0;
	uint16 pci_vendor_id = 0;
	uint16 pci_device_id = 0;
	uint16 pci_subsystem_vendor_id = 0;
	uint16 pci_subsystem_id = 0;

	// SCSI attributes
	uint8 scsi_target_lun = 0;
	uint8 scsi_target_id = 0;
	uint8 scsi_type = 255;
	char scsi_vendor[64];
	char scsi_product[64];

	const char *venShort;
	const char *venFull;
	const char *devShort;
	const char *devFull;

	attr.cookie = 0;
	attr.node_cookie = *node;
	attr.value.raw.data = data;
	attr.value.raw.length = sizeof(data);

	while (dm_get_next_attr(&attr) == B_OK) {
		if (!strcmp(attr.name, B_DEVICE_BUS)
			&& attr.type == B_STRING_TYPE) {
			strlcpy(device_bus, attr.value.string, 64);
		} else if (!strcmp(attr.name, "scsi/path_id")
			&& attr.type == B_UINT8_TYPE) {
			scsi_path_id = attr.value.ui8;
		} else if (!strcmp(attr.name, B_DEVICE_TYPE)
			&& attr.type == B_UINT16_TYPE)
			pci_class_base_id = attr.value.ui8;
		else if (!strcmp(attr.name, B_DEVICE_SUB_TYPE)
			&& attr.type == B_UINT16_TYPE)
			pci_class_sub_id = attr.value.ui8;
		else if (!strcmp(attr.name, B_DEVICE_INTERFACE)
			&& attr.type == B_UINT16_TYPE)
			pci_class_api_id = attr.value.ui8;
		else if (!strcmp(attr.name, B_DEVICE_VENDOR_ID)
			&& attr.type == B_UINT16_TYPE)
			pci_vendor_id = attr.value.ui16;
		else if (!strcmp(attr.name, B_DEVICE_ID)
			&& attr.type == B_UINT16_TYPE)
			pci_device_id = attr.value.ui16;
		else if (!strcmp(attr.name, SCSI_DEVICE_TARGET_LUN_ITEM)
			&& attr.type == B_UINT8_TYPE)
			scsi_target_lun = attr.value.ui8;
		else if (!strcmp(attr.name, SCSI_DEVICE_TARGET_ID_ITEM)
			&& attr.type == B_UINT8_TYPE)
			scsi_target_id = attr.value.ui8;
		else if (!strcmp(attr.name, SCSI_DEVICE_TYPE_ITEM)
			&& attr.type == B_UINT8_TYPE)
			scsi_type = attr.value.ui8;
		else if (!strcmp(attr.name, SCSI_DEVICE_VENDOR_ITEM)
			&& attr.type == B_STRING_TYPE)
			strlcpy(scsi_vendor, attr.value.string, 64);
		else if (!strcmp(attr.name, SCSI_DEVICE_PRODUCT_ITEM)
			&& attr.type == B_STRING_TYPE)
			strlcpy(scsi_product, attr.value.string, 64);

		if (!strcmp(device_bus, "isa"))
			bus = BUS_ISA;
		else if (!strcmp(device_bus, "pci"))
			bus = BUS_PCI;
		else if (scsi_path_id < 255)
			bus = BUS_SCSI;

		/*else if (!strcmp(attr.name, PCI_DEVICE_SUBVENDOR_ID_ITEM)
			&& attr.type == B_UINT16_TYPE)
			pci_subsystem_vendor_id = attr.value.ui16;
		else if (!strcmp(attr.name, PCI_DEVICE_SUBSYSTEM_ID_ITEM)
			&& attr.type == B_UINT16_TYPE)
			pci_subsystem_id = attr.value.ui16;*/

		attr.value.raw.data = data;
		attr.value.raw.length = sizeof(data);
	}

	switch (bus) {
		case BUS_ISA:
			new_level = level + 1;
			break;
		case BUS_PCI:
			printf("\n");
			{
				char classInfo[64];
				get_class_info(pci_class_base_id, pci_class_sub_id,
					pci_class_api_id, classInfo, 64);
				put_level(level);
				printf("device %s [%x|%x|%x]\n", classInfo, pci_class_base_id,
					pci_class_sub_id, pci_class_api_id);
			}

			put_level(level);
			printf("  ");
			get_vendor_info(pci_vendor_id, &venShort, &venFull);
			if (!venShort && !venFull) {
				printf("vendor %04x: Unknown\n", pci_vendor_id);
			} else if (venShort && venFull) {
				printf("vendor %04x: %s - %s\n", pci_vendor_id,
					venShort, venFull);
			} else {
				printf("vendor %04x: %s\n", pci_vendor_id,
					venShort ? venShort : venFull);
			}

			put_level(level);
			printf("  ");
			get_device_info(pci_vendor_id, pci_device_id,
				pci_subsystem_vendor_id, pci_subsystem_id, &devShort, &devFull);
			if (!devShort && !devFull) {
				printf("device %04x: Unknown\n", pci_device_id);
			} else if (devShort && devFull) {
				printf("device %04x: %s (%s)\n", pci_device_id,
					devShort, devFull);
			} else {
				printf("device %04x: %s\n", pci_device_id,
					devShort ? devShort : devFull);
			}
			new_level = level + 1;
			break;
		case BUS_SCSI:
			if (scsi_type == 255)
				break;
			put_level(level);
			printf("  device [%x|%x]\n", scsi_target_id, scsi_target_lun);
			put_level(level);
			printf("  vendor %15s\tmodel %15s\ttype %s\n", scsi_vendor,
				scsi_product, get_scsi_device_type(scsi_type));

			new_level = level + 1;
			break;
	}

	return new_level;
}


static void
display_nodes(device_node_cookie *node, uint8 level)
{
	status_t err;
	device_node_cookie child = *node;
	level = display_device(node, level);

	if (get_child(&child) != B_OK)
		return;

	do {
		display_nodes(&child, level);
	} while ((err = get_next_child(&child)) == B_OK);
}


int
main(int argc, char **argv)
{
	status_t error;
	device_node_cookie root;

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
		display_nodes(&root, 0);
	}

	uninit_dm_wrapper();

	return 0;
}
