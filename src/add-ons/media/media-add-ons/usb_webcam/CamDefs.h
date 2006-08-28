#ifndef _CAM_DEFS_H
#define _CAM_DEFS_H

#include <be_prim.h>

struct supported_usb_camera
{
	uint16 vendor_id;
	uint16 product_id;	
};

extern supported_usb_camera gkSupportedCameras[];

#endif
