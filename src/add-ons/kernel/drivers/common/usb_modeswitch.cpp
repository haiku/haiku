/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, korli@users.berlios.de
 */

/*
	Devices and messages reference: usb-modeswitch-data-20100826
	Huawei devices updated to usb-modeswitch-data-20150115
*/

#include <ByteOrder.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <lock.h>
#include <USB3.h>

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define DRIVER_NAME			"usb_modeswitch"

#define TRACE_USB_MODESWITCH 1
#ifdef TRACE_USB_MODESWITCH
#define TRACE(x...)			dprintf(DRIVER_NAME ": " x)
#else
#define TRACE(x...)			/* nothing */
#endif
#define TRACE_ALWAYS(x...)	dprintf(DRIVER_NAME ": " x)
#define ENTER()	TRACE("%s", __FUNCTION__)


enum msgType {
	MSG_HUAWEI_1 = 0,
	MSG_HUAWEI_2,
	MSG_HUAWEI_3,
	MSG_NOKIA_1,
	MSG_OLIVETTI_1,
	MSG_OLIVETTI_2,
	MSG_OPTION_1,
	MSG_ATHEROS_1,
	MSG_ZTE_1,
	MSG_ZTE_2,
	MSG_ZTE_3,
	MSG_NONE
};


unsigned char kDevicesMsg[][31] = {
	{ 	/* MSG_HUAWEI_1 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
		0x06, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_HUAWEI_2 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x0a, 0x11,
		0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{ 	/* MSG_HUAWEI_3 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11,
		0x06, 0x20, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_NOKIA_1 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_OLIVETTI_1 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_OLIVETTI_2 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0xc0, 0x00, 0x00, 0x00, 0x80, 0x01, 0x06, 0x06,
		0xf5, 0x04, 0x02, 0x52, 0x70, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_OPTION_1 */
		0x55, 0x53, 0x42, 0x43, 0x78, 0x56, 0x34, 0x12,
		0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x06, 0x10,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_ATHEROS_1 */
		0x55, 0x53, 0x42, 0x43, 0x29, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_ZTE_1 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x78,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1e,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_ZTE_2 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x79,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x1b,
		0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	{	/* MSG_ZTE_3 */
		0x55, 0x53, 0x42, 0x43, 0x12, 0x34, 0x56, 0x70,
		0x20, 0x00, 0x00, 0x00, 0x80, 0x00, 0x0c, 0x85,
		0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};


#define HUAWEI_VENDOR	0x12d1
#define NOKIA_VENDOR	0x0421
#define NOVATEL_VENDOR	0x1410
#define ZYDAS_VENDOR	0x0ace
#define ZTE_VENDOR		0x19d2
#define OLIVETTI_VENDOR	0x0b3c
#define OPTION_VENDOR	0x0af0
#define ATHEROS_VENDOR	0x0cf3


static const struct {
	usb_support_descriptor desc;
	msgType type, type2, type3;
} kDevices[] = {
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x101e}, MSG_HUAWEI_1},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1030}, MSG_HUAWEI_2},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1031}, MSG_HUAWEI_2},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1446}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1449}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14ad}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14b5}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14b7}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14ba}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14c1}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14c3}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14c4}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14c5}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14d1}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x14fe}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1505}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x151a}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1520}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1521}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1523}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1526}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1553}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1557}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x155b}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x156a}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1576}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x157d}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1583}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x15ca}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x15e7}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1c0b}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1c1b}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1c24}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1da1}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f01}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f02}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f03}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f11}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f15}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f16}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f17}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f18}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f19}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f1b}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f1c}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f1d}, MSG_HUAWEI_3},
	{{ 0, 0, 0, HUAWEI_VENDOR, 0x1f1e}, MSG_HUAWEI_3},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x060c}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x0610}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x061d}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x0622}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x0627}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x062c}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x0632}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOKIA_VENDOR, 0x0637}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5010}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5020}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5030}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5031}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5041}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x5059}, MSG_NOKIA_1},
	{{ 0, 0, 0, NOVATEL_VENDOR, 0x7001}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZYDAS_VENDOR, 0x2011}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZYDAS_VENDOR, 0x20ff}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0013}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0026}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0031}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0083}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0101}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0115}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0120}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0169}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0325}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1001}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1007}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1009}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1013}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1017}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1171}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1175}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1179}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1201}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1523}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0xffde}, MSG_NOKIA_1},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0003}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0053}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0103}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0154}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1224}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1517}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x1542}, MSG_ZTE_1, MSG_ZTE_2},
	{{ 0, 0, 0, ZTE_VENDOR, 0x0149}, MSG_ZTE_1, MSG_ZTE_2, MSG_ZTE_3},
	{{ 0, 0, 0, ZTE_VENDOR, 0x2000}, MSG_ZTE_1, MSG_ZTE_2, MSG_ZTE_3},
	{{ 0, 0, 0, OLIVETTI_VENDOR, 0xc700}, MSG_OLIVETTI_1},
	{{ 0, 0, 0, OLIVETTI_VENDOR, 0xf000}, MSG_OLIVETTI_2},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6711}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6731}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6751}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6771}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6791}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6811}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6911}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6951}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x6971}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7011}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7031}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7051}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7071}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7111}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7211}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7251}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7271}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7301}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7311}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7361}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7381}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7401}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7501}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7601}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7701}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7706}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7801}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x7901}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8006}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8200}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8201}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8300}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8302}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8304}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8400}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8600}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8800}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x8900}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0x9000}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xc031}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xc100}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xc031}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd013}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd031}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd033}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd035}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd055}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd057}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd058}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd155}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd157}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd255}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd257}, MSG_OPTION_1},
	{{ 0, 0, 0, OPTION_VENDOR, 0xd357}, MSG_OPTION_1},
	{{ 0, 0, 0, ATHEROS_VENDOR, 0x20ff}, MSG_ATHEROS_1},
};
static uint32 kDevicesCount = sizeof(kDevices) / sizeof(kDevices[0]);


typedef struct _my_device {
	usb_device	device;
	bool		removed;
	mutex		lock;
	struct _my_device *link;

	// device state
	usb_pipe	bulk_in;
	usb_pipe	bulk_out;
	uint8		interface;
	uint8       alternate_setting;

	// used to store callback information
	sem_id		notify;
	status_t	status;
	size_t		actual_length;

	msgType		type[3];
} my_device;


int32 api_version = B_CUR_DRIVER_API_VERSION;
static usb_module_info *gUSBModule = NULL;
static my_device *gDeviceList = NULL;
static uint32 gDeviceCount = 0;
static mutex gDeviceListLock;


//
//#pragma mark - Device Allocation Helper Functions
//


static void
my_free_device(my_device *device)
{
	mutex_lock(&device->lock);
	mutex_destroy(&device->lock);
	delete_sem(device->notify);
	free(device);
}


//
//#pragma mark - Bulk-only Functions
//


static void
my_callback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	my_device *device = (my_device *)cookie;
	device->status = status;
	device->actual_length = actualLength;
	release_sem(device->notify);
}


static status_t
my_transfer_data(my_device *device, bool directionIn, void *data,
	size_t dataLength)
{
	status_t result = gUSBModule->queue_bulk(directionIn ? device->bulk_in
		: device->bulk_out, data, dataLength, my_callback, device);
	if (result != B_OK) {
		TRACE_ALWAYS("failed to queue data transfer\n");
		return result;
	}

	do {
		bigtime_t timeout = 500000;
		result = acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT,
			timeout);
		if (result == B_TIMED_OUT) {
			// Cancel the transfer and collect the sem that should now be
			// released through the callback on cancel. Handling of device
			// reset is done in usb_printer_operation() when it detects that
			// the transfer failed.
			gUSBModule->cancel_queued_transfers(directionIn ? device->bulk_in
				: device->bulk_out);
			acquire_sem_etc(device->notify, 1, B_RELATIVE_TIMEOUT, 0);
		}
	} while (result == B_INTERRUPTED);

	if (result != B_OK) {
		TRACE_ALWAYS("acquire_sem failed while waiting for data transfer\n");
		return result;
	}

	return B_OK;
}


enum msgType
my_get_msg_type(const usb_device_descriptor *desc, int index)
{
	for (uint32 i = 0; i < kDevicesCount; i++) {
		if (kDevices[i].desc.dev_class != 0x0
			&& kDevices[i].desc.dev_class != desc->device_class)
			continue;
		if (kDevices[i].desc.dev_subclass != 0x0
			&& kDevices[i].desc.dev_subclass != desc->device_subclass)
			continue;
		if (kDevices[i].desc.dev_protocol != 0x0
			&& kDevices[i].desc.dev_protocol != desc->device_protocol)
			continue;
		if (kDevices[i].desc.vendor != 0x0
			&& kDevices[i].desc.vendor != desc->vendor_id)
			continue;
		if (kDevices[i].desc.product != 0x0
			&& kDevices[i].desc.product != desc->product_id)
			continue;
		switch (index) {
			case 0:
				return kDevices[i].type;
			case 1:
				return kDevices[i].type2;
			case 2:
				return kDevices[i].type3;
		}

	}

	return MSG_NONE;
}



status_t
my_modeswitch(my_device* device)
{
	status_t err = B_OK;
	if (device->type[0] == MSG_NONE)
			return B_OK;
	for (int i = 0; i < 3; i++) {
		if (device->type[i] == MSG_NONE)
			break;

		err = my_transfer_data(device, false, kDevicesMsg[device->type[i]],
			sizeof(kDevicesMsg[device->type[i]]));
		if (err != B_OK) {
			TRACE_ALWAYS("send message %d failed\n", i + 1);
			return err;
		}

		TRACE("device switched: %p\n", device);

		char data[36];
		err = my_transfer_data(device, true, data, sizeof(data));
		if (err != B_OK) {
			TRACE_ALWAYS("receive response %d failed 0x%" B_PRIx32 "\n",
				i + 1, device->status);
			return err;
		}
		TRACE("device switched (response length %ld)\n", device->actual_length);
	}

	TRACE("device switched: %p\n", device);

	return B_OK;
}


//
//#pragma mark - Device Attach/Detach Notifications and Callback
//


static status_t
my_device_added(usb_device newDevice, void **cookie)
{
	TRACE("device_added(0x%08" B_PRIx32 ")\n", newDevice);
	my_device *device = (my_device *)malloc(sizeof(my_device));
	device->device = newDevice;
	device->removed = false;
	device->interface = 0xff;
	device->alternate_setting = 0;

	// scan through the interfaces to find our bulk-only data interface
	const usb_configuration_info *configuration =
		gUSBModule->get_configuration(newDevice);
	if (configuration == NULL) {
		free(device);
		return B_ERROR;
	}

	for (size_t i = 0; i < configuration->interface_count; i++) {
		usb_interface_info *interface = configuration->interface[i].active;
		if (interface == NULL)
			continue;

		if (true) {

			bool hasIn = false;
			bool hasOut = false;
			for (size_t j = 0; j < interface->endpoint_count; j++) {
				usb_endpoint_info *endpoint = &interface->endpoint[j];
				if (endpoint == NULL
					|| endpoint->descr->attributes != USB_ENDPOINT_ATTR_BULK)
					continue;

				if (!hasIn && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN)) {
					device->bulk_in = endpoint->handle;
					hasIn = true;
				} else if (!hasOut && (endpoint->descr->endpoint_address
					& USB_ENDPOINT_ADDR_DIR_IN) == 0) {
					device->bulk_out = endpoint->handle;
					hasOut = true;
				}

				if (hasIn && hasOut)
					break;
			}

			if (!(hasIn && hasOut))
				continue;

			device->interface = interface->descr->interface_number;
			device->alternate_setting = interface->descr->alternate_setting;

			break;
		}
	}

	if (device->interface == 0xff) {
		TRACE_ALWAYS("no valid interface found\n");
		free(device);
		return B_ERROR;
	}

	const usb_device_descriptor *descriptor
		= gUSBModule->get_device_descriptor(newDevice);
	if (descriptor == NULL) {
		free(device);
		return B_ERROR;
	}
	for (int i = 0; i < 3; i++) {
		device->type[i] = my_get_msg_type(descriptor, i);
	}

	mutex_init(&device->lock, DRIVER_NAME " device lock");

	sem_id callbackSem = create_sem(0, DRIVER_NAME " callback notify");
	if (callbackSem < B_OK) {
		mutex_destroy(&device->lock);
		free(device);
		return callbackSem;
	}
	device->notify = callbackSem;

	mutex_lock(&gDeviceListLock);
	device->link = gDeviceList;
	gDeviceList = device;
	mutex_unlock(&gDeviceListLock);

	*cookie = device;

	return my_modeswitch(device);
}


static status_t
my_device_removed(void *cookie)
{
	TRACE("device_removed(%p)\n", cookie);
	my_device *device = (my_device *)cookie;

	mutex_lock(&gDeviceListLock);
	if (gDeviceList == device) {
		gDeviceList = device->link;
	} else {
		my_device *element = gDeviceList;
		while (element) {
			if (element->link == device) {
				element->link = device->link;
				break;
			}

			element = element->link;
		}
	}
	gDeviceCount--;

	device->removed = true;
	gUSBModule->cancel_queued_transfers(device->bulk_in);
	gUSBModule->cancel_queued_transfers(device->bulk_out);
	my_free_device(device);

	mutex_unlock(&gDeviceListLock);
	return B_OK;
}


//
//#pragma mark - Driver Entry Points
//


status_t
init_hardware()
{
	TRACE("init_hardware()\n");
	return B_OK;
}


status_t
init_driver()
{
	TRACE("init_driver()\n");
	static usb_notify_hooks notifyHooks = {
		&my_device_added,
		&my_device_removed
	};

	gDeviceList = NULL;
	gDeviceCount = 0;
	mutex_init(&gDeviceListLock, DRIVER_NAME " device list lock");

	TRACE("trying module %s\n", B_USB_MODULE_NAME);
	status_t result = get_module(B_USB_MODULE_NAME,
		(module_info **)&gUSBModule);
	if (result < B_OK) {
		TRACE_ALWAYS("getting module failed 0x%08" B_PRIx32 "\n", result);
		mutex_destroy(&gDeviceListLock);
		return result;
	}

	size_t descriptorsSize = kDevicesCount * sizeof(usb_support_descriptor);
	usb_support_descriptor *supportedDevices =
		(usb_support_descriptor *)malloc(descriptorsSize);
	if (supportedDevices == NULL) {
		TRACE_ALWAYS("descriptor allocation failed\n");
		put_module(B_USB_MODULE_NAME);
		mutex_destroy(&gDeviceListLock);
		return result;
	}

	for (uint32 i = 0; i < kDevicesCount; i++)
		supportedDevices[i] = kDevices[i].desc;

	gUSBModule->register_driver(DRIVER_NAME, supportedDevices, kDevicesCount,
		NULL);
	gUSBModule->install_notify(DRIVER_NAME, &notifyHooks);
	free(supportedDevices);
	return B_OK;
}


void
uninit_driver()
{
	TRACE("uninit_driver()\n");
	gUSBModule->uninstall_notify(DRIVER_NAME);
	mutex_lock(&gDeviceListLock);
	mutex_destroy(&gDeviceListLock);
	put_module(B_USB_MODULE_NAME);
}


const char **
publish_devices()
{
	TRACE("publish_devices()\n");
	return NULL;
}


device_hooks *
find_device(const char *name)
{
	TRACE("find_device()\n");
	return NULL;
}
