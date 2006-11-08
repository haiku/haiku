#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <drivers/device_manager.h>
#include <drivers/module.h>
#include <drivers/PCI.h>

#include "config_driver.h"
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
dump_device(uint8 level)
{
	char data[256];
	
	while (dm_get_next_attr() == B_OK) {
		struct dev_attr attr;
		attr.value.raw.data = data;
		attr.value.raw.length = sizeof(data);
		dm_retrieve_attr(&attr);
		dump_attribute(&attr, level);
	}
}


static void
dump_nodes(uint8 level)
{
	status_t err;
	dump_device(level);

	if (get_child() != B_OK)
		return;

	do {
		dump_nodes(level+1);
	} while ((err = get_next_child()) == B_OK);

	get_parent();
}

int main()
{
	status_t error;

	if ((error = init_dm_wrapper()) < 0) {
		printf("Error initializing device manager (%s)\n", strerror(error));
		return error;
	}

	dump_nodes(0);

	uninit_dm_wrapper();

	return 0;
}
