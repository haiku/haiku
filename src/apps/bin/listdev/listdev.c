#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <drivers/config_manager.h>
#include <drivers/isapnp.h>
#include <drivers/module.h>
#include <drivers/PCI.h>

#include "config_driver.h"

#define NEXT_POSSIBLE(c) \
	(c) = (void *)((uchar *)(c) + sizeof(struct device_configuration) \
			+ (c)->num_resources * sizeof(resource_descriptor));


static void
dump_mask(uint32 mask)
{
	bool first = true;
	int i = 0;
	printf("[");
	if (!mask)
		printf("none");
	for (;mask;mask>>=1,i++)
		if (mask & 1) {
			printf("%s%d", first ? "" : ",", i);
			first = false;
		}
	printf("]");
}

static void
dump_device_configuration(struct device_configuration *c)
{
	int i, num;
	resource_descriptor r;

	num = count_resource_descriptors_of_type(c, B_IRQ_RESOURCE);
	if (num) {
		printf("irq%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IRQ_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		printf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_DMA_RESOURCE);
	if (num) {
		printf("dma%s ", (num == 1) ? "" : "s");
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_DMA_RESOURCE,
					&r, sizeof(resource_descriptor));
			dump_mask(r.d.m.mask);
		}
		printf(" ");
	}

	num = count_resource_descriptors_of_type(c, B_IO_PORT_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_IO_PORT_RESOURCE,
					&r, sizeof(resource_descriptor));
			printf("\n  io range:  min %x max %x align %x len %x",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}

	num = count_resource_descriptors_of_type(c, B_MEMORY_RESOURCE);
	if (num) {
		for (i=0;i<num;i++) {
			get_nth_resource_descriptor_of_type(c, i, B_MEMORY_RESOURCE,
					&r, sizeof(resource_descriptor));
			printf("\n  mem range: min %x max %x align %x len %x",
					r.d.r.minbase, r.d.r.maxbase,
					r.d.r.basealign, r.d.r.len);
		}
	}
	printf("\n");
}

const char *base_desc[] = {
	"Legacy",
	"Mass Storage Controller",
	"NIC",
	"Display Controller",
	"Multimedia Device",
	"Memory Controller",
	"Bridge Device",
	"Communication Device",
	"Generic System Peripheral",
	"Input Device",
	"Docking Station",
	"CPU",
	"Serial Bus Controller"
};

struct subtype_descriptions {
	uchar base, subtype;
	char *name;
} subtype_desc[] = {
	{ 0, 0, "non-VGA" },
	{ 0, 0, "VGA" },
	{ 1, 0, "SCSI" },
	{ 1, 1, "IDE" },
	{ 1, 2, "Floppy" },
	{ 1, 3, "IPI" },
	{ 1, 4, "RAID" },
	{ 2, 0, "Ethernet" },
	{ 2, 1, "Token Ring" },
	{ 2, 2, "FDDI" },
	{ 2, 3, "ATM" },
	{ 3, 0, "VGA/8514" },
	{ 3, 1, "XGA" },
	{ 4, 0, "Video" },
	{ 4, 1, "Audio" },
	{ 5, 0, "RAM" },
	{ 5, 1, "Flash" },
	{ 6, 0, "Host" },
	{ 6, 1, "ISA" },
	{ 6, 2, "EISA" },
	{ 6, 3, "MCA" },
	{ 6, 4, "PCI-PCI" },
	{ 6, 5, "PCMCIA" },
	{ 6, 6, "NuBus" },
	{ 6, 7, "CardBus" },
	{ 7, 0, "Serial" },
	{ 7, 1, "Parallel" },
	{ 8, 0, "PIC" },
	{ 8, 1, "DMA" },
	{ 8, 2, "Timer" },
	{ 8, 3, "RTC" },
	{ 9, 0, "Keyboard" },
	{ 9, 1, "Digitizer" },
	{ 9, 2, "Mouse" },
	{10, 0, "Generic" },
	{11, 0, "386" },
	{11, 1, "486" },
	{11, 2, "Pentium" },
	{11,16, "Alpha" },
	{11,32, "PowerPC" },
	{11,48, "Coprocessor" },
	{12, 0, "IEEE 1394" },
	{12, 1, "ACCESS" },
	{12, 2, "SSA" },
	{12, 3, "USB" },
	{12, 4, "Fibre Channel" },
	{255,255, NULL }
};

static void
dump_device_info(struct device_info *info, struct device_configuration *current,
		struct possible_device_configurations *possible)
{
	int i;
	struct device_configuration *config;

	printf((info->devtype.base < 13) ? base_desc[info->devtype.base] : "Unknown");
	if (info->devtype.subtype == 0x80)
		printf(" (Other)");
	else {
		struct subtype_descriptions *s = subtype_desc;
		
		while (s->name) {
			if ((info->devtype.base == s->base) && (info->devtype.subtype == s->subtype))
				break;
			s++;
		}
		printf(" (%s)", (s->name) ? s->name : "Unknown");
	}
	printf(" [%x|%x|%x]\n", info->devtype.base, info->devtype.subtype, info->devtype.interface);

	if (!(info->flags & B_DEVICE_INFO_ENABLED)) {
		printf("Device is disabled (%s)\n", strerror(info->config_status));
	} else if (info->flags & B_DEVICE_INFO_CONFIGURED) {
		printf("Current configuration: ");
		dump_device_configuration(current);
	} else {
		printf("Currently unconfigured.\n");
	}

	printf("%x configurations\n", possible->num_possible);
	config = possible->possible + 0;
	for (i=0;i<possible->num_possible;i++) {
		printf("\nPossible configuration #%d: ", i);
		dump_device_configuration(config);
		NEXT_POSSIBLE(config);
	}

	printf("\n");
}

static void
dump_isa_info(struct device_info *info)
{
	struct isa_info *isa = (struct isa_info *)
			((uchar *)info + info->bus_dependent_info_offset);

	printf("CSN %x LDN %x %s %s\n", isa->csn,
		isa->ldn, isa->card_name, isa->logical_device_name);
}

static void
dump_pci_info(struct device_info *info)
{
	struct pci_info *pci = (struct pci_info *)
			((uchar *)info + info->bus_dependent_info_offset);

	printf("vendor id: %x, device id: %x\n", pci->vendor_id, pci->device_id);
}

static void
dump_devices_for_bus(bus_type bus, void (*f)(struct device_info *))
{
	status_t size;
	uint64 cookie;
	int index = 0;
	void *current, *possible;
	struct device_info dummy, *info;

	cookie = 0;
	while (get_next_device_info(bus, &cookie, &dummy, sizeof(dummy)) == B_OK) {
		info = (struct device_info *)malloc(dummy.size);
		if (!info) {
			printf("Out of core\n");
			break;
		}

		if (get_device_info_for(cookie, info, dummy.size) != B_OK) {
			printf("Error getting device information\n");
			break;
		}

		size = get_size_of_current_configuration_for(cookie);
		if (size < B_OK) {
			printf("Error getting the size of the current configuration\n");
			break;
		}
		current = malloc(size);
		if (!current) {
			printf("Out of core\n");
			break;
		}

		if (get_current_configuration_for(cookie, current, size) != B_OK) {
			printf("Error getting the current configuration\n");
			break;
		}

		size = get_size_of_possible_configurations_for(cookie);
		if (size < B_OK) {
			printf("Error getting the size of the possible configurations\n");
			break;
		}
		possible = malloc(size);
		if (!possible) {
			printf("Out of core\n");
			break;
		}

		if (get_possible_configurations_for(cookie, possible, size) != B_OK) {
			printf("Error getting the possible configurations\n");
			break;
		}

		switch (info->bus) {
			case B_ISA_BUS : printf("ISA"); break;
			case B_PCI_BUS : printf("PCI"); break;
			default : printf("unknown"); break;
		}
		printf(" bus, device #%d: ", index++);

		dump_device_info(info, current, possible);
		if (f) {
			printf("Bus-dependent information:\n");
			(*f)(info);
		}
		printf("\n");

		free(possible);
		free(current);
		free(info);
	}
}

int main()
{
	status_t error;

	if ((error = init_cm_wrapper()) < 0) {
		printf("Error initializing configuration manager (%s)\n", strerror(error));
		return error;
	}

	dump_devices_for_bus(B_ISA_BUS, dump_isa_info);
	dump_devices_for_bus(B_PCI_BUS, dump_pci_info);

	uninit_cm_wrapper();

	return 0;
}
