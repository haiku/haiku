#ifndef _CM_WRAPPER_H_
#define _CM_WRAPPER_H_

status_t init_cm_wrapper(void);
status_t uninit_cm_wrapper(void);

status_t get_next_device_info(bus_type bus, uint64 *cookie,
		struct device_info *info, uint32 size);
status_t get_device_info_for(uint64 id, struct device_info *info,
		uint32 size);
status_t get_size_of_current_configuration_for(uint64 id);
status_t get_current_configuration_for(uint64 id,
		struct device_configuration *current, uint32 size);
status_t get_size_of_possible_configurations_for(uint64 id);
status_t get_possible_configurations_for(uint64 id,
		struct possible_device_configurations *possible, uint32 size);
status_t count_resource_descriptors_of_type(
		struct device_configuration *c, resource_type type);
status_t get_nth_resource_descriptor_of_type(
		struct device_configuration *c, uint32 n, resource_type type,
		resource_descriptor *d, uint32 size);

uchar mask_to_value(uint32 mask);

#endif
