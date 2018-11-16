/*
 * Copyright 2013, 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2017, Philippe Houdoin, philippe.houdoin@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <net/if_media.h>
#include <new>

#include <ethernet.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <virtio.h>

#include "ether_driver.h"
#define ETHER_ADDR_LEN	ETHER_ADDRESS_LENGTH
#include "virtio_net.h"


#define VIRTIO_NET_DRIVER_MODULE_NAME "drivers/network/virtio_net/driver_v1"
#define VIRTIO_NET_DEVICE_MODULE_NAME "drivers/network/virtio_net/device_v1"
#define VIRTIO_NET_DEVICE_ID_GENERATOR	"virtio_net/device_id"

#define BUFFER_SIZE	2048
#define MAX_FRAME_SIZE 1536


struct virtio_net_rx_hdr {
	struct virtio_net_hdr	hdr;
	uint8					pad[4];
} _PACKED;


struct virtio_net_tx_hdr {
	union {
		struct virtio_net_hdr			hdr;
		struct virtio_net_hdr_mrg_rxbuf mhdr;
	};
} _PACKED;


struct BufInfo : DoublyLinkedListLinkImpl<BufInfo> {
	char*					buffer;
	struct virtio_net_hdr*	hdr;
	physical_entry			entry;
	physical_entry			hdrEntry;
	uint32					rxUsedLength;
};


typedef DoublyLinkedList<BufInfo> BufInfoList;


typedef struct {
	device_node*			node;
	::virtio_device			virtio_device;
	virtio_device_interface*	virtio;

	uint32 					features;

	uint32					pairsCount;

	::virtio_queue*			rxQueues;
	uint16*					rxSizes;

	BufInfo**				rxBufInfos;
	sem_id					rxDone;
	area_id					rxArea;
	BufInfoList				rxFullList;
	mutex					rxLock;

	::virtio_queue*			txQueues;
	uint16*					txSizes;

	BufInfo**				txBufInfos;
	sem_id					txDone;
	area_id					txArea;
	BufInfoList				txFreeList;
	mutex					txLock;

	::virtio_queue			ctrlQueue;

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


//#define TRACE_VIRTIO_NET
#ifdef TRACE_VIRTIO_NET
#	define TRACE(x...) dprintf("virtio_net: " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33mvirtio_net:\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static device_manager_info* sDeviceManager;


static void virtio_net_rxDone(void* driverCookie, void* cookie);
static void virtio_net_txDone(void* driverCookie, void* cookie);


const char*
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


static status_t
virtio_net_drain_queues(virtio_net_driver_info* info)
{
	while (true) {
		BufInfo* buf = (BufInfo*)info->virtio->queue_dequeue(
			info->txQueues[0], NULL);
		if (buf == NULL)
			break;
		info->txFreeList.Add(buf);
	}

	while (true) {
		BufInfo* buf = (BufInfo*)info->virtio->queue_dequeue(
			info->rxQueues[0], NULL);
		if (buf == NULL)
			break;
	}

	while (true) {
		BufInfo* buf = info->rxFullList.RemoveHead();
		if (buf == NULL)
			break;
	}

	return B_OK;
}


static status_t
virtio_net_rx_enqueue_buf(virtio_net_driver_info* info, BufInfo* buf)
{
	physical_entry entries[2];
	entries[0] = buf->hdrEntry;
	entries[1] = buf->entry;

	memset(buf->hdr, 0, sizeof(struct virtio_net_hdr));

	// queue the rx buffer
	status_t status = info->virtio->queue_request_v(info->rxQueues[0],
		entries, 0, 2, buf);
	if (status != B_OK) {
		ERROR("rx queueing on queue %d failed (%s)\n", 0, strerror(status));
		return status;
	}

	return B_OK;
}


#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))


//	#pragma mark - device module API


static status_t
virtio_net_init_device(void* _info, void** _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&info->virtio,
		(void**)&info->virtio_device);
	sDeviceManager->put_node(parent);

	info->virtio->negotiate_features(info->virtio_device,
		VIRTIO_NET_F_STATUS | VIRTIO_NET_F_MAC
		/* VIRTIO_NET_F_CTRL_VQ | VIRTIO_NET_F_MQ */,
		 &info->features, &get_feature_name);

	if ((info->features & VIRTIO_NET_F_MQ) != 0
			&& (info->features & VIRTIO_NET_F_CTRL_VQ) != 0
			&& info->virtio->read_device_config(info->virtio_device,
				offsetof(struct virtio_net_config, max_virtqueue_pairs),
				&info->pairsCount, sizeof(info->pairsCount)) == B_OK) {
		system_info sysinfo;
		if (get_system_info(&sysinfo) == B_OK
			&& info->pairsCount > sysinfo.cpu_count) {
			info->pairsCount = sysinfo.cpu_count;
		}
	} else
		info->pairsCount = 1;

	// TODO read config

	// Setup queues
	uint32 queueCount = info->pairsCount * 2;
	if ((info->features & VIRTIO_NET_F_CTRL_VQ) != 0)
		queueCount++;
	::virtio_queue virtioQueues[queueCount];
	status_t status = info->virtio->alloc_queues(info->virtio_device, queueCount,
		virtioQueues);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}

	char* rxBuffer;
	char* txBuffer;

	info->rxQueues = new(std::nothrow) virtio_queue[info->pairsCount];
	info->txQueues = new(std::nothrow) virtio_queue[info->pairsCount];
	info->rxSizes = new(std::nothrow) uint16[info->pairsCount];
	info->txSizes = new(std::nothrow) uint16[info->pairsCount];
	if (info->rxQueues == NULL || info->txQueues == NULL
		|| info->rxSizes == NULL || info->txSizes == NULL) {
		status = B_NO_MEMORY;
		goto err1;
	}
	for (uint32 i = 0; i < info->pairsCount; i++) {
		info->rxQueues[i] = virtioQueues[i * 2];
		info->txQueues[i] = virtioQueues[i * 2 + 1];
		info->rxSizes[i] = info->virtio->queue_size(info->rxQueues[i]) / 2;
		info->txSizes[i] = info->virtio->queue_size(info->txQueues[i]) / 2;
	}
	if ((info->features & VIRTIO_NET_F_CTRL_VQ) != 0)
		info->ctrlQueue = virtioQueues[info->pairsCount * 2];

	info->rxBufInfos = new(std::nothrow) BufInfo*[info->rxSizes[0]];
	info->txBufInfos = new(std::nothrow) BufInfo*[info->txSizes[0]];
	if (info->rxBufInfos == NULL || info->txBufInfos == NULL) {
		status = B_NO_MEMORY;
		goto err2;
	}
	memset(info->rxBufInfos, 0, sizeof(info->rxBufInfos));
	memset(info->txBufInfos, 0, sizeof(info->txBufInfos));

	// create receive buffer area
	info->rxArea = create_area("virtionet rx buffer", (void**)&rxBuffer,
		B_ANY_KERNEL_BLOCK_ADDRESS, ROUND_TO_PAGE_SIZE(
			BUFFER_SIZE * info->rxSizes[0]),
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (info->rxArea < B_OK) {
		status = info->rxArea;
		goto err3;
	}

	// initialize receive buffer descriptors
	for (int i = 0; i < info->rxSizes[0]; i++) {
		BufInfo* buf = new(std::nothrow) BufInfo;
		if (buf == NULL) {
			status = B_NO_MEMORY;
			goto err4;
		}

		info->rxBufInfos[i] = buf;
		buf->hdr = (struct virtio_net_hdr*)((addr_t)rxBuffer
			+ i * BUFFER_SIZE);
		buf->buffer = (char*)((addr_t)buf->hdr + sizeof(virtio_net_rx_hdr));

		status = get_memory_map(buf->buffer,
			BUFFER_SIZE - sizeof(virtio_net_rx_hdr), &buf->entry, 1);
		if (status != B_OK)
			goto err4;

		status = get_memory_map(buf->hdr, sizeof(struct virtio_net_hdr),
			&buf->hdrEntry, 1);
		if (status != B_OK)
			goto err4;
	}

	// create transmit buffer area
	info->txArea = create_area("virtionet tx buffer", (void**)&txBuffer,
		B_ANY_KERNEL_BLOCK_ADDRESS, ROUND_TO_PAGE_SIZE(
			BUFFER_SIZE * info->txSizes[0]),
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (info->txArea < B_OK) {
		status = info->txArea;
		goto err5;
	}

	// initialize transmit buffer descriptors
	for (int i = 0; i < info->txSizes[0]; i++) {
		BufInfo* buf = new(std::nothrow) BufInfo;
		if (buf == NULL) {
			status = B_NO_MEMORY;
			goto err6;
		}

		info->txBufInfos[i] = buf;
		buf->hdr = (struct virtio_net_hdr*)((addr_t)txBuffer
			+ i * BUFFER_SIZE);
		buf->buffer = (char*)((addr_t)buf->hdr + sizeof(virtio_net_tx_hdr));

		status = get_memory_map(buf->buffer,
			BUFFER_SIZE - sizeof(virtio_net_tx_hdr), &buf->entry, 1);
		if (status != B_OK)
			goto err6;

		status = get_memory_map(buf->hdr, sizeof(struct virtio_net_hdr),
			&buf->hdrEntry, 1);
		if (status != B_OK)
			goto err6;

		info->txFreeList.Add(buf);
	}

	mutex_init(&info->rxLock, "virtionet rx lock");
	mutex_init(&info->txLock, "virtionet tx lock");

	// Setup interrupt
	status = info->virtio->setup_interrupt(info->virtio_device, NULL, info);
	if (status != B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(status));
		goto err6;
	}

	status = info->virtio->queue_setup_interrupt(info->rxQueues[0],
		virtio_net_rxDone, info);
	if (status != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(status));
		goto err6;
	}

	status = info->virtio->queue_setup_interrupt(info->txQueues[0],
		virtio_net_txDone, info);
	if (status != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(status));
		goto err6;
	}

	if ((info->features & VIRTIO_NET_F_CTRL_VQ) != 0) {
		status = info->virtio->queue_setup_interrupt(info->ctrlQueue,
			NULL, info);
		if (status != B_OK) {
			ERROR("queue interrupt setup failed (%s)\n", strerror(status));
			goto err6;
		}
	}

	*_cookie = info;
	return B_OK;

err6:
	for (int i = 0; i < info->txSizes[0]; i++)
		delete info->txBufInfos[i];
err5:
	delete_area(info->txArea);
err4:
	for (int i = 0; i < info->rxSizes[0]; i++)
		delete info->rxBufInfos[i];
err3:
	delete_area(info->rxArea);
err2:
	delete[] info->rxBufInfos;
	delete[] info->txBufInfos;
err1:
	delete[] info->rxQueues;
	delete[] info->txQueues;
	delete[] info->rxSizes;
	delete[] info->txSizes;
	return status;
}


static void
virtio_net_uninit_device(void* _cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)_cookie;

	info->virtio->free_interrupts(info->virtio_device);

	mutex_destroy(&info->rxLock);
	mutex_destroy(&info->txLock);

	while (true) {
		BufInfo* buf = info->txFreeList.RemoveHead();
		if (buf == NULL)
			break;
	}

	for (int i = 0; i < info->rxSizes[0]; i++) {
		delete info->rxBufInfos[i];
	}
	for (int i = 0; i < info->txSizes[0]; i++) {
		delete info->txBufInfos[i];
	}
	delete_area(info->rxArea);
	delete_area(info->txArea);
	delete[] info->rxBufInfos;
	delete[] info->txBufInfos;
	delete[] info->rxSizes;
	delete[] info->txSizes;
	delete[] info->rxQueues;
	delete[] info->txQueues;

	info->virtio->free_queues(info->virtio_device);
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
	info->rxDone = create_sem(0, "virtio_net_rx");
	info->txDone = create_sem(1, "virtio_net_tx");
	if (info->rxDone < B_OK || info->txDone < B_OK)
		goto error;
	handle->info = info;

	if ((info->features & VIRTIO_NET_F_MAC) != 0) {
		info->virtio->read_device_config(info->virtio_device,
			offsetof(struct virtio_net_config, mac),
			&info->macaddr, sizeof(info->macaddr));
	}

	for (int i = 0; i < info->rxSizes[0]; i++)
		virtio_net_rx_enqueue_buf(info, info->rxBufInfos[i]);

	*_cookie = handle;
	return B_OK;

error:
	delete_sem(info->rxDone);
	delete_sem(info->txDone);
	info->rxDone = info->txDone = -1;
	free(handle);
	return B_ERROR;
}


static status_t
virtio_net_close(void* cookie)
{
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	CALLED();

	virtio_net_driver_info* info = handle->info;
	delete_sem(info->rxDone);
	delete_sem(info->txDone);
	info->rxDone = info->txDone = -1;

	return B_OK;
}


static status_t
virtio_net_free(void* cookie)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;

	virtio_net_driver_info* info = handle->info;
	virtio_net_drain_queues(info);
	free(handle);
	return B_OK;
}


static void
virtio_net_rxDone(void* driverCookie, void* cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)cookie;

	release_sem_etc(info->rxDone, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
virtio_net_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	virtio_net_driver_info* info = handle->info;

	mutex_lock(&info->rxLock);
	while (info->rxFullList.Head() == NULL) {
		mutex_unlock(&info->rxLock);

		if (info->nonblocking)
			return B_WOULD_BLOCK;
		TRACE("virtio_net_read: waiting\n");
		status_t status = acquire_sem(info->rxDone);
		if (status != B_OK) {
			ERROR("acquire_sem(rxDone) failed (%s)\n", strerror(status));
			return status;
		}
		int32 semCount = 0;
		get_sem_count(info->rxDone, &semCount);
		if (semCount > 0)
			acquire_sem_etc(info->rxDone, semCount, B_RELATIVE_TIMEOUT, 0);

		mutex_lock(&info->rxLock);
		while (info->rxDone != -1) {
			uint32 usedLength = 0;
			BufInfo* buf = (BufInfo*)info->virtio->queue_dequeue(
				info->rxQueues[0], &usedLength);
			if (buf == NULL)
				break;

			buf->rxUsedLength = usedLength;
			info->rxFullList.Add(buf);
		}
		TRACE("virtio_net_read: finished waiting\n");
	}

	BufInfo* buf = info->rxFullList.RemoveHead();
	*_length = MIN(buf->rxUsedLength, *_length);
	memcpy(buffer, buf->buffer, *_length);
	virtio_net_rx_enqueue_buf(info, buf);
	mutex_unlock(&info->rxLock);
	return B_OK;
}


static void
virtio_net_txDone(void* driverCookie, void* cookie)
{
	CALLED();
	virtio_net_driver_info* info = (virtio_net_driver_info*)cookie;

	release_sem_etc(info->txDone, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
virtio_net_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	virtio_net_driver_info* info = handle->info;

	mutex_lock(&info->txLock);
	while (info->txFreeList.Head() == NULL) {
		mutex_unlock(&info->txLock);
		if (info->nonblocking)
			return B_WOULD_BLOCK;

		status_t status = acquire_sem(info->txDone);
		if (status != B_OK) {
			ERROR("acquire_sem(txDone) failed (%s)\n", strerror(status));
			return status;
		}

		int32 semCount = 0;
		get_sem_count(info->txDone, &semCount);
		if (semCount > 0)
			acquire_sem_etc(info->txDone, semCount, B_RELATIVE_TIMEOUT, 0);

		mutex_lock(&info->txLock);
		while (info->txDone != -1) {
			BufInfo* buf = (BufInfo*)info->virtio->queue_dequeue(
				info->txQueues[0], NULL);
			if (buf == NULL)
				break;
			info->txFreeList.Add(buf);
		}
	}
	BufInfo* buf = info->txFreeList.RemoveHead();

	TRACE("virtio_net_write: copying %lu\n", MIN(MAX_FRAME_SIZE, *_length));
	memcpy(buf->buffer, buffer, MIN(MAX_FRAME_SIZE, *_length));
	memset(buf->hdr, 0, sizeof(virtio_net_hdr));

	physical_entry entries[2];
	entries[0] = buf->hdrEntry;
	entries[0].size = sizeof(virtio_net_hdr);
	entries[1] = buf->entry;
	entries[1].size = MIN(MAX_FRAME_SIZE, *_length);

	// queue the virtio_net_hdr + buffer data
	status_t status = info->virtio->queue_request_v(info->txQueues[0],
		entries, 2, 0, buf);
	mutex_unlock(&info->txLock);
	if (status != B_OK) {
		ERROR("tx queueing on queue %d failed (%s)\n", 0, strerror(status));
		return status;
	}

	return B_OK;
}


static status_t
virtio_net_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	// CALLED();
	virtio_net_handle* handle = (virtio_net_handle*)cookie;
	virtio_net_driver_info* info = handle->info;

	// TRACE("ioctl(op = %lx)\n", op);

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
			info->nonblocking = *(int32*)buffer == 0;
			TRACE("ioctl: non blocking ? %s\n", info->nonblocking ? "yes" : "no");
			return B_OK;

		case ETHER_ADDMULTI:
			TRACE("ioctl: add multicast\n");
			break;

		case ETHER_REMMULTI:
			TRACE("ioctl: remove multicast\n");
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
virtio_net_supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
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
virtio_net_register_device(device_node* node)
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
virtio_net_init_driver(device_node* node, void** cookie)
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
virtio_net_uninit_driver(void* _cookie)
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
