/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AGP_H_
#define _AGP_H_


#include <bus_manager.h>


typedef struct agp_info {
	ushort	vendor_id;			/* vendor id */
	ushort	device_id;			/* device id */
	uchar	bus;				/* bus number */
	uchar	device;				/* device number on bus */
	uchar	function;			/* function number in device */
	uchar	class_sub;			/* specific device function */
	uchar	class_base;			/* device type (display vs host bridge) */
	struct {
		uint32	capability_id;	/* AGP capability register */
		uint32	status;			/* AGP status register */
		uint32	command;		/* AGP command register */
	} interface;
} agp_info;

typedef struct aperture_info {
	phys_addr_t	physical_base;
	addr_t		base;
	size_t		size;
	size_t		reserved_size;	
} aperture_info;

/* flags for allocate_memory */
enum {
	B_APERTURE_NON_RESERVED		= 0x01,
	B_APERTURE_NEED_PHYSICAL	= 0x02,
};

typedef int32 aperture_id;
typedef struct gart_bus_module_info gart_bus_module_info;

typedef struct agp_gart_module_info {
	bus_manager_info info;

	/* AGP functionality */
	status_t	(*get_nth_agp_info)(uint32 index, agp_info *info);
	status_t	(*acquire_agp)(void);
	void		(*release_agp)(void);
	uint32		(*set_agp_mode)(uint32 command);

	/* GART functionality */
	aperture_id	(*map_aperture)(uint8 bus, uint8 device, uint8 function,
					size_t size, addr_t *_apertureBase);
	aperture_id	(*map_custom_aperture)(gart_bus_module_info *module,
					addr_t *_apertureBase);
	status_t	(*unmap_aperture)(aperture_id id);
	status_t	(*get_aperture_info)(aperture_id id, aperture_info *info);

	status_t	(*allocate_memory)(aperture_id id, size_t size,
					size_t alignment, uint32 flags, addr_t *_apertureBase,
					phys_addr_t *_physicalBase);
	status_t	(*free_memory)(aperture_id id, addr_t apertureBase);
	status_t	(*reserve_aperture)(aperture_id id, size_t size,
					addr_t *_apertureBase);
	status_t	(*unreserve_aperture)(aperture_id id, addr_t apertureBase);
	status_t	(*bind_aperture)(aperture_id id, area_id area, addr_t base,
					size_t size, size_t alignment, addr_t reservedBase,
					addr_t *_apertureBase);
	status_t	(*unbind_aperture)(aperture_id id, addr_t apertureBase);
} agp_gart_module_info;

#define	B_AGP_GART_MODULE_NAME "bus_managers/agp_gart/v0"

struct agp_gart_for_bus_module_info {
	module_info info;
};

#define B_AGP_GART_FOR_BUS_MODULE_NAME "bus_managers/agp_gart/bus/v0"

struct agp_gart_bus_module_info {
	module_info info;

	// TODO: add some stuff for non-generic AGP support as well

	status_t	(*create_aperture)(uint8 bus, uint8 device, uint8 function,
					size_t size, void **_aperture);
	void		(*delete_aperture)(void *aperture);

	status_t	(*get_aperture_info)(void *aperture, aperture_info *info);
	status_t	(*set_aperture_size)(void *aperture, size_t size);
	status_t	(*bind_page)(void *aperture, uint32 offset,
					phys_addr_t physicalAddress);
	status_t	(*unbind_page)(void *aperture, uint32 offset);
	void		(*flush_tlbs)(void *aperture);
};

/* defines for capability ID register bits */
#define AGP_REV_MINOR		0x000f0000	/* AGP Revision minor number reported */
#define AGP_REV_MINOR_SHIFT	16
#define AGP_REV_MAJOR		0x00f00000	/* AGP Revision major number reported */
#define AGP_REV_MAJOR_SHIFT	20

/* defines for status and command register bits */
#define AGP_2_1x			0x00000001	/* AGP Revision 2.0 1x speed transfer mode */
#define AGP_2_2x			0x00000002	/* AGP Revision 2.0 2x speed transfer mode */
#define AGP_2_4x			0x00000004	/* AGP Revision 2.0 4x speed transfer mode */
#define AGP_3_4x			0x00000001	/* AGP Revision 3.0 4x speed transfer mode */
#define AGP_3_8x			0x00000002	/* AGP Revision 3.0 8x speed transfer mode */
#define AGP_RATE_MASK		0x00000007	/* mask for supported rates info */
#define AGP_3_MODE			0x00000008	/* 0 if AGP Revision 2.0 or earlier rate scheme,
										 * 1 if AGP Revision 3.0 rate scheme */
#define AGP_FAST_WRITE		0x00000010	/* 1 if fast write transfers supported */
#define AGP_ABOVE_4G		0x00000020	/* 1 if adresses above 4G bytes supported */
#define AGP_SBA				0x00000200	/* 1 if side band adressing supported */
#define AGP_REQUEST			0xff000000	/* max. number of enqueued AGP command requests
										 * supported, minus one */
#define AGP_REQUEST_SHIFT	24

/* masks for command register bits */
#define AGP_ENABLE			0x00000100	/* set to 1 if AGP should be enabled */


#endif	/* _AGP_H_ */
