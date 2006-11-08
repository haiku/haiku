#ifndef _CONFIG_DRIVER_H_
#define _CONFIG_DRIVER_H_

/* definitions for the /dev/misc/config driver which provides access
 * to the device_manager via calls to ioctl().
 */

/* the magic is the ioctl */

#define DM_GET_CHILD				'GCHI'
#define DM_GET_NEXT_CHILD			'GNCH'
#define DM_GET_PARENT				'GPAR'
#define DM_GET_NEXT_ATTRIBUTE			'GNAT'
#define DM_RETRIEVE_ATTRIBUTE			'RATT'

struct dev_attr {
	char		name[255];
	type_code	type;
	union {
		uint8	ui8;
		uint16	ui16;
		uint32	ui32;
		uint64	ui64;
		char	string[255];
		struct {
			void	*data;
			size_t	length;
		} raw;
	} value;
};

struct dm_ioctl_data {
	uint32				magic;
	struct dev_attr			*attr;
};

#define DM_DEVICE_NAME "misc/config"

#endif	/* _CONFIG_DRIVER_H_ */
