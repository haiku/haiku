#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <drivers/module.h>
#include <drivers/PCI.h>

#include "dm_wrapper.h"

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

	if (get_child(&child) != B_OK)
		return;

	do {
		dump_nodes(&child, level+1);
	} while ((err = get_next_child(&child)) == B_OK);

}

int main()
{
	status_t error;
	uint32 root;

	if ((error = init_dm_wrapper()) < 0) {
		printf("Error initializing device manager (%s)\n", strerror(error));
		return error;
	}

	get_root(&root);
	dump_nodes(&root, 0);

	uninit_dm_wrapper();

	return 0;
}
