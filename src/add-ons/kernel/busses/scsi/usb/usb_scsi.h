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
#ifndef _USB_SCSI_H_ 
	#define _USB_SCSI_H_

#ifndef _OS_H
	#include <OS.h>
#endif //_OS_H

#ifndef _USB_V3_H_
	#include <USB3.h>
#endif /* _USB_V3_H_ */

#ifndef _CAM_H
	#include <CAM.h>
#endif /*_CAM_H*/

#define MODULE_NAME "usb_scsi"
	
#define CONTROLLER_SCSI_BUS 0x00 /* Narrow SCSI bus. Use PI_* to alter this*/
#define MAX_DEVICES_COUNT	0x07 /* simulate Narrow SCSI bus - 8 devices*/
#define CONTROLLER_SCSI_ID	0x07 /* "controller" SCSI ID */
#define MAX_LUNS_COUNT		0x08

/* transport protocol definitions - are not bitmasks */
#define PROTO_NONE		0x00000000
#define PROTO_BULK_ONLY	0x00000001
#define PROTO_CB		0x00000002
#define PROTO_CBI		0x00000003

#define PROTO_VENDOR	0x0000000e
#define PROTO_MASK		0x0000000f

#define PROTO(__value) ((__value) & PROTO_MASK)
					
/* command set definitions	- are not bitmasks */
#define CMDSET_NONE		0x00000000
#define CMDSET_SCSI		0x00000010
#define CMDSET_UFI		0x00000020
#define CMDSET_ATAPI	0x00000030
#define CMDSET_RBC		0x00000040
#define CMDSET_QIC157	0x00000050

#define CMDSET_VENDOR	0x000000e0
#define CMDSET_MASK		0x000000f0

#define CMDSET(__value)((__value) & CMDSET_MASK)

#define HAS_SET(__mask, __flag) \
					(((__mask) & __flag) == (__flag))

/* fixes - bitmasked */ 
#define FIX_NO_GETMAXLUN		0x00000100
#define FIX_FORCE_RW_TO_6		0x00000200
#define FIX_NO_TEST_UNIT		0x00000400
#define FIX_NO_INQUIRY			0x00000800
#define FIX_TRANS_TEST_UNIT		0x00001000
#define FIX_NO_PREVENT_MEDIA	0x00002000
#define FIX_FORCE_MS_TO_10		0x00004000
#define FIX_FORCE_READ_ONLY		0x00008000

#define FIX_NONE				0x00000000
#define FIX_MASK				0x000fff00

#define HAS_FIXES(__value, __fix) \
								HAS_SET((__value), (__fix))

#endif /*_USB_SCSI_H_*/

