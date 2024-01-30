/*
 * Copyright 2023, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <graphic_driver.h>

#include <lock.h>
#include <virtio.h>
#include <virtio_info.h>

#include <util/AutoLock.h>

#include "viogpu.h"


#define VIRTIO_GPU_DRIVER_MODULE_NAME "drivers/graphics/virtio_gpu/driver_v1"
#define VIRTIO_GPU_DEVICE_MODULE_NAME "drivers/graphics/virtio_gpu/device_v1"
#define VIRTIO_GPU_DEVICE_ID_GENERATOR	"virtio_gpu/device_id"


typedef struct {
	device_node*			node;
	::virtio_device			virtio_device;
	virtio_device_interface*	virtio;

	uint64 					features;

	::virtio_queue			controlQueue;
	mutex					commandLock;
	area_id					commandArea;
	addr_t					commandBuffer;
	phys_addr_t				commandPhysAddr;
	sem_id					commandDone;
	uint64					fenceId;

	::virtio_queue			cursorQueue;

	int						displayResourceId;
	uint32					framebufferWidth;
	uint32					framebufferHeight;
	area_id					framebufferArea;
	addr_t					framebuffer;
	size_t					framebufferSize;
	uint32					displayWidth;
	uint32					displayHeight;

	thread_id				updateThread;
	bool					updateThreadRunning;

	area_id					sharedArea;
	virtio_gpu_shared_info* sharedInfo;
} virtio_gpu_driver_info;


typedef struct {
	virtio_gpu_driver_info*		info;
} virtio_gpu_handle;


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fs/devfs.h>

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))


#define DEVICE_NAME				"virtio_gpu"
#define ACCELERANT_NAME	"virtio_gpu.accelerant"
//#define TRACE_VIRTIO_GPU
#ifdef TRACE_VIRTIO_GPU
#	define TRACE(x...) dprintf(DEVICE_NAME ": " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)			dprintf("\33[33m" DEVICE_NAME ":\33[0m " x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


static device_manager_info* sDeviceManager;


static void virtio_gpu_vqwait(void* driverCookie, void* cookie);


const char*
get_feature_name(uint64 feature)
{
	switch (feature) {
		case VIRTIO_GPU_F_VIRGL:
			return "virgl";
		case VIRTIO_GPU_F_EDID:
			return "edid";
		case VIRTIO_GPU_F_RESOURCE_UUID:
			return "res_uuid";
		case VIRTIO_GPU_F_RESOURCE_BLOB:
			return "res_blob";
	}
	return NULL;
}


static status_t
virtio_gpu_drain_queues(virtio_gpu_driver_info* info)
{
	while (info->virtio->queue_dequeue(info->controlQueue, NULL, NULL))
		;

	while (info->virtio->queue_dequeue(info->cursorQueue, NULL, NULL))
		;

	return B_OK;
}


status_t
virtio_gpu_send_cmd(virtio_gpu_driver_info* info, void *cmd, size_t cmdSize, void *response,
    size_t responseSize)
{
	struct virtio_gpu_ctrl_hdr *hdr = (struct virtio_gpu_ctrl_hdr *)info->commandBuffer;
	struct virtio_gpu_ctrl_hdr *responseHdr = (struct virtio_gpu_ctrl_hdr *)response;

	memcpy((void*)info->commandBuffer, cmd, cmdSize);
	memset((void*)(info->commandBuffer + cmdSize), 0, responseSize);
	hdr->flags |= VIRTIO_GPU_FLAG_FENCE;
	hdr->fence_id = ++info->fenceId;

	physical_entry entries[] {
		{ info->commandPhysAddr, cmdSize },
		{ info->commandPhysAddr + cmdSize, responseSize },
	};
	if (!info->virtio->queue_is_empty(info->controlQueue))
		return B_ERROR;

	status_t status = info->virtio->queue_request_v(info->controlQueue, entries, 1, 1, NULL);
	if (status != B_OK)
		return status;

	acquire_sem(info->commandDone);

	while (!info->virtio->queue_dequeue(info->controlQueue, NULL, NULL))
		spin(10);

	memcpy(response, (void*)(info->commandBuffer + cmdSize), responseSize);

	if (responseHdr->fence_id != info->fenceId) {
		ERROR("response fence id not right\n");
	}
	return B_OK;
}


status_t
virtio_gpu_get_display_info(virtio_gpu_driver_info* info)
{
	CALLED();
	struct virtio_gpu_ctrl_hdr hdr = {};
	struct virtio_gpu_resp_display_info displayInfo = {};

	hdr.type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO;

	virtio_gpu_send_cmd(info, &hdr, sizeof(hdr), &displayInfo, sizeof(displayInfo));

	if (displayInfo.hdr.type != VIRTIO_GPU_RESP_OK_DISPLAY_INFO) {
		ERROR("failed getting display info\n");
		return B_ERROR;
	}

	if (!displayInfo.pmodes[0].enabled) {
		ERROR("pmodes[0] is not enabled\n");
		return B_BAD_VALUE;
	}

	info->displayWidth = displayInfo.pmodes[0].r.width;
	info->displayHeight = displayInfo.pmodes[0].r.height;
	TRACE("virtio_gpu_get_display_info width %" B_PRIu32 " height %" B_PRIu32 "\n",
		info->displayWidth, info->displayHeight);

	return B_OK;
}


status_t
virtio_gpu_get_edids(virtio_gpu_driver_info* info, int scanout)
{
	CALLED();
	struct virtio_gpu_cmd_get_edid getEdid = {};
	struct virtio_gpu_resp_edid response = {};
	getEdid.hdr.type = VIRTIO_GPU_CMD_GET_EDID;
	getEdid.scanout = scanout;

	virtio_gpu_send_cmd(info, &getEdid, sizeof(getEdid), &response, sizeof(response));

	if (response.hdr.type != VIRTIO_GPU_RESP_OK_EDID) {
		ERROR("failed getting edids %d\n", response.hdr.type);
		return B_ERROR;
	}

	info->sharedInfo->has_edid = true;
	memcpy(&info->sharedInfo->edid_raw, response.edid, sizeof(edid1_raw));
	TRACE("virtio_gpu_get_edids success\n");

	return B_OK;
}


status_t
virtio_gpu_create_2d(virtio_gpu_driver_info* info, int resourceId, int width, int height)
{
	CALLED();
	struct virtio_gpu_resource_create_2d resource = {};
	struct virtio_gpu_ctrl_hdr response = {};

	resource.hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D;
	resource.resource_id = resourceId;
	resource.format = VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM;
	resource.width = width;
	resource.height = height;

	virtio_gpu_send_cmd(info, &resource, sizeof(resource), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("viogpu_create_2d: failed %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_unref(virtio_gpu_driver_info* info, int resourceId)
{
	CALLED();
	struct virtio_gpu_resource_unref resource = {};
	struct virtio_gpu_ctrl_hdr response = {};

	resource.hdr.type = VIRTIO_GPU_CMD_RESOURCE_UNREF;
	resource.resource_id = resourceId;

	virtio_gpu_send_cmd(info, &resource, sizeof(resource), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_unref: failed %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_attach_backing(virtio_gpu_driver_info* info, int resourceId)
{
	CALLED();
	struct virtio_gpu_resource_attach_backing_entries {
		struct virtio_gpu_resource_attach_backing backing;
		struct virtio_gpu_mem_entry entries[16];
	} _PACKED backing = {};
	struct virtio_gpu_ctrl_hdr response = {};

	physical_entry entries[16] = {};
	status_t status = get_memory_map((void*)info->framebuffer, info->framebufferSize, entries, 16);
	if (status != B_OK) {
		ERROR("virtio_gpu_attach_backing get_memory_map failed: %s\n", strerror(status));
		return status;
	}

	backing.backing.hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING;
	backing.backing.resource_id = resourceId;
	for (int i = 0; i < 16; i++) {
		if (entries[i].size == 0)
			break;
		TRACE("virtio_gpu_attach_backing %d %" B_PRIxPHYSADDR " %" B_PRIxPHYSADDR "\n", i,
			entries[i].address, entries[i].size);
		backing.entries[i].addr = entries[i].address;
		backing.entries[i].length = entries[i].size;
		backing.backing.nr_entries++;
	}

	virtio_gpu_send_cmd(info, &backing, sizeof(backing), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_attach_backing failed: %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_detach_backing(virtio_gpu_driver_info* info, int resourceId)
{
	CALLED();
	struct virtio_gpu_resource_detach_backing backing;
	struct virtio_gpu_ctrl_hdr response = {};

	backing.hdr.type = VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING;
	backing.resource_id = resourceId;

	virtio_gpu_send_cmd(info, &backing, sizeof(backing), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_detach_backing failed: %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_set_scanout(virtio_gpu_driver_info* info, int scanoutId, int resourceId,
    uint32 width, uint32 height)
{
	CALLED();
	struct virtio_gpu_set_scanout set_scanout = {};
	struct virtio_gpu_ctrl_hdr response = {};

	set_scanout.hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT;
	set_scanout.scanout_id = scanoutId;
	set_scanout.resource_id = resourceId;
	set_scanout.r.width = width;
	set_scanout.r.height = height;

	virtio_gpu_send_cmd(info, &set_scanout, sizeof(set_scanout), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_set_scanout failed %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_transfer_to_host_2d(virtio_gpu_driver_info* info, int resourceId,
    uint32 width, uint32 height)
{
	struct virtio_gpu_transfer_to_host_2d transferToHost = {};
	struct virtio_gpu_ctrl_hdr response = {};

	transferToHost.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
	transferToHost.resource_id = resourceId;
	transferToHost.r.width = width;
	transferToHost.r.height = height;

	virtio_gpu_send_cmd(info, &transferToHost, sizeof(transferToHost), &response,
		sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_transfer_to_host_2d failed %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_gpu_flush_resource(virtio_gpu_driver_info* info, int resourceId, uint32 width,
    uint32 height)
{
	struct virtio_gpu_resource_flush resourceFlush = {};
	struct virtio_gpu_ctrl_hdr response = {};

	resourceFlush.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
	resourceFlush.resource_id = resourceId;
	resourceFlush.r.width = width;
	resourceFlush.r.height = height;

	virtio_gpu_send_cmd(info, &resourceFlush, sizeof(resourceFlush), &response, sizeof(response));

	if (response.type != VIRTIO_GPU_RESP_OK_NODATA) {
		ERROR("virtio_gpu_flush_resource failed %d\n", response.type);
		return B_ERROR;
	}

	return B_OK;
}


status_t
virtio_update_thread(void *arg)
{
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)arg;

	while (info->updateThreadRunning) {
		bigtime_t start = system_time();
		MutexLocker commandLocker(&info->commandLock);
		virtio_gpu_transfer_to_host_2d(info, info->displayResourceId, info->displayWidth,
			info->displayHeight);
		virtio_gpu_flush_resource(info, info->displayResourceId, info->displayWidth, info->displayHeight);
		bigtime_t delay = system_time() - start;
		if (delay < 20000)
			snooze(20000 - delay);
	}
	return B_OK;
}


status_t
virtio_gpu_set_display_mode(virtio_gpu_driver_info* info, display_mode *mode)
{
	CALLED();

	int newResourceId = info->displayResourceId + 1;

	// create framebuffer area
	TRACE("virtio_gpu_set_display_mode %" B_PRIu16 " %" B_PRIu16 "\n", mode->virtual_width,
		mode->virtual_height);

	status_t status = virtio_gpu_create_2d(info, newResourceId, mode->virtual_width, mode->virtual_height);
	if (status != B_OK)
		return status;

	status = virtio_gpu_attach_backing(info, newResourceId);
	if (status != B_OK)
		return status;

	status = virtio_gpu_unref(info, info->displayResourceId);
	if (status != B_OK)
		return status;

	info->displayResourceId = newResourceId;
	info->displayWidth = mode->virtual_width;
	info->displayHeight = mode->virtual_height;

	status = virtio_gpu_set_scanout(info, 0, 0, 0, 0);
	if (status != B_OK)
		return status;

	status = virtio_gpu_set_scanout(info, 0, info->displayResourceId, info->displayWidth, info->displayHeight);
	if (status != B_OK)
		return status;

	status = virtio_gpu_transfer_to_host_2d(info, info->displayResourceId, info->displayWidth, info->displayHeight);
	if (status != B_OK)
		return status;

	status = virtio_gpu_flush_resource(info, info->displayResourceId, info->displayWidth, info->displayHeight);
	if (status != B_OK)
		return status;

	{
		virtio_gpu_shared_info& sharedInfo = *info->sharedInfo;
		sharedInfo.frame_buffer_area = info->framebufferArea;
		sharedInfo.frame_buffer = (uint8*)info->framebuffer;
		sharedInfo.bytes_per_row = info->displayWidth * 4;
		sharedInfo.current_mode.virtual_width = info->displayWidth;
		sharedInfo.current_mode.virtual_height = info->displayHeight;
		sharedInfo.current_mode.space = B_RGB32;
	}

	return B_OK;
}


//	#pragma mark - device module API


static status_t
virtio_gpu_init_device(void* _info, void** _cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)_info;

	device_node* parent = sDeviceManager->get_parent_node(info->node);
	sDeviceManager->get_driver(parent, (driver_module_info**)&info->virtio,
		(void**)&info->virtio_device);
	sDeviceManager->put_node(parent);

	info->virtio->negotiate_features(info->virtio_device, VIRTIO_GPU_F_EDID,
		 &info->features, &get_feature_name);

	// TODO read config

	// Setup queues
	::virtio_queue virtioQueues[2];
	status_t status = info->virtio->alloc_queues(info->virtio_device, 2,
		virtioQueues);
	if (status != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(status));
		return status;
	}

	info->controlQueue = virtioQueues[0];
	info->cursorQueue = virtioQueues[1];

	// create command buffer area
	info->commandArea = create_area("virtiogpu command buffer", (void**)&info->commandBuffer,
		B_ANY_KERNEL_BLOCK_ADDRESS, B_PAGE_SIZE,
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (info->commandArea < B_OK) {
		status = info->commandArea;
		goto err1;
	}

	physical_entry entry;
	status = get_memory_map((void*)info->commandBuffer, B_PAGE_SIZE, &entry, 1);
	if (status != B_OK)
		goto err2;

	info->commandPhysAddr = entry.address;
	mutex_init(&info->commandLock, "virtiogpu command lock");

	// Setup interrupt
	status = info->virtio->setup_interrupt(info->virtio_device, NULL, info);
	if (status != B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(status));
		goto err3;
	}

	status = info->virtio->queue_setup_interrupt(info->controlQueue,
		virtio_gpu_vqwait, info);
	if (status != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(status));
		goto err3;
	}

	*_cookie = info;
	return B_OK;

err3:
err2:
	delete_area(info->commandArea);
err1:
	return status;
}


static void
virtio_gpu_uninit_device(void* _cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)_cookie;

	info->virtio->free_interrupts(info->virtio_device);

	mutex_destroy(&info->commandLock);

	delete_area(info->commandArea);
	info->commandArea = -1;
	info->virtio->free_queues(info->virtio_device);
}


static status_t
virtio_gpu_open(void* _info, const char* path, int openMode, void** _cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)_info;
	status_t status;
	size_t sharedSize = (sizeof(virtio_gpu_shared_info) + 7) & ~7;
	MutexLocker commandLocker;

	virtio_gpu_handle* handle = (virtio_gpu_handle*)malloc(
		sizeof(virtio_gpu_handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	info->commandDone = create_sem(1, "virtio_gpu_command");
	if (info->commandDone < B_OK)
		goto error;

	info->sharedArea = create_area("virtio_gpu shared info",
		(void**)&info->sharedInfo, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sharedSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_CLONEABLE_AREA);
	if (info->sharedArea < 0)
		goto error;
	memset(info->sharedInfo, 0, sizeof(virtio_gpu_shared_info));

	commandLocker.SetTo(&info->commandLock, false, true);

	status = virtio_gpu_get_display_info(info);
	if (status != B_OK)
		goto error;

	if ((info->features & VIRTIO_GPU_F_EDID) != 0)
		virtio_gpu_get_edids(info, 0);

	// so we can fit every mode
	info->framebufferWidth = 3840;
	info->framebufferHeight = 2160;

	// create framebuffer area
	info->framebufferSize = 4 * info->framebufferWidth * info->framebufferHeight;
	info->framebufferArea = create_area("virtio_gpu framebuffer", (void**)&info->framebuffer,
		B_ANY_KERNEL_ADDRESS, info->framebufferSize,
		B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (info->framebufferArea < B_OK) {
		status = info->framebufferArea;
		goto error;
	}

	info->displayResourceId = 1;
	status = virtio_gpu_create_2d(info, info->displayResourceId, info->displayWidth,
		info->displayHeight);
	if (status != B_OK)
		goto error;

	status = virtio_gpu_attach_backing(info, info->displayResourceId);
	if (status != B_OK)
		goto error;

	status = virtio_gpu_set_scanout(info, 0, info->displayResourceId, info->displayWidth,
		info->displayHeight);
	if (status != B_OK)
		goto error;

	{
		virtio_gpu_shared_info& sharedInfo = *info->sharedInfo;
		sharedInfo.frame_buffer_area = info->framebufferArea;
		sharedInfo.frame_buffer = (uint8*)info->framebuffer;
		sharedInfo.bytes_per_row = info->displayWidth * 4;
		sharedInfo.current_mode.virtual_width = info->displayWidth;
		sharedInfo.current_mode.virtual_height = info->displayHeight;
		sharedInfo.current_mode.space = B_RGB32;
	}
	info->updateThreadRunning = true;
	info->updateThread = spawn_kernel_thread(virtio_update_thread, "virtio_gpu update",
		B_DISPLAY_PRIORITY, info);
	if (info->updateThread < B_OK)
		goto error;
	resume_thread(info->updateThread);

	handle->info = info;

	*_cookie = handle;
	return B_OK;

error:
	delete_area(info->framebufferArea);
	info->framebufferArea = -1;
	delete_sem(info->commandDone);
	info->commandDone = -1;
	free(handle);
	return B_ERROR;
}


static status_t
virtio_gpu_close(void* cookie)
{
	virtio_gpu_handle* handle = (virtio_gpu_handle*)cookie;
	CALLED();

	virtio_gpu_driver_info* info = handle->info;
	info->updateThreadRunning = false;
	delete_sem(info->commandDone);
	info->commandDone  = -1;

	return B_OK;
}


static status_t
virtio_gpu_free(void* cookie)
{
	CALLED();
	virtio_gpu_handle* handle = (virtio_gpu_handle*)cookie;

	virtio_gpu_driver_info* info = handle->info;
	int32 result;
	wait_for_thread(info->updateThread, &result);
	info->updateThread = -1;
	virtio_gpu_drain_queues(info);
	free(handle);
	return B_OK;
}


static void
virtio_gpu_vqwait(void* driverCookie, void* cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)cookie;

	release_sem_etc(info->commandDone, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
virtio_gpu_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
virtio_gpu_write(void* cookie, off_t pos, const void* buffer,
	size_t* _length)
{
	*_length = 0;
	return B_NOT_ALLOWED;
}


static status_t
virtio_gpu_ioctl(void* cookie, uint32 op, void* buffer, size_t length)
{
	CALLED();
	virtio_gpu_handle* handle = (virtio_gpu_handle*)cookie;
	virtio_gpu_driver_info* info = handle->info;

	// TRACE("ioctl(op = %lx)\n", op);

	switch (op) {
		case B_GET_ACCELERANT_SIGNATURE:
			dprintf(DEVICE_NAME ": acc: %s\n", ACCELERANT_NAME);
			if (user_strlcpy((char*)buffer, ACCELERANT_NAME,
					B_FILE_NAME_LENGTH) < B_OK)
				return B_BAD_ADDRESS;

			return B_OK;

		// needed to share data between kernel and accelerant
		case VIRTIO_GPU_GET_PRIVATE_DATA:
			return user_memcpy(buffer, &info->sharedArea, sizeof(area_id));

		case VIRTIO_GPU_SET_DISPLAY_MODE:
		{
			if (length != sizeof(display_mode))
				return B_BAD_VALUE;

			display_mode mode;
			if (user_memcpy(&mode, buffer, sizeof(display_mode)) != B_OK)
				return B_BAD_ADDRESS;

			MutexLocker commandLocker(&info->commandLock);

			return virtio_gpu_set_display_mode(info, &mode);
		}
		default:
			ERROR("ioctl: unknown message %" B_PRIx32 "\n", op);
			break;
	}


	return B_DEV_INVALID_IOCTL;
}


//	#pragma mark - driver module API


static float
virtio_gpu_supports_device(device_node* parent)
{
	CALLED();
	const char* bus;
	uint16 deviceType;

	// make sure parent is really the Virtio bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "virtio"))
		return 0.0;

	// check whether it's really a Virtio GPU device
	if (sDeviceManager->get_attr_uint16(parent, VIRTIO_DEVICE_TYPE_ITEM,
			&deviceType, true) != B_OK || deviceType != VIRTIO_DEVICE_ID_GPU)
		return 0.0;

	TRACE("Virtio gpu device found!\n");

	return 0.6;
}


static status_t
virtio_gpu_register_device(device_node* node)
{
	CALLED();

	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "Virtio GPU"} },
		{ NULL }
	};

	return sDeviceManager->register_node(node, VIRTIO_GPU_DRIVER_MODULE_NAME,
		attrs, NULL, NULL);
}


static status_t
virtio_gpu_init_driver(device_node* node, void** cookie)
{
	CALLED();

	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)malloc(
		sizeof(virtio_gpu_driver_info));
	if (info == NULL)
		return B_NO_MEMORY;

	memset(info, 0, sizeof(*info));

	info->node = node;

	*cookie = info;
	return B_OK;
}


static void
virtio_gpu_uninit_driver(void* _cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)_cookie;
	free(info);
}


static status_t
virtio_gpu_register_child_devices(void* _cookie)
{
	CALLED();
	virtio_gpu_driver_info* info = (virtio_gpu_driver_info*)_cookie;
	status_t status;

	int32 id = sDeviceManager->create_id(VIRTIO_GPU_DEVICE_ID_GENERATOR);
	if (id < 0)
		return id;

	char name[64];
	snprintf(name, sizeof(name), "graphics/virtio/%" B_PRId32,
		id);

	status = sDeviceManager->publish_device(info->node, name,
		VIRTIO_GPU_DEVICE_MODULE_NAME);

	return status;
}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&sDeviceManager},
	{}
};

struct device_module_info sVirtioGpuDevice = {
	{
		VIRTIO_GPU_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	virtio_gpu_init_device,
	virtio_gpu_uninit_device,
	NULL, // remove,

	virtio_gpu_open,
	virtio_gpu_close,
	virtio_gpu_free,
	virtio_gpu_read,
	virtio_gpu_write,
	NULL,	// io
	virtio_gpu_ioctl,

	NULL,	// select
	NULL,	// deselect
};

struct driver_module_info sVirtioGpuDriver = {
	{
		VIRTIO_GPU_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	virtio_gpu_supports_device,
	virtio_gpu_register_device,
	virtio_gpu_init_driver,
	virtio_gpu_uninit_driver,
	virtio_gpu_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};

module_info* modules[] = {
	(module_info*)&sVirtioGpuDriver,
	(module_info*)&sVirtioGpuDevice,
	NULL
};
