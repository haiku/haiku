/**
 *
 * TODO: description
 * 
 * This file is a part of USB SCSI CAM for Haiku OS.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 * 	
 * 	
 */
/** driver settings support definitions */

#ifndef _USB_SCSI_SETTINGS_H_ 
	#define _USB_SCSI_SETTINGS_H_

void load_module_settings(void);

typedef struct{
	uint16 vendor_id;
	uint16 product_id;
	uint32 properties;
#define VENDOR_PROP_NAME_LEN	0x08
	char vendor_protocol[VENDOR_PROP_NAME_LEN + 1];
	char vendor_commandset[VENDOR_PROP_NAME_LEN + 1];
}usb_device_settings;

bool lookup_device_settings(const usb_device_descriptor *udd,
											usb_device_settings *uds);

extern int	reserved_devices;
extern int	reserved_luns;
extern bool b_reservation_on; 
extern bool b_ignore_sysinit2;

#endif /*_USB_SCSI_SETTINGS_H_*/ 

