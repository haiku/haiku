/*
 * Copyright 2005-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Portions of this code are based on Be Sample Code released under the
 * Be Sample Code license. (USB sound device driver sample code IIRC.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <OS.h>
#include <USB3.h>

#include <kernel.h>
#include <wacom_driver.h>

int32 api_version = B_CUR_DRIVER_API_VERSION;

#define DEBUG_DRIVER 0

#if DEBUG_DRIVER
#	define DPRINTF_INFO(x) dprintf x;
#	define DPRINTF_ERR(x) dprintf x;
#else
#	define DPRINTF_INFO(x)
#	define DPRINTF_ERR(x) dprintf x;
#endif

typedef struct wacom_device wacom_device;

struct wacom_device {
	wacom_device*		next;

	int					open;
	int					number;

	usb_device			dev;

	usb_pipe			pipe;
	char*				data;
	size_t				max_packet_size;
	size_t				length;

	uint16				vendor;
	uint16				product;

	sem_id				notify_lock;
	uint32				status;
};

// handy strings for referring to ourself
#define ID "wacom: "
static const char* kDriverName = "wacom";
static const char* kBasePublishPath = "input/wacom/usb/";

// list of device instances and names for publishing
static wacom_device* sDeviceList = NULL;
static sem_id sDeviceListLock = -1;
static int sDeviceCount = 0;

static char** sDeviceNames = NULL;

// handle for the USB bus manager
static usb_module_info* usb;

// These rather inelegant routines are used to assign numbers to
// device instances so that they have unique names in devfs.

static uint32 sDeviceNumbers = 0;

// get_number
static int
get_number()
{
	int num;

	for (num = 0; num < 32; num++) {
		if (!(sDeviceNumbers & (1 << num))) {
			sDeviceNumbers |= (1 << num);
			return num;
		}
	}

	return -1;
}

// put_number
static void
put_number(int num)
{
	sDeviceNumbers &= ~(1 << num);
}

// #pragma mark - Device addition and removal
//
// add_device() and remove_device() are used to create and tear down
// device instances.  They are used from the callbacks device_added()
// and device_removed() which are invoked by the USB bus manager.

// add_device
static wacom_device*
add_device(usb_device dev)
{
	wacom_device *device = NULL;
	int num, ifc, alt;
	const usb_interface_info *ii;
	status_t st;
	const usb_device_descriptor* udd;
	const usb_configuration_info *conf;
	bool setConfiguration = false;

	// we need these four for a Wacom tablet
	size_t controlTransferLength;
	int tryCount;
	char repData[2] = { 0x02, 0x02 };
	char retData[2] = { 0x00, 0x00 };

	conf = usb->get_configuration(dev);
	DPRINTF_INFO((ID "add_device(%ld, %p)\n", dev, conf));

	if ((num = get_number()) < 0)
		return NULL;

	udd = usb->get_device_descriptor(dev);
	// only pick up wacom tablets
	if (udd && udd->vendor_id == 0x056a) {

		DPRINTF_ERR((ID "add_device() - wacom detected\n"));

		// see if the device has been configured already
		if (!conf) {
			conf = usb->get_nth_configuration(dev, 0);
			setConfiguration = true;
		}

		if (!conf)
			goto fail;

		for (ifc = 0; ifc < conf->interface_count; ifc++) {
			DPRINTF_INFO((ID "add_device() - examining interface: %d\n", ifc));
			for (alt = 0; alt < conf->interface[ifc].alt_count; alt++) {
				ii = &conf->interface[ifc].alt[alt];
				DPRINTF_INFO((ID "add_device() - examining alt interface: "
					"%d\n", alt));


				// does it have the correct type of interface?
				if (ii->descr->interface_class != 3) continue;
				if (ii->descr->interface_subclass != 1) continue;
				if (ii->endpoint_count != 1) continue;

				// only accept input endpoints
				if (ii->endpoint[0].descr->endpoint_address & 0x80) {
					DPRINTF_INFO((ID "add_device() - found input endpoint\n"));
					goto got_one;
				}
			}
		}
	} else
		goto fail;

fail:
	put_number(num);
	if (device) {
		free(device->data);
		free(device);
	}
	return NULL;

got_one:
	if ((device = (wacom_device *) malloc(sizeof(wacom_device))) == NULL)
		goto fail;

	device->dev = dev;
	device->number = num;
	device->open = 1;
	device->notify_lock = -1;
	device->data = NULL;

//	if (setConfiguration) {
		// the configuration has to be set yet (was not the current one)
		DPRINTF_INFO((ID "add_device() - setting configuration...\n"));
		if ((st = usb->set_configuration(dev, conf)) != B_OK) {
			dprintf(ID "add_device() -> "
				"set_configuration() returns %" B_PRId32 "\n", st);
			goto fail;
		} else
			DPRINTF_ERR((ID " ... success!\n"));

		if (conf->interface[ifc].active != ii) {
			// the interface we found is not the active one and has to be set
			DPRINTF_INFO((ID "add_device() - setting interface: %p...\n", ii));
			if ((st = usb->set_alt_interface(dev, ii)) != B_OK) {
				dprintf(ID "add_device() -> "
					"set_alt_interface() returns %" B_PRId32 "\n", st);
				goto fail;
			} else
				DPRINTF_ERR((ID " ... success!\n"));
		}
		// see if the device is a Wacom tablet and needs some special treatment
		// let's hope Wacom doesn't produce normal mice any time soon, or this
		// check will have to be more specific about product_id...hehe
		if (udd->vendor_id == 0x056a) {
			// do the control transfers to set up absolute mode (default is HID
			// mode)

			// see 'Device Class Definition for HID 1.11' (HID1_11.pdf),
			// par. 7.2 (available at www.usb.org/developers/hidpage)

			// set protocol mode to 'report' (instead of 'boot')
			controlTransferLength = 0;
			// HID Class-Specific Request, Host to device (=0x21):
			// SET_PROTOCOL (=0x0b) to Report Protocol (=1)
			// of Interface #0 (=0)
			st = usb->send_request(dev, 0x21, 0x0b, 1, 0, 0, 0,
				&controlTransferLength);

			if (st < B_OK) {
				dprintf(ID "add_device() - "
					"control transfer 1 failed: %" B_PRId32 "\n", st);
			}

			// try up to five times to set the tablet to 'Wacom'-mode (enabling
			// absolute mode, pressure data, etc.)
			controlTransferLength = 2;

			for (tryCount = 0; tryCount < 5; tryCount++) {
				// HID Class-Specific Request, Host to device (=0x21):
				// SET_REPORT (=0x09) type Feature (=3) with ID 2 (=2) of
				// Interface #0 (=0) to repData (== { 0x02, 0x02 })
				st = usb->send_request(dev, 0x21, 0x09, (3 << 8) + 2, 0, 2,
					repData, &controlTransferLength);

				if (st < B_OK) {
					dprintf(ID "add_device() - "
						"control transfer 2 failed: %" B_PRId32 "\n", st);
				}

				// check if registers are set correctly

				// HID Class-Specific Request, Device to host (=0xA1):
				// GET_REPORT (=0x01) type Feature (=3) with ID 2 (=2) of
				// Interface #0 (=0) to retData
				st = usb->send_request(dev, 0xA1, 0x01, (3 << 8) + 2, 0, 2,
					retData, &controlTransferLength);

				if (st < B_OK) {
					dprintf(ID "add_device() - "
						"control transfer 3 failed: %" B_PRId32 "\n", st);
				}

				DPRINTF_INFO((ID "add_device() - retData: %u - %u\n",
					retData[0], retData[1]));

				if (retData[0] == repData[0] && retData[1] == repData[1]) {
					DPRINTF_INFO((ID "add_device() - successfully set "
						"'Wacom'-mode\n"));
					break;
				}
			}

			DPRINTF_INFO((ID "add_device() - number of tries: %u\n",
				tryCount + 1));

			if (tryCount > 4) {
				dprintf(ID "add_device() - set 'Wacom'-mode failed\n");
			}
		}
//	}

	// configure the rest of the wacom_device
	device->pipe = ii->endpoint[0].handle;
//DPRINTF_INFO((ID "add_device() - pipe id = %ld\n", device->pipe));
	device->length = 0;
	device->max_packet_size = ii->endpoint[0].descr->max_packet_size;
	device->data = (char*)malloc(device->max_packet_size);
	if (device->data == NULL)
		goto fail;
//DPRINTF_INFO((ID "add_device() - max packet length = %ld\n",
//	device->max_packet_size));
	device->status = 0;//B_USB_STATUS_SUCCESS;
	device->vendor = udd->vendor_id;
	device->product = udd->product_id;

	DPRINTF_INFO((ID "add_device() - added %p (/dev/%s%d)\n", device,
		kBasePublishPath, num));

	// add it to the list of devices so it will be published, etc
	acquire_sem(sDeviceListLock);
	device->next = sDeviceList;
	sDeviceList = device;
	sDeviceCount++;
	release_sem(sDeviceListLock);

	return device;
}

// remove_device
static void
remove_device(wacom_device *device)
{
	put_number(device->number);

	usb->cancel_queued_transfers(device->pipe);

	delete_sem(device->notify_lock);
	if (device->data)
		free(device->data);
	free(device);
}

// device_added
static status_t
device_added(usb_device dev, void** cookie)
{
	wacom_device* device;

	DPRINTF_INFO((ID "device_added(%ld,...)\n", dev));

	// first see, if this device is already added
	acquire_sem(sDeviceListLock);
	for (device = sDeviceList; device; device = device->next) {
		DPRINTF_ERR((ID "device_added() - old device: %" B_PRIu32 "\n",
			device->dev));
		if (device->dev == dev) {
			DPRINTF_ERR((ID "device_added() - already added - done!\n"));
			*cookie = (void*)device;
			release_sem(sDeviceListLock);
			return B_OK;
		}
	}
	release_sem(sDeviceListLock);

	if ((device = add_device(dev)) != NULL) {
		*cookie = (void*)device;
		DPRINTF_INFO((ID "device_added() - done!\n"));
		return B_OK;
	} else
		DPRINTF_INFO((ID "device_added() - failed to add device!\n"));

	return B_ERROR;
}

// device_removed
static status_t
device_removed(void *cookie)
{
	wacom_device *device = (wacom_device *) cookie;

	DPRINTF_INFO((ID "device_removed(%p)\n", device));

	if (device) {

		acquire_sem(sDeviceListLock);

		// remove it from the list of devices
		if (device == sDeviceList) {
			sDeviceList = device->next;
		} else {
			wacom_device *n;
			for (n = sDeviceList; n; n = n->next) {
				if (n->next == device) {
					n->next = device->next;
					break;
				}
			}
		}
		sDeviceCount--;

		// tear it down if it's not open --
		// otherwise the last device_free() will handle it

		device->open--;

		DPRINTF_ERR((ID "device_removed() open: %d\n", device->open));

		if (device->open == 0) {
			remove_device(device);
		} else {
			dprintf(ID "device /dev/%s%d still open -- marked for removal\n",
				kBasePublishPath, device->number);
		}

		release_sem(sDeviceListLock);
	}

	return B_OK;
}

// #pragma mark - Device Hooks
//
// Here we implement the posixy driver hooks (open/close/read/write/ioctl)

// device_open
static status_t
device_open(const char *dname, uint32 flags, void** cookie)
{
	wacom_device *device;
	int n;
	status_t ret = B_ERROR;

	n = atoi(dname + strlen(kBasePublishPath));

	DPRINTF_INFO((ID "device_open(\"%s\",%d,...)\n", dname, flags));

	acquire_sem(sDeviceListLock);
	for (device = sDeviceList; device; device = device->next) {
		if (device->number == n) {
//			if (device->open <= 1) {
				device->open++;
				*cookie = device;
				DPRINTF_ERR((ID "device_open() open: %d\n", device->open));

				if (device->notify_lock < 0) {
					device->notify_lock = create_sem(0, "notify_lock");
					if (device->notify_lock < 0) {
						ret = device->notify_lock;
						device->open--;
						*cookie = NULL;
						dprintf(ID "device_open() -> "
							"create_sem() returns %" B_PRId32 "\n", ret);
					} else {
						ret = B_OK;
					}
				}
				release_sem(sDeviceListLock);
				return ret;
//			} else {
//				dprintf(ID "device_open() -> device is already open %ld\n",
//					ret);
//				release_sem(sDeviceListLock);
//				return B_ERROR;
//			}
		}
	}
	release_sem(sDeviceListLock);
	return ret;
}

// device_close
static status_t
device_close (void *cookie)
{
#if DEBUG_DRIVER
	wacom_device *device = (wacom_device*) cookie;
	DPRINTF_ERR((ID "device_close() name = \"%s%d\"\n", kBasePublishPath,
		device->number));
#endif
	return B_OK;
}

// device_free
static status_t
device_free(void *cookie)
{
	wacom_device *device = (wacom_device *)cookie;

	DPRINTF_INFO((ID "device_free() name = \"%s%d\"\n", kBasePublishPath,
		device->number));

	acquire_sem(sDeviceListLock);

	device->open--;

	DPRINTF_INFO((ID "device_free() open: %ld\n", device->open));

	if (device->open == 0) {
		remove_device(device);
	}
	release_sem(sDeviceListLock);

	return B_OK;
}

// device_interupt_callback
static void
device_interupt_callback(void* cookie, status_t status, void* data,
	uint32 actualLength)
{
	wacom_device* device = (wacom_device*)cookie;
	uint32 length = min_c(actualLength, device->max_packet_size);

	DPRINTF_INFO((ID "device_interupt_callback(%p) name = \"%s%d\" -> "
		"status: %ld, length: %ld\n", cookie, kBasePublishPath, device->number,
		status, actualLength));

	device->status = status;
	if (device->notify_lock >= 0) {
		if (status == B_OK) {
			memcpy(device->data, data, length);
			device->length = length;
		} else {
			device->length = 0;
		}
		release_sem(device->notify_lock);
	}

	DPRINTF_INFO((ID "device_interupt_callback() - done\n"));
}

// read_header
static status_t
read_header(const wacom_device* device, void* buffer)
{
	wacom_device_header device_header;
	device_header.vendor_id = device->vendor;
	device_header.product_id = device->product;
	device_header.max_packet_size = device->max_packet_size;

	if (!IS_USER_ADDRESS(buffer)) {
		memcpy(buffer, &device_header, sizeof(wacom_device_header));
		return B_OK;
	}

	return user_memcpy(buffer, &device_header, sizeof(wacom_device_header));
}

// device_read
static status_t
device_read(void* cookie, off_t pos, void* buf, size_t* count)
{
	wacom_device* device = (wacom_device*) cookie;
	status_t ret = B_BAD_VALUE;
	uint8* buffer = (uint8*)buf;
	uint32 dataLength;

	if (!device)
		return ret;

	ret = device->notify_lock;

	DPRINTF_INFO((ID "device_read(%p,%Ld,0x%x,%d) name = \"%s%d\"\n",
			 cookie, pos, buf, *count, kBasePublishPath, device->number));

	if (ret >= B_OK) {
		// what the client "reads" is decided depending on how much bytes are
		// provided. "sizeof(wacom_device_header)" bytes are needed to "read"
		// vendor id, product id and max packet size in case the client wants to
		// read more than "sizeof(wacom_device_header)" bytes, a usb interupt
		// transfer is scheduled, and an error report is returned as appropriate
		if (*count > sizeof(wacom_device_header)) {
			// queue the interrupt transfer
			ret = usb->queue_interrupt(device->pipe, device->data,
				device->max_packet_size, device_interupt_callback, device);
			if (ret >= B_OK) {
				// we will block here until the interrupt transfer has been done
				ret = acquire_sem_etc(device->notify_lock, 1,
					B_RELATIVE_TIMEOUT, 500 * 1000);
				// handle time out
				if (ret < B_OK) {
					usb->cancel_queued_transfers(device->pipe);
					acquire_sem(device->notify_lock);
						// collect the sem released by the cancel

					if (ret == B_TIMED_OUT) {
						// a time_out is ok, since it only means that the device
						// had nothing to report (ie mouse/pen was not moved)
						// within the given time interval
						DPRINTF_INFO((ID "device_read(%p) name = \"%s%d\" -> "
							"B_TIMED_OUT\n", cookie, kBasePublishPath,
							device->number));
						*count = sizeof(wacom_device_header);
						ret = read_header(device, buffer);
					} else {
						// any other error trying to acquire the semaphore
						*count = 0;
					}
				} else {
					if (device->status == 0/*B_USBD_SUCCESS*/) {
						DPRINTF_INFO((ID "interrupt transfer - success\n"));
						// copy the data from the buffer
						dataLength = min_c(device->length,
							*count - sizeof(wacom_device_header));
						*count = dataLength + sizeof(wacom_device_header);
						ret = read_header(device, buffer);
						if (ret == B_OK) {
							if (IS_USER_ADDRESS(buffer))
								ret = user_memcpy(
									buffer + sizeof(wacom_device_header),
									device->data, dataLength);
							else
								memcpy(buffer + sizeof(wacom_device_header),
									device->data, dataLength);
						}
					} else {
						// an error happened during the interrupt transfer
						*count = 0;
						dprintf(ID "interrupt transfer - "
							"failure: %" B_PRIu32 "\n", device->status);
						ret = B_ERROR;
					}
				}
			} else {
				*count = 0;
				dprintf(ID "device_read(%p) name = \"%s%d\" -> error queuing "
					"interrupt: %" B_PRId32 "\n", cookie, kBasePublishPath,
					device->number, ret);
			}
		} else if (*count == sizeof(wacom_device_header)) {
			ret = read_header(device, buffer);
		} else {
			dprintf(ID "device_read(%p) name = \"%s%d\" -> buffer size must be "
				"at least the size of the wacom_device_header struct!\n",
				cookie, kBasePublishPath, device->number);
			*count = 0;
			ret = B_BAD_VALUE;
		}
	}

	return ret;
}

// device_write
static status_t
device_write(void *cookie, off_t pos, const void *buf, size_t *count)
{
	return B_ERROR;
}

// device_control
static status_t
device_control (void *cookie, uint32 msg, void *arg1, size_t len)
{
	return B_ERROR;
}

// #pragma mark - Driver Hooks
//
// These functions provide the glue used by DevFS to load/unload
// the driver and also handle registering with the USB bus manager
// to receive device added and removed events

static usb_notify_hooks notify_hooks =
{
	&device_added,
	&device_removed
};

static const usb_support_descriptor kSupportedDevices[] =
{
	{ 3, 1, 2, 0, 0 }
};

// init_hardware
status_t
init_hardware(void)
{
	return B_OK;
}

// init_driver
status_t
init_driver(void)
{
	DPRINTF_INFO((ID "init_driver(), built %s %s\n", __DATE__, __TIME__));

#if DEBUG_DRIVER && !defined(__HAIKU__)
	if (load_driver_symbols(kDriverName) == B_OK) {
		DPRINTF_INFO((ID "loaded symbols\n"));
	} else {
		DPRINTF_ERR((ID "no driver symbols loaded!\n"));
	}
#endif

	if (get_module(B_USB_MODULE_NAME, (module_info**) &usb) != B_OK) {
		DPRINTF_ERR((ID "cannot get module \"%s\"\n", B_USB_MODULE_NAME));
		return B_ERROR;
	}

	if ((sDeviceListLock = create_sem(1,"sDeviceListLock")) < 0) {
		put_module(B_USB_MODULE_NAME);
		return sDeviceListLock;
	}

	usb->register_driver(kDriverName, kSupportedDevices, 1, NULL);
	usb->install_notify(kDriverName, &notify_hooks);

	return B_OK;
}

// uninit_driver
void
uninit_driver(void)
{
	int i;

	DPRINTF_INFO((ID "uninit_driver()\n"));

	usb->uninstall_notify(kDriverName);

	delete_sem(sDeviceListLock);

	put_module(B_USB_MODULE_NAME);

	if (sDeviceNames) {
		for (i = 0; sDeviceNames[i]; i++)
			free(sDeviceNames[i]);
		free(sDeviceNames);
	}

	DPRINTF_INFO((ID "uninit_driver() done\n"));
}

// publish_devices
const char**
publish_devices()
{
	wacom_device *device;
	int i;

	DPRINTF_INFO((ID "publish_devices()\n"));

	if (sDeviceNames) {
		for (i = 0; sDeviceNames[i]; i++)
			free((char *) sDeviceNames[i]);
		free(sDeviceNames);
	}

	acquire_sem(sDeviceListLock);
	sDeviceNames = (char**)malloc(sizeof(char*) * (sDeviceCount + 2));
	if (sDeviceNames) {
		for (i = 0, device = sDeviceList; device; device = device->next) {
			sDeviceNames[i] = (char*)malloc(strlen(kBasePublishPath) + 4);
			if (sDeviceNames[i]) {
				sprintf(sDeviceNames[i],"%s%d",kBasePublishPath,device->number);
				DPRINTF_INFO((ID "publishing: \"/dev/%s\"\n",sDeviceNames[i]));
				i++;
			}
		}
		sDeviceNames[i] = NULL;
	}

	release_sem(sDeviceListLock);

	return (const char**)sDeviceNames;
}

static device_hooks sDeviceHooks = {
	device_open,
	device_close,
	device_free,
	device_control,
	device_read,
	device_write
};

// find_device
device_hooks*
find_device(const char* name)
{
	return &sDeviceHooks;
}
