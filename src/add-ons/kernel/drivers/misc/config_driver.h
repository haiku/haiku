#ifndef _CONFIG_MANAGER_DRIVER_H_
#define _CONFIG_MANAGER_DRIVER_H_
/* 
** Distributed under the terms of the MIT License.
*/

/* definitions for the /dev/misc/config driver which provides access
 * to the config_manager via calls to ioctl().
 *
 * ToDo: This file should probably located in a publicly accessable place?!
 */

/* the magic is the ioctl */

#define CM_GET_NEXT_DEVICE_INFO 					'GNDI'
#define CM_GET_DEVICE_INFO_FOR						'GDIF'
#define CM_GET_SIZE_OF_CURRENT_CONFIGURATION_FOR	'GSCC'
#define CM_GET_CURRENT_CONFIGURATION_FOR			'GCCF'
#define CM_GET_SIZE_OF_POSSIBLE_CONFIGURATIONS_FOR	'GSPC'
#define CM_GET_POSSIBLE_CONFIGURATIONS_FOR			'GPCF'

#define CM_COUNT_RESOURCE_DESCRIPTORS_OF_TYPE		'CRDT'
#define CM_GET_NTH_RESOURCE_DESCRIPTOR_OF_TYPE		'GNRD'

struct cm_ioctl_data {
	uint32				magic;
	bus_type			bus;
	uint64				cookie;
	void				*config;
	uint32				n;
	uint32				type;
	void				*data;
	uint32				data_len;
};

#define CM_DEVICE_NAME "misc/config"

#endif	/* _CONFIG_MANAGER_DRIVER_H_ */
