/*******************************************************************************
/
/	File:		AGP.h
/
/	Description:	Interface to the AGP bus and driver.
/	For more information, see "AGP interface specification", Revision 2.0 and 3.0,
/	Intel Corporation, 1998-2002.
/
/	Rudolf Cornelissen 6/2004.
/
*******************************************************************************/

#if !defined(_AGP_H_)
#define _AGP_H_

#include <bus_manager.h>

#ifdef __cplusplus
extern "C" {
#endif


/* -----
	agp device info
----- */

typedef struct agp_info {
	ushort	vendor_id;				/* vendor id */
	ushort	device_id;				/* device id */
	uchar	bus;					/* bus number */
	uchar	device;					/* device number on bus */
	uchar	function;				/* function number in device */
	uchar	class_sub;				/* specific device function */
	uchar	class_base;				/* device type (display vs host bridge) */
	struct
	{
		uint32	agp_cap_id;	/* AGP capability register as defined in the AGP standard */
		uint32	agp_stat;	/* AGP STATUS register as defined in the AGP standard */
		uint32	agp_cmd;	/* AGP COMMAND register as defined in the AGP standard */
	} interface;
	status_t	status;		/* B_OK if usable, B_ERROR if device did not respond
							 * according to the AGP standard */
} agp_info;

typedef struct agp_module_info agp_module_info;

struct agp_module_info {
	bus_manager_info	binfo;

	long			(*get_nth_agp_info) (
						long		index,	/* index into agp device table */
						agp_info 	*info	/* caller-supplied buffer for info */
					);
	void			(*enable_agp) (
						uint32	*command	/* max. mode to set */
					);
	//fixme: GART and APERTURE stuff is lacking for now, add here...
};

#define	B_AGP_MODULE_NAME		"bus_managers/agp/v0"


/* ---
	value for the AGP_id field in the agp_cap_id register
--- */
#define AGP_id	0x02	/* AGP device identification */


/* ---
	masks for capability ID register bits
--- */
#define AGP_id_mask		0x000000ff	/* AGP capability identification, contains value 0x02 for AGP device */
#define AGP_next_ptr	0x0000ff00	/* pointer to next item in PCI capabilities list, contains 0x00 if this is last item */
#define AGP_next_ptr_shift		 8
#define AGP_rev_minor	0x000f0000	/* AGP Revision minor number reported */
#define AGP_rev_minor_shift		16
#define AGP_rev_major	0x00f00000	/* AGP Revision major number reported */
#define AGP_rev_major_shift		20


/* ---
	masks for status and command register bits
--- */
#define AGP_2_1x		0x00000001	/* AGP Revision 2.0 1x speed transfer mode */
#define AGP_2_2x		0x00000002	/* AGP Revision 2.0 2x speed transfer mode */
#define AGP_2_4x		0x00000004	/* AGP Revision 2.0 4x speed transfer mode */
#define AGP_3_4x		0x00000001	/* AGP Revision 3.0 4x speed transfer mode */
#define AGP_3_8x		0x00000002	/* AGP Revision 3.0 8x speed transfer mode */
#define AGP_rates		0x00000007	/* mask for supported rates info */
#define AGP_rate_rev	0x00000008	/* 0 if AGP Revision 2.0 or earlier rate scheme, 1 if AGP Revision 3.0 rate scheme */
#define AGP_FW			0x00000010	/* 1 if fastwrite transfers supported */
#define AGP_4G			0x00000020	/* 1 if adresses above 4G bytes supported */
#define AGP_SBA			0x00000200	/* 1 if sideband adressing supported */
#define AGP_RQ			0xff000000	/* max. number of enqueued AGP command requests supported, minus one */
#define AGP_RQ_shift			24


/* ---
	masks for command register bits
--- */
#define AGP_enable		0x00000100	/* set to 1 if AGP should be enabled */


#ifdef __cplusplus
}
#endif

#endif
