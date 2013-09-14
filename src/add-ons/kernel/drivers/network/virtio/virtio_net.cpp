/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include <ethernet.h>
#include <virtio.h>

#include <net/if_media.h>
#include <new>

#include "ether_driver.h"
#define ETHER_ADDR_LEN	ETHER_ADDRESS_LENGTH
#include "virtio_net.h"

#define MAX_FRAME_SIZE	1536

#define VIRTIO_NET_DRIVER_MODULE_NAME "drivers/network/virtio_net/driver_v1"
#define VIRTIO_NET_DEVICE_MODULE_NAME "drivers/network/virtio_net/device_v1"
#define VIRTIO_NET_DEVICE_ID_GENERATOR	"virtio_net/device_id"


typedef struct {
	device_node*			node;
	::virtio_device			virtio_device;
	virtio_device_interface*	virtio;

	uint32 					features;

	uint32					pairs_count;
	::virtio_queue*			receive_queues;
	::virtio_queue*			send_queues;


	bool					nonblocking;
	uint32					maxframesize;
	uint8					macaddr[6];

} virtio_net_driver_info;


typedef struct {
	virtio_net_driver_info*		info;
} virtio_net_handle;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fs/devfs.h>


#define TRACE_VIRTIO_NET
#ifdef TRACE_VIRTIO_NET
#	define TRACE(x...) dprintf("virtio_net: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_net:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static device_manager_info* sDeviceManager;


const char *
get_feature_name(uint32 feature)
{
	switch (feature) {
		case VIRTIO_NET_F_CSUM:
			return "host checksum";
		case VIRTIO_NET_F_GUEST_CSUM:
			return "guest checksum";
		case VIRTIO_NET_F_MAC:
			return "macaddress";
		case VIRTIO_NET_F_GSO:
			return "host allgso";
		case VIRTIO_NET_F_GUEST_TSO4:
			return "guest tso4";
		case VIRTIO_NET_F_GUEST_TSO6:
			return "guest tso6";
		case VIRTIO_NET_F_GUEST_ECN:
			return "guest tso6+ecn";
		case VIRTIO_NET_F_GUEST_UFO:
			return "guest ufo";
		case VIRTIO_NET_F_HOST_TSO4:
			return "host tso4";
		case VIRTIO_NET_F_HOST_TSO6:
			return "host tso6";
		case VIRTIO_NET_F_HOST_ECN:
			return "host tso6+ecn";
		case VIRTIO_NET_F_HOST_UFO:
			return "host UFO";
		case VIRTIO_NET_F_MRG_RXBUF:
			return "host mergerxbuffers";
		case VIRTIO_NET_F_STATUS:
			return "status";
		case VIRTIO_NET_F_CTRL_VQ:
			return "control vq";
		case VIRTIO_NET_F_CTRL_RX:
			return "rx mode";
		case VIRTIO_NET_F_CTRL_VLAN:
			return "vlan filter";
		case VIRTIO_NET_F_CTRL_RX_EXTRA:
			return "rx mode extra";
		case VIRTIO_NET_F_GUEST_ANNOUNCE:
			return "guest announce";
		case VIRTIO_NET_F_MQ:
			return "multiqueue";
		case VIRTIO_NET_F_CTRL_MAC_ADDR:
			return "set macaddress";
	}
	return NULL;
}


//	#pragma mark - device module API


static status_t
virtio_net_init_device(void* _info, void** _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&info->virtio,
		(void **)&info->virtio_device);
	sDeviceManager->put_node(parent);

	info->virtio->negociate_features(info->virtio_device,
		0 /*  */, &info->features, &get_feature_name);

	if ((info->features & VIRTIO_NET_F_MQ) != 0
			&& info->virtio->read_device_config(info->virtio_device,
				offsetof(struct virtio_net_config, max_virtqueue_pairs),
				&info->pairs_count, sizeof(info->pairs_count)) == B_OK) {
		system_info sysinfo;
		if (get_system_info(&sysinfo) == B_OK
			&& info->pairs_count > sysinfo.cpu_count) {
			info->pairs_count = sysinfo.cpu_count;
		}
	} else
		info->pairs_count = 1;

	// TODO read config

	uint32 queueCount = info->pairs_count * 2;
	if ((info->features & VIRTIO_NET_F_CTRL_VQ) != 0)
		queueCount++;
	::virtio_queue virtioQueues[queueCount];
	status_t status = info->virtio->alloc_queues(info->virtio_device, queueCount,
		virtioQueues);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}

	info->receive_queues = new(std::nothrow) virtio_queue[info->pairs_count];
	info->send_queues = new(std::nothrow) virtio_queue[info->pairs_count];
	if (info->receive_queues == NULL || info->receive_queues == NULL)
		return B_NO_MEMORY;
	for (uint32 i = 0; i < info->pairs_count; i++) {
		info->receive_queues[i] = virtioQueues[i * 2];
		info->send_queues[i] = virtioQueues[i * 2 + 1];
	}

	// TODO setup interrupts

	*_cookie = info;
	return B_OK;
}


static void
virtio_net_uninit_device(void* _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
}


static status_t
virtio_net_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_info;

	virtio_net_handle* handle = (virtio_net_handle*)malloc(
		sizeof(virtio_net_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	info->nonblocking = (openMode & O_NONBLOCK) != 0;
	info->maxframesize = MAX_FRAME_SIZE;
	handle->info = info;

	if ((info->features & VIRTIO_NET_F_MAC) != 0) {
		info->virtio->read_device_config(info->virtio_device,
			offsetof(struct virtio_net_config, mac),
			&info->macaddr, sizeof(info->macaddr));
	}

	*_cookie = handle;
	return B_OK;
}


static status_t
virtio_net_close(void* cookie)
{
	//virtio_net_handle* handle = (virtio_net_handle*)cookie;
	CALLED();

	return B_OK;
}


static status_t
virtio_net_free(void* cookie)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;

	free(handle);
	return B_OK;
}


static status_t
virtio_net_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	// TODO implement
	return B_ERROR;
}


static status_t
virtio_net_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	// TODO implement
	return B_ERROR;
}


static status_t
virtio_net_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	virtio_net_driver_info* info = handle->info;

	TRACE("ioctl(op = %lx)\n", op);

	switch (op) {
		case ETHER_GETADDR:
			TRACE("ioctl: get macaddr\n");
			memcpy(buffer, &info->macaddr, sizeof(info->macaddr));
			return B_OK;

		case ETHER_INIT:
			TRACE("ioctl: init\n");
			return B_OK;

		case ETHER_GETFRAMESIZE:
			TRACE("ioctl: get frame size\n");
			*(uint32*)buffer = info->maxframesize;
			return B_OK;

		case ETHER_SETPROMISC:
			TRACE("ioctl: set promisc\n");
			break;

		case ETHER_NONBLOCK:
			info->nonblocking = *(int32 *)buffer == 0;
			TRACE("ioctl: non blocking ? %s\n", info->nonblocking ? "yes" : "no");
			return B_OK;

		case ETHER_ADDMULTI:
			TRACE("ioctl: add multicast\n");
			break;

		case ETHER_GET_LINK_STATE:
		{
			TRACE("ioctl: get link state\n");
			ether_link_state_t state;
			uint16 status = VIRTIO_NET_S_LINK_UP;
			if ((info->features & VIRTIO_NET_F_STATUS) != 0) {
				info->virtio->read_device_config(info->virtio_device,
					offsetof(struct virtio_net_config, status),
					&status, sizeof(status));
			}
			state.media = ((status & VIRTIO_NET_S_LINK_UP) != 0 ? IFM_ACTIVE : 0)
				| IFM_ETHER | IFM_FULL_DUPLEX | IFM_10G_T;
			state.speed = 10000000000ULL;
			state.quality = 1000;

			return user_memcpy(buffer, &state, sizeof(ether_link_state_t));
		}

		default:
			ERROR("ioctl: unknown message %" B_PRIx32 "\n", op);
			break;
	}


	return B_DEV_INVALID_IOCTL;
}


//	#pragma mark - driver module API


static float
virtio_net_supports_device(device_node *parent)
{
	CALLED();
	const char *bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Direct Access Device
	if (sDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_NETWORK)
		return 0.0;

	TRACE("Virtio network device found!\n");

	return 0.6;
}


static status_t
virtio_net_register_device(device_node *node)
{
	CALLED();

	// ready to register
	device_attr attrs[] = {
		{ NULL }
	};

	return sDeviceManager->register_node(node, VIRTIO_NET_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_net_init_driver(device_node *node, void **cookie)
{
	CALLED();

	virtio_net_driver_info* info = (virtio_net_driver_info*)malloc(
		sizeof(virtio_net_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
virtio_net_uninit_driver(void *_cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
	free(info);
}


static status_t
virtio_net_register_child_devices(void* _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(VIRTIO_NET_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "net/virtio/%" B_PRId32,
		id);

	status = sDeviceManager->publish_device(info->node, name,
		VIRTIO_NET_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct device_module_info sVirtioNetDevice = {
	{
		VIRTIO_NET_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	virtio_net_init_device,
	virtio_net_uninit_device,
	NULL, // remove,

	virtio_net_open,
	virtio_net_close,
	virtio_net_free,
	virtio_net_read,
	virtio_net_write,
	NULL,	// io
	virtio_net_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sVirtioNetDriver = {
	{
		VIRTIO_NET_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	virtio_net_supports_device,
	virtio_net_register_device,
	virtio_net_init_driver,
	virtio_net_uninit_driver,
	virtio_net_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sVirtioNetDriver,
	(module_info*)&sVirtioNetDevice,
	NULL
};
