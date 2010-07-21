#ifndef _CONFIG_MANAGER_H
#define _CONFIG_MANAGER_H

#include <module.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	B_ISA_BUS,
	B_PCI_BUS,
	B_PCMCIA_BUS,
	B_UNKNOWN_BUS = 0x80
} bus_type;

typedef struct {
	uchar	base;
	uchar	subtype;
	uchar	interface;
} device_type;

typedef struct {
	uint32	mask;
	uint32	flags;
	uint32	cookie;
} resource_mask_descriptor;

typedef struct {
	uint32	minbase;
	uint32	maxbase;
	uint32	basealign;
	uint32	len;
	uint32	flags;
	uint32	cookie;
} resource_range_descriptor;

typedef enum {
	B_IRQ_RESOURCE,
	B_DMA_RESOURCE,
	B_IO_PORT_RESOURCE,
	B_MEMORY_RESOURCE
} resource_type;

typedef struct {
	resource_type type;
	union {
		resource_mask_descriptor m;
		resource_range_descriptor r;
		uint32 for_padding[7];
	} d;
} resource_descriptor;

struct device_configuration {
	uint32 flags;
	uint32 num_resources;
	resource_descriptor resources[0];
};

struct possible_device_configurations {
	uint32 flags;
	uint32 num_possible;
	struct device_configuration possible[0];
};

struct device_info {
	uint32		size;
	uint32		bus_dependent_info_offset;
	bus_type	bus;
	device_type	devtype;
	uint32		id[4];		/* unique (per-bus) normally-persistent id for device */
	uint32		flags;
	status_t	config_status;

	/* bus-dependent data goes here... */
};

#define B_DEVICE_INFO_ENABLED			1
#define B_DEVICE_INFO_CONFIGURED		2
#define B_DEVICE_INFO_CAN_BE_DISABLED	4

typedef struct config_manager_for_driver_module_info {
	module_info	minfo;

	status_t	(*get_next_device_info)(bus_type bus, uint64 *cookie,
						struct device_info *info, uint32 len);
	status_t	(*get_device_info_for)(uint64 device,
						struct device_info *info, uint32 len);

	status_t	(*get_size_of_current_configuration_for)(uint64 device);
	status_t	(*get_current_configuration_for)(uint64 device,
						struct device_configuration *config, uint32 len);

	status_t	(*get_size_of_possible_configurations_for)(uint64 device);
	status_t	(*get_possible_configurations_for)(uint64 device,
						struct possible_device_configurations *possible,
						uint32 len);

	/* helper routines for drivers */
	status_t	(*count_resource_descriptors_of_type)(
						const struct device_configuration *config,
						resource_type type);
	status_t	(*get_nth_resource_descriptor_of_type)(
						const struct device_configuration *config, uint32 n,
						resource_type type, resource_descriptor *descriptor,
						uint32 len);
} config_manager_for_driver_module_info;

int config_manager_scan_hardcoded(struct device_info **info, int32 *count);

#define	B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME	"bus_managers/config_manager/driver/v1"

#ifdef __cplusplus
}
#endif

#endif	/* _CONFIG_MANAGER_H */
