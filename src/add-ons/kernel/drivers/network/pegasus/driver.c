/*
 * Pegasus BeOS Driver
 * 
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
 
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver.h"
#include "usbdevs.h"

typedef struct driver_cookie {
	struct driver_cookie	*next;
	pegasus_dev			*device;
} driver_cookie;

int32 api_version = B_CUR_DRIVER_API_VERSION;

static status_t pegasus_device_added(const usb_device dev, void **cookie);
static status_t pegasus_device_removed(void *cookie);

static int sDeviceNumber = 0;
static const char *kBaseName = "net/pegasus/";

static const char *kDriverName = DRIVER_NAME;

usb_module_info *usb;


usb_support_descriptor supported_devices[] = {
 { 0, 0, 0, USB_VENDOR_3COM,		USB_PRODUCT_3COM_3C460B},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX1},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX2},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_UFE1000},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX4},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX5},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX6},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX7},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX8},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX9},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX10},
 { 0, 0, 0, USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_DSB650TX_PNA},
 { 0, 0, 0, USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_USB320_EC},
 { 0, 0, 0, USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_SS1001},
 { 0, 0, 0, USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUS},
 { 0, 0, 0, USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII},
 { 0, 0, 0, USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_2},
 { 0, 0, 0, USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_3},
 { 0, 0, 0, USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_USB2LAN},
 { 0, 0, 0, USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USB100},
 { 0, 0, 0, USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBLP100},
 { 0, 0, 0, USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBEL100},
 { 0, 0, 0, USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBE100},
 { 0, 0, 0, USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TX},
 { 0, 0, 0, USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TXS},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX4},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX1},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX_PNA},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX3},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX2},
 { 0, 0, 0, USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650},
 { 0, 0, 0, USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX0},
 { 0, 0, 0, USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX1},
 { 0, 0, 0, USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX2},
 { 0, 0, 0, USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX3},
 { 0, 0, 0, USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBLTX},
 { 0, 0, 0, USB_VENDOR_ELSA,		USB_PRODUCT_ELSA_USB2ETHERNET},
 { 0, 0, 0, USB_VENDOR_HAWKING,		USB_PRODUCT_HAWKING_UF100},
 { 0, 0, 0, USB_VENDOR_HP,		USB_PRODUCT_HP_HN210E},
 { 0, 0, 0, USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTX},
 { 0, 0, 0, USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTXS},
 { 0, 0, 0, USB_VENDOR_KINGSTON,	USB_PRODUCT_KINGSTON_KNU101TX},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX1},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10T},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100TX},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100H1},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TA},
 { 0, 0, 0, USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX2},
 { 0, 0, 0, USB_VENDOR_MICROSOFT,	USB_PRODUCT_MICROSOFT_MN110},
 { 0, 0, 0, USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX1},
 { 0, 0, 0, USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX5},
 { 0, 0, 0, USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUA2TX5},
 { 0, 0, 0, USB_VENDOR_NETGEAR,		USB_PRODUCT_NETGEAR_FA101},
 { 0, 0, 0, USB_VENDOR_SIEMENS,		USB_PRODUCT_SIEMENS_SPEEDSTREAM},
 { 0, 0, 0, USB_VENDOR_SMARTBRIDGES,	USB_PRODUCT_SMARTBRIDGES_SMARTNIC},
 { 0, 0, 0, USB_VENDOR_SMC,		USB_PRODUCT_SMC_2202USB},
 { 0, 0, 0, USB_VENDOR_SMC,		USB_PRODUCT_SMC_2206USB},
 { 0, 0, 0, USB_VENDOR_SOHOWARE,	USB_PRODUCT_SOHOWARE_NUB100},
};


static const struct aue_type aue_devs[] = {
 {{ USB_VENDOR_3COM,		USB_PRODUCT_3COM_3C460B},		 PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX1},	  PNA|PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX2},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_UFE1000},	  LSYS },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX4},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX5},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX6},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX7},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX8},	  PII },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX9},	  PNA },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_XX10},	  0 },
 {{ USB_VENDOR_ABOCOM,		USB_PRODUCT_ABOCOM_DSB650TX_PNA}, 0 },
 {{ USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_USB320_EC},	  0 },
 {{ USB_VENDOR_ACCTON,		USB_PRODUCT_ACCTON_SS1001},	  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUS},	  PNA },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII},	  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_2},  PII },
 {{ USB_VENDOR_ADMTEK,		USB_PRODUCT_ADMTEK_PEGASUSII_3},  PII },
 {{ USB_VENDOR_BELKIN,		USB_PRODUCT_BELKIN_USB2LAN},	  PII },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USB100},	  0 },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBLP100}, PNA },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBEL100}, 0 },
 {{ USB_VENDOR_BILLIONTON,	USB_PRODUCT_BILLIONTON_USBE100},  PII },
 {{ USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TX}, 0 },
 {{ USB_VENDOR_COREGA,		USB_PRODUCT_COREGA_FETHER_USB_TXS},PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX4},	  LSYS|PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX1},	  LSYS },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX},	  LSYS },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX_PNA},  PNA },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX3},	  LSYS|PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650TX2},	  LSYS|PII },
 {{ USB_VENDOR_DLINK,		USB_PRODUCT_DLINK_DSB650},	  LSYS },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX0},	  0 },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX1},	  LSYS },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX2},	  0 },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBTX3},	  LSYS },
 {{ USB_VENDOR_ELECOM,		USB_PRODUCT_ELECOM_LDUSBLTX},	  PII },
 {{ USB_VENDOR_ELSA,		USB_PRODUCT_ELSA_USB2ETHERNET},	  0 },
 {{ USB_VENDOR_HAWKING,		USB_PRODUCT_HAWKING_UF100},	   PII },
 {{ USB_VENDOR_HP,			USB_PRODUCT_HP_HN210E},				PII },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_ETXUS2},		PII },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTX},	  0 },
 {{ USB_VENDOR_IODATA,		USB_PRODUCT_IODATA_USBETTXS},	  PII },
 {{ USB_VENDOR_KINGSTON,	USB_PRODUCT_KINGSTON_KNU101TX},   0 },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX1},	  LSYS|PII },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10T},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100TX},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB100H1},	  LSYS|PNA },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TA},	  LSYS },
 {{ USB_VENDOR_LINKSYS,		USB_PRODUCT_LINKSYS_USB10TX2},	  LSYS|PII },
 {{ USB_VENDOR_MICROSOFT,	USB_PRODUCT_MICROSOFT_MN110},	  PII },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX1},	  0 },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUATX5},	  0 },
 {{ USB_VENDOR_MELCO,		USB_PRODUCT_MELCO_LUA2TX5},		PII },
 {{ USB_VENDOR_NETGEAR,		USB_PRODUCT_NETGEAR_FA101},		PII },
 {{ USB_VENDOR_SIEMENS,		USB_PRODUCT_SIEMENS_SPEEDSTREAM}, PII },
 {{ USB_VENDOR_SMARTBRIDGES,	USB_PRODUCT_SMARTBRIDGES_SMARTNIC},PII },
 {{ USB_VENDOR_SMC,			USB_PRODUCT_SMC_2202USB},		0 },
 {{ USB_VENDOR_SMC,			USB_PRODUCT_SMC_2206USB},		PII },
 {{ USB_VENDOR_SOHOWARE,	USB_PRODUCT_SOHOWARE_NUB100},	0 },
};


static status_t
pegasus_checkdeviceinfo(pegasus_dev *dev)
{
	if (!dev || dev->cookieMagic != PEGASUS_COOKIE_MAGIC)
		return EINVAL;

	return B_OK;
}


pegasus_dev *
create_device(const usb_device dev, const usb_interface_info *ii, uint16 ifno)
{
	pegasus_dev *device = NULL;
	sem_id sem;
	
	ASSERT(usb != NULL && dev != 0);

	device = malloc(sizeof(pegasus_dev));
	if (device == NULL)
		return NULL;
		
	memset(device, 0, sizeof(pegasus_dev));

	device->sem_lock = sem = create_sem(1, DRIVER_NAME "_lock");
	if (sem < B_OK) {
		DPRINTF_ERR("create_sem() failed 0x%" B_PRIx32 "\n", sem);
		free(device);
		return NULL;
	}
	
	device->rx_sem = sem = create_sem(1, DRIVER_NAME"_receive");
	if (sem < B_OK) {
		DPRINTF_ERR("create_sem() failed 0x%" B_PRIx32 "\n", sem);
		delete_sem(device->sem_lock);
		free(device);
		return NULL;
	}
	set_sem_owner(device->rx_sem, B_SYSTEM_TEAM);

	device->rx_sem_cb = sem = create_sem(0, DRIVER_NAME"_receive_cb");
	if (sem < B_OK) {
		DPRINTF_ERR("create_sem() failed 0x%" B_PRIx32 "\n", sem);
		delete_sem(device->rx_sem);
		delete_sem(device->sem_lock);
		free(device);
		return NULL;
	}
	set_sem_owner(device->rx_sem_cb, B_SYSTEM_TEAM);

	device->tx_sem = sem = create_sem(1, DRIVER_NAME"_transmit");
	if (sem < B_OK) {
		delete_sem(device->sem_lock);
		delete_sem(device->rx_sem);
		delete_sem(device->rx_sem_cb);
		free(device);
		return NULL;
	}
	set_sem_owner(device->tx_sem, B_SYSTEM_TEAM);
	
	device->tx_sem_cb = sem = create_sem(0, DRIVER_NAME"_transmit_cb");
	if (sem < B_OK) {
		delete_sem(device->sem_lock);
		delete_sem(device->rx_sem);
		delete_sem(device->rx_sem_cb);
		delete_sem(device->tx_sem);
		free(device);
		return NULL;
	}
	set_sem_owner(device->tx_sem_cb, B_SYSTEM_TEAM);
		
	device->number = sDeviceNumber++;
	device->cookieMagic = PEGASUS_COOKIE_MAGIC;
	sprintf(device->name, "%s%d", kBaseName, device->number);
	device->dev = dev;
	device->ifno = ifno;
	device->open = 0;
	device->open_fds = NULL;
	device->aue_dying = false;
	device->flags = 0;
	device->maxframesize = 1514; // XXX is MAXIMUM_ETHERNET_FRAME_SIZE = 1518 too much?
		
	return device;
}


void
remove_device(pegasus_dev *device)
{
	ASSERT(device != NULL);

	delete_sem(device->rx_sem);
	delete_sem(device->tx_sem);
	delete_sem(device->rx_sem_cb);
	delete_sem(device->tx_sem_cb);
		
	delete_sem(device->sem_lock);

	free(device);
}


/**
	\fn:
*/
static status_t 
setup_endpoints(const usb_interface_info *uii, pegasus_dev *dev)
{
	size_t epts[3] = { -1, -1, -1 };
	size_t ep = 0;
	for(; ep < uii->endpoint_count; ep++){
		usb_endpoint_descriptor *ed = uii->endpoint[ep].descr;
		DPRINTF_INFO("try endpoint:%ld %x %x %lx\n", ep, ed->attributes,
			ed->endpoint_address, uii->endpoint[ep].handle);
		if ((ed->attributes & USB_ENDPOINT_ATTR_MASK) == USB_ENDPOINT_ATTR_BULK) {
			if ((ed->endpoint_address & USB_ENDPOINT_ADDR_DIR_IN)
				== USB_ENDPOINT_ADDR_DIR_IN) {
				epts[0]	= ep;
			} else
				epts[1] = ep;
		} else if ((ed->attributes & USB_ENDPOINT_ATTR_MASK)
				== USB_ENDPOINT_ATTR_INTERRUPT) {
				epts[2] = ep;
		}
	}
	
	dev->pipe_in = uii->endpoint[epts[0]].handle;
	dev->pipe_out = uii->endpoint[epts[1]].handle;
	dev->pipe_intr = uii->endpoint[epts[2]].handle;
	DPRINTF_INFO("endpoint:%lx %lx %lx\n", dev->pipe_in, dev->pipe_out, dev->pipe_intr);
	
	return ((epts[0] > -1) && (epts[1] > -1) && (epts[2] > -1)) ? B_OK : B_ENTRY_NOT_FOUND;
}


static void 
pegasus_rx_callback(void *cookie, status_t status, void *data, size_t actual_len)
{
	pegasus_dev *dev = (pegasus_dev *)cookie;

	DPRINTF_INFO("pegasus_rx_callback() %ld %ld\n", status, actual_len);
	if (status == B_CANCELED) {
		/* cancelled: device is unplugged */
		DPRINTF_ERR("pegasus_rx_callback() cancelled\n");
		return;
	}
	
	ASSERT(cookie != NULL);
	dev->rx_actual_length = actual_len;
	dev->rx_status = status;	/* B_USB_STATUS_* */
	release_sem(dev->rx_sem_cb);
	DPRINTF_INFO("pegasus_rx_callback release sem %ld\n", dev->rx_sem_cb);
}


static void 
pegasus_tx_callback(void *cookie, status_t status, void *data, size_t actual_len)
{
	pegasus_dev *dev = (pegasus_dev *)cookie;
	
	DPRINTF_INFO("pegasus_tx_callback() %ld %ld\n", status, actual_len);
	if (status == B_CANCELED) {
		/* cancelled: device is unplugged */
		DPRINTF_ERR("pegasus_tx_callback() cancelled\n");
		return;
	}
			
	ASSERT(cookie != NULL);
	dev->tx_actual_length = actual_len;
	dev->tx_status = status;	/* B_USB_STATUS_* */
	release_sem(dev->tx_sem_cb);	
	DPRINTF_INFO("pegasus_tx_callback release sem %ld\n", dev->tx_sem_cb);
}


//	#pragma mark - device hooks


static status_t
pegasus_device_added(const usb_device dev, void **cookie)
{
	pegasus_dev *device;
	const usb_device_descriptor *dev_desc;
	const usb_configuration_info *conf;
	const usb_interface_info *intf;
	status_t status;
	uint16 ifno;
	int i;
	
	ASSERT(dev != 0 && cookie != NULL);
	DPRINTF_INFO("device_added()\n");

	dev_desc = usb->get_device_descriptor(dev);

	DPRINTF_INFO("vendor ID 0x%04X, product ID 0x%04X\n", dev_desc->vendor_id,
		dev_desc->product_id);

	if ((conf = usb->get_nth_configuration(dev, DEFAULT_CONFIGURATION)) 
		== NULL) {
		DPRINTF_ERR("cannot get default configuration\n");
		return B_ERROR;
	}
	
	ifno = AUE_IFACE_IDX;
	intf = conf->interface [ifno].active;

	/* configuration */

	if ((status = usb->set_configuration(dev, conf)) != B_OK) {
		DPRINTF_ERR("set_configuration() failed %s\n", strerror(status));
		return B_ERROR;
	}
	
	if ((device = create_device(dev, intf, ifno)) == NULL) {
		DPRINTF_ERR("create_device() failed\n");
		return B_ERROR;
	}
	
	device->aue_vendor = dev_desc->vendor_id;
	device->aue_product = dev_desc->product_id;
	
	for (i=0; i < sizeof(aue_devs) / sizeof(struct aue_type); i++)
		if (aue_devs[i].aue_dev.vendor == dev_desc->vendor_id
			&& aue_devs[i].aue_dev.product == dev_desc->product_id) {
			device->aue_flags = aue_devs[i].aue_flags;		
			break;
		}
		
	/* Find endpoints. */
	setup_endpoints(intf, device);
	
	aue_attach(device);

	DPRINTF_INFO("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		device->macaddr[0], device->macaddr[1], device->macaddr[2],
		device->macaddr[3], device->macaddr[4], device->macaddr[5]);
	
	aue_init(device);

	/* create a port */
	add_device_info(device);
	
	*cookie = device;
	DPRINTF_INFO("added %s\n", device->name);
	return B_OK;
}


static status_t
pegasus_device_removed(void *cookie)
{
	pegasus_dev *device = cookie;

	ASSERT(cookie != NULL);

	DPRINTF_INFO("device_removed(%s)\n", device->name);

	aue_uninit(device);
	
	usb->cancel_queued_transfers(device->pipe_in);
	usb->cancel_queued_transfers(device->pipe_out);
	usb->cancel_queued_transfers(device->pipe_intr);
	remove_device_info(device);

	if (device->open == 0) {
		remove_device(device);
	} else {
		DPRINTF_INFO("%s still open\n", device->name);
		AUE_LOCK(device);
		device->aue_dying = true;
		AUE_UNLOCK(device);
	}

	return B_OK;
}


static status_t 
pegasus_device_open(const char *name, uint32 flags,
	driver_cookie **out_cookie)
{
	driver_cookie *cookie;
	pegasus_dev *device;
	status_t err;
	
	ASSERT(name != NULL);
	ASSERT(out_cookie != NULL);
	DPRINTF_INFO("open(%s)\n", name);

	if ((device = search_device_info(name)) == NULL)
		return B_ENTRY_NOT_FOUND;
	if ((cookie = malloc(sizeof(driver_cookie))) == NULL)
		return B_NO_MEMORY;

	if ((err = acquire_sem(device->sem_lock)) != B_OK) {
		free(cookie);
		return err;
	}
	device->nonblocking = (flags & O_NONBLOCK) ? true : false;
	
	cookie->device = device;
	cookie->next = device->open_fds;
	device->open_fds = cookie;
	device->open++;
	release_sem(device->sem_lock);
	
	*out_cookie = cookie;
	DPRINTF_INFO("device %s open (%d)\n", name, device->open);
	return B_OK;
}


static status_t 
pegasus_device_read(driver_cookie *cookie, off_t position, void *buffer, size_t *_length)
{
	pegasus_dev *dev;
	status_t status;
	int32 blockFlag;
	size_t size;
	
	DPRINTF_INFO("device %p read\n", cookie);

	if (pegasus_checkdeviceinfo(dev = cookie->device) != B_OK) {
		DPRINTF_ERR("EINVAL\n");
#ifndef __HAIKU__
		*_length = 0;
			// net_server work-around; it obviously doesn't care about error conditions
			// For Haiku, this can be removed
#endif
		return B_BAD_VALUE;
	}

	if (dev->aue_dying)
		return B_DEVICE_NOT_FOUND;		/* already unplugged */

	blockFlag = dev->nonblocking ? B_TIMEOUT : 0;

	// block until receive is available (if blocking is allowed)
	if ((status = acquire_sem_etc(dev->rx_sem, 1, B_CAN_INTERRUPT | blockFlag, 0)) != B_NO_ERROR) {
		DPRINTF_ERR("cannot acquire read sem: %" B_PRIx32 ", %s\n", status, strerror(status));
#ifndef __HAIKU__
		*_length = 0;
#endif
		return status;
	}

	// queue new request
	status = usb->queue_bulk(dev->pipe_in, dev->rx_buffer, MAX_FRAME_SIZE, &pegasus_rx_callback, dev);
	
	if (status != B_OK) {
		DPRINTF_ERR("queue_bulk:failed:%08" B_PRIx32 "\n", status);
		goto rx_done;
	}
	
	// block until data is available (if blocking is allowed)
	if ((status = acquire_sem_etc(dev->rx_sem_cb, 1, B_CAN_INTERRUPT | blockFlag, 0)) != B_NO_ERROR) {
		DPRINTF_ERR("cannot acquire read sem: %" B_PRIx32 ", %s\n", status, strerror(status));
#ifndef __HAIKU__
		*_length = 0;
#endif
		goto rx_done;
	}
	
	if (dev->rx_status != B_OK) {
		status = usb->clear_feature(dev->pipe_in, USB_FEATURE_ENDPOINT_HALT);
		if (status != B_OK)
			DPRINTF_ERR("clear_feature() error %s\n", strerror(status));
		goto rx_done;
	}
	
	// copy buffer
	size = dev->rx_actual_length;
	if (size > MAX_FRAME_SIZE || (size - 2) > *_length) {
		DPRINTF_ERR("ERROR read: bad frame size %ld\n", size);
		size = *_length;
	} else if (size < *_length)
		*_length = size - 2;

	memcpy(buffer, dev->rx_buffer, size);
	
	DPRINTF_INFO("read done %ld\n", *_length);

rx_done:
	release_sem(dev->rx_sem);
	return status;
}


static status_t 
pegasus_device_write(driver_cookie *cookie, off_t position,	const void *buffer, size_t *_length)
{
	pegasus_dev *dev;
	status_t status;
	uint16 frameSize;

	DPRINTF_INFO("device %p write %ld\n", cookie, *_length);

	if (pegasus_checkdeviceinfo(dev = cookie->device) != B_OK) {
		DPRINTF_ERR("EINVAL\n");
		return EINVAL;
	}

	if (dev->aue_dying)
		return B_DEVICE_NOT_FOUND;		/* already unplugged */
	
		// block until a free tx descriptor is available
	if ((status = acquire_sem_etc(dev->tx_sem, 1, B_TIMEOUT, ETHER_TRANSMIT_TIMEOUT)) < B_NO_ERROR) {
		DPRINTF_ERR("write: acquiring sem failed: %" B_PRIx32 ", %s\n", status, strerror(status));
		return status;
	}


	if (*_length > MAX_FRAME_SIZE)
		*_length = MAX_FRAME_SIZE;

	frameSize = *_length;
	
	/* Copy data to tx buffer */
	memcpy(dev->tx_buffer+2, buffer, frameSize);
	
	/*
	 * The ADMtek documentation says that the packet length is
	 * supposed to be specified in the first two bytes of the
	 * transfer, however it actually seems to ignore this info
	 * and base the frame size on the bulk transfer length.
	 */
	dev->tx_buffer[0] = (uint8)frameSize;
	dev->tx_buffer[1] = (uint8)(frameSize >> 8);
	
	// queue new request, bulk length is one more if size is a multiple of 64
	status = usb->queue_bulk(dev->pipe_out, dev->tx_buffer, ((frameSize + 2) & 0x3f) ? frameSize + 2 : frameSize + 3, 
		&pegasus_tx_callback, dev);
	
	if (status != B_OK){
		DPRINTF_ERR("queue_bulk:failed:%08" B_PRIx32 "\n", status);
		goto tx_done;
	}	
	
	// block until data is sent (if blocking is allowed)
	if ((status = acquire_sem_etc(dev->tx_sem_cb, 1, B_CAN_INTERRUPT, 0)) != B_NO_ERROR) {
		DPRINTF_ERR("cannot acquire write done sem: %" B_PRIx32 ", %s\n", status, strerror(status));
#ifndef __HAIKU__
		*_length = 0;
#endif
		goto tx_done;
	}

	if (dev->tx_status != B_OK) {
		status = usb->clear_feature(dev->pipe_out, USB_FEATURE_ENDPOINT_HALT);
		if (status != B_OK)
			DPRINTF_ERR("clear_feature() error %s\n", strerror(status));
		goto tx_done;
	}
	
	*_length = frameSize;

tx_done:
	release_sem(dev->tx_sem);
	return status;
}


static status_t 
pegasus_device_control(driver_cookie *cookie, uint32 op,
		void *arg, size_t len)
{
	status_t err = B_ERROR;
	pegasus_dev *device;

	ASSERT(cookie != NULL);
	device = cookie->device;
	ASSERT(device != NULL);
	DPRINTF_INFO("ioctl(0x%x)\n", (int)op);

	if (device->aue_dying)
		return B_DEVICE_NOT_FOUND;		/* already unplugged */

	switch (op) {
		case ETHER_INIT:
			DPRINTF_INFO("control() ETHER_INIT\n");
			return B_OK;
		
		case ETHER_GETADDR:
			DPRINTF_INFO("control() ETHER_GETADDR\n");
			memcpy(arg, &device->macaddr, sizeof(device->macaddr));
			return B_OK;
			
		case ETHER_NONBLOCK:
			if (*(int32 *)arg) {
				DPRINTF_INFO("non blocking mode on\n");
				device->nonblocking = true;
			} else {
				DPRINTF_INFO("non blocking mode off\n");
				device->nonblocking = false;
			}
			return B_OK;

		case ETHER_ADDMULTI:
			DPRINTF_INFO("control() ETHER_ADDMULTI\n");
			break;
		
		case ETHER_REMMULTI:
			DPRINTF_INFO("control() ETHER_REMMULTI\n");
			return B_OK;
		
		case ETHER_SETPROMISC:
			if (*(int32 *)arg) {
				DPRINTF_INFO("control() ETHER_SETPROMISC on\n");
			} else {
				DPRINTF_INFO("control() ETHER_SETPROMISC off\n");
			}
			return B_OK;

		case ETHER_GETFRAMESIZE:
			DPRINTF_INFO("control() ETHER_GETFRAMESIZE, framesize = %ld (MTU = %ld)\n", device->maxframesize,  device->maxframesize - ENET_HEADER_SIZE);
			*(uint32*)arg = device->maxframesize;
			return B_OK;
			
		default:
			DPRINTF_INFO("control() Invalid command\n");
			break;
	}
	
	return err;
}


static status_t 
pegasus_device_close(driver_cookie *cookie)
{
	pegasus_dev *device;

	ASSERT(cookie != NULL && cookie->device != NULL);
	device = cookie->device;
	DPRINTF_INFO("close(%s)\n", device->name);

	/* detach the cookie from list */

	acquire_sem(device->sem_lock);
	if (device->open_fds == cookie)
		device->open_fds = cookie->next;
	else {
		driver_cookie *p;
		for (p = device->open_fds; p != NULL; p = p->next) {
			if (p->next == cookie) {
				p->next = cookie->next;
				break;
			}
		}
	}
	--device->open;
	release_sem(device->sem_lock);

	return B_OK;
}


static status_t 
pegasus_device_free(driver_cookie *cookie)
{
	pegasus_dev *device;

	ASSERT(cookie != NULL && cookie->device != NULL);
	device = cookie->device;
	DPRINTF_INFO("free(%s)\n", device->name);

	free(cookie);
	if (device->open > 0)
		DPRINTF_INFO("%d opens left\n", device->open);
	else if (device->aue_dying) {
		DPRINTF_INFO("removed %s\n", device->name);
		remove_device(device);
	}

	return B_OK;
}



/* Driver Hooks ---------------------------------------------------------
**
** These functions provide the glue used by DevFS to load/unload
** the driver and also handle registering with the USB bus manager
** to receive device added and removed events
*/

static usb_notify_hooks notify_hooks = 
{
	&pegasus_device_added,
	&pegasus_device_removed
};


status_t 
init_hardware(void)
{
	return B_OK;
}


status_t
init_driver(void)
{
	DPRINTF_INFO("init_driver(), built %s %s\n", __DATE__, __TIME__);
	
#if DEBUG_DRIVER
	if (load_driver_symbols(drivername) == B_OK) {
		DPRINTF_INFO("loaded symbols\n");
	} else {
		DPRINTF_INFO("no symbols for you!\n");
	}
#endif
			
	if (get_module(B_USB_MODULE_NAME, (module_info**) &usb) != B_OK) {
		DPRINTF_INFO("cannot get module \"%s\"\n", B_USB_MODULE_NAME);
		return B_ERROR;
	} 
	
	if ((gDeviceListLock = create_sem(1, "dev_list_lock")) < 0) {
		put_module(B_USB_MODULE_NAME);
		return gDeviceListLock;
	}
	
	usb->register_driver(kDriverName, supported_devices, 
		sizeof(supported_devices)/sizeof(usb_support_descriptor), NULL);
	usb->install_notify(kDriverName, &notify_hooks);
	
	return B_OK;
}


void
uninit_driver(void)
{
	DPRINTF_INFO("uninit_driver()\n");
	
	usb->uninstall_notify(kDriverName);
	delete_sem(gDeviceListLock);
	put_module(B_USB_MODULE_NAME);
	free_device_names();
}


const char**
publish_devices(void)
{
	if (gDeviceListChanged) {
		free_device_names();
		alloc_device_names();
		if (gDeviceNames != NULL)
			rebuild_device_names();
		gDeviceListChanged = false;
	}
	ASSERT(gDeviceNames != NULL);
	return (const char **) gDeviceNames;
}


device_hooks*
find_device(const char* name)
{
	static device_hooks hooks = {
		(device_open_hook)pegasus_device_open,
		(device_close_hook)pegasus_device_close,
		(device_free_hook)pegasus_device_free,
		(device_control_hook)pegasus_device_control,
		(device_read_hook)pegasus_device_read,
		(device_write_hook)pegasus_device_write,
		NULL
	};
	
	if (search_device_info(name) != NULL)
		return &hooks;
	return NULL;
}
