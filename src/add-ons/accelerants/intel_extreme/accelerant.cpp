/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "accelerant.h"

#include "utility.h"

#include <Debug.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include <new>

#include <AGP.h>


#undef TRACE
#define TRACE_ACCELERANT
#ifdef TRACE_ACCELERANT
#	define TRACE(x...) _sPrintf("intel_extreme: " x)
#else
#	define TRACE(x...)
#endif

#define ERROR(x...) _sPrintf("intel_extreme: " x)
#define CALLED(x...) TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


struct accelerant_info* gInfo;


class AreaCloner {
public:
							AreaCloner();
							~AreaCloner();

			area_id			Clone(const char* name, void** _address,
								uint32 spec, uint32 protection,
								area_id sourceArea);
			status_t		InitCheck()
								{ return fArea < 0 ? (status_t)fArea : B_OK; }
			void			Keep();

private:
			area_id			fArea;
};


AreaCloner::AreaCloner()
	:
	fArea(-1)
{
}


AreaCloner::~AreaCloner()
{
	if (fArea >= 0)
		delete_area(fArea);
}


area_id
AreaCloner::Clone(const char* name, void** _address, uint32 spec,
	uint32 protection, area_id sourceArea)
{
	fArea = clone_area(name, _address, spec, protection, sourceArea);
	return fArea;
}


void
AreaCloner::Keep()
{
	fArea = -1;
}


//	#pragma mark -


/*! This is the common accelerant_info initializer. It is called by
	both, the first accelerant and all clones.
*/
static status_t
init_common(int device, bool isClone)
{
	// initialize global accelerant info structure

	gInfo = (accelerant_info*)malloc(sizeof(accelerant_info));
	if (gInfo == NULL)
		return B_NO_MEMORY;

	memset(gInfo, 0, sizeof(accelerant_info));

	gInfo->is_clone = isClone;
	gInfo->device = device;

	// get basic info from driver

	intel_get_private_data data;
	data.magic = INTEL_PRIVATE_DATA_MAGIC;

	if (ioctl(device, INTEL_GET_PRIVATE_DATA, &data,
			sizeof(intel_get_private_data)) != 0) {
		free(gInfo);
		return B_ERROR;
	}

	AreaCloner sharedCloner;
	gInfo->shared_info_area = sharedCloner.Clone("intel extreme shared info",
		(void**)&gInfo->shared_info, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		data.shared_info_area);
	status_t status = sharedCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	AreaCloner regsCloner;
	gInfo->regs_area = regsCloner.Clone("intel extreme regs",
		(void**)&gInfo->registers, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		gInfo->shared_info->registers_area);
	status = regsCloner.InitCheck();
	if (status < B_OK) {
		free(gInfo);
		return status;
	}

	sharedCloner.Keep();
	regsCloner.Keep();

	// The overlay registers, hardware status, and cursor memory share
	// a single area with the shared_info

	if (gInfo->shared_info->overlay_offset != 0) {
		gInfo->overlay_registers = (struct overlay_registers*)
			(gInfo->shared_info->graphics_memory
			+ gInfo->shared_info->overlay_offset);
	}

	if (gInfo->shared_info->device_type.InGroup(INTEL_GROUP_96x)) {
		// allocate some extra memory for the 3D context
		if (intel_allocate_memory(INTEL_i965_3D_CONTEXT_SIZE,
				B_APERTURE_NON_RESERVED, gInfo->context_base) == B_OK) {
			gInfo->context_offset = gInfo->context_base
				- (addr_t)gInfo->shared_info->graphics_memory;
		}
	}

	return B_OK;
}


/*! Clean up data common to both primary and cloned accelerant */
static void
uninit_common(void)
{
	intel_free_memory(gInfo->context_base);

	delete_area(gInfo->regs_area);
	delete_area(gInfo->shared_info_area);

	gInfo->regs_area = gInfo->shared_info_area = -1;

	// close the file handle ONLY if we're the clone
	if (gInfo->is_clone)
		close(gInfo->device);

	free(gInfo);
}


static void
dump_ports()
{
	if (gInfo->port_count == 0) {
		TRACE("%s: No ports connected\n", __func__);
		return;
	}

	TRACE("%s: Connected ports: (port_count: %" B_PRIu32 ")\n", __func__,
		gInfo->port_count);

	for (uint32 i = 0; i < gInfo->port_count; i++) {
		Port* port = gInfo->ports[i];
		if (!port) {
			TRACE("port %" B_PRIu32 ":: INVALID ALLOC!\n", i);
			continue;
		}
		TRACE("port %" B_PRIu32 ": %s %s\n", i, port->PortName(),
			port->IsConnected() ? "connected" : "disconnected");
	}
}


static bool
has_connected_port(port_index portIndex, uint32 type)
{
	for (uint32 i = 0; i < gInfo->port_count; i++) {
		Port* port = gInfo->ports[i];
		if (type != INTEL_PORT_TYPE_ANY && port->Type() != type)
			continue;
		if (portIndex != INTEL_PORT_ANY && port->PortIndex() != portIndex)
			continue;

		return true;
	}

	return false;
}


static status_t
probe_ports()
{
	// Try to determine what ports to use. We use the following heuristic:
	// * Check for DisplayPort, these can be more or less detected reliably.
	// * Check for HDMI, it'll fail on devices not having HDMI for us to fall
	//   back to DVI.
	// * Assume DVI B if no HDMI and no DisplayPort is present, confirmed by
	//   reading EDID in the IsConnected() call.
	// * Check for analog if possible (there's a detection bit on PCH),
	//   otherwise the assumed presence is confirmed by reading EDID in
	//   IsConnected().

	TRACE("adpa: %08" B_PRIx32 "\n", read32(INTEL_ANALOG_PORT));
	TRACE("dova: %08" B_PRIx32 ", dovb: %08" B_PRIx32
		", dovc: %08" B_PRIx32 "\n", read32(INTEL_DIGITAL_PORT_A),
		read32(INTEL_DIGITAL_PORT_B), read32(INTEL_DIGITAL_PORT_C));
	TRACE("lvds: %08" B_PRIx32 "\n", read32(INTEL_DIGITAL_LVDS_PORT));

	gInfo->port_count = 0;
	for (int i = INTEL_PORT_B; i <= INTEL_PORT_D; i++) {
		Port* displayPort = new(std::nothrow) DisplayPort((port_index)i);
		if (displayPort == NULL)
			return B_NO_MEMORY;

		if (displayPort->IsConnected())
			gInfo->ports[gInfo->port_count++] = displayPort;
		else
			delete displayPort;
	}

	for (int i = INTEL_PORT_B; i <= INTEL_PORT_D; i++) {
		if (has_connected_port((port_index)i, INTEL_PORT_TYPE_ANY)) {
			// we overlap with a DisplayPort, this is not HDMI
			continue;
		}

		Port* hdmiPort = new(std::nothrow) HDMIPort((port_index)i);
		if (hdmiPort == NULL)
			return B_NO_MEMORY;

		if (hdmiPort->IsConnected())
			gInfo->ports[gInfo->port_count++] = hdmiPort;
		else
			delete hdmiPort;
	}

	if (!has_connected_port(INTEL_PORT_ANY, INTEL_PORT_TYPE_ANY)) {
		// there's neither DisplayPort nor HDMI so far, assume DVI B
		Port* dviPort = new(std::nothrow) DigitalPort(INTEL_PORT_B);
		if (dviPort == NULL)
			return B_NO_MEMORY;

		if (dviPort->IsConnected()) {
			gInfo->ports[gInfo->port_count++] = dviPort;
			gInfo->head_mode |= HEAD_MODE_B_DIGITAL;
		} else
			delete dviPort;
	}

	// always try the LVDS port, it'll simply fail if not applicable
	Port* lvdsPort = new(std::nothrow) LVDSPort();
	if (lvdsPort == NULL)
		return B_NO_MEMORY;
	if (lvdsPort->IsConnected()) {
		gInfo->ports[gInfo->port_count++] = lvdsPort;
		gInfo->head_mode |= HEAD_MODE_LVDS_PANEL;
		gInfo->head_mode |= HEAD_MODE_B_DIGITAL;
	} else
		delete lvdsPort;

	// also always try eDP, it'll also just fail if not applicable
	Port* eDPPort = new(std::nothrow) EmbeddedDisplayPort();
	if (eDPPort == NULL)
		return B_NO_MEMORY;
	if (eDPPort->IsConnected())
		gInfo->ports[gInfo->port_count++] = eDPPort;
	else
		delete eDPPort;

	// then finally always try the analog port
	Port* analogPort = new(std::nothrow) AnalogPort();
	if (analogPort == NULL)
		return B_NO_MEMORY;
	if (analogPort->IsConnected()) {
		gInfo->ports[gInfo->port_count++] = analogPort;
		gInfo->head_mode |= HEAD_MODE_A_ANALOG;
	} else
		delete analogPort;

	if (gInfo->port_count == 0)
		return B_ERROR;

	return B_OK;
}


static status_t
assign_pipes()
{
	uint32 assigned = 0;

	// TODO: At some point we should "group" ports to pipes with the same mode.
	// You can drive multiple ports from a single pipe as long as the mode is
	// the same. For the moment we could get displays with the wrong pipes
	// assigned when the count is > 1;

    for (uint32 i = 0; i < gInfo->port_count; i++) {
        if (gInfo->ports[i] == NULL)
            continue;

		if (gInfo->ports[i]->PipePreference() != INTEL_PIPE_ANY) {
			// Some ports *really* need to be assigned a pipe due to
			// implementation bugs.
			gInfo->ports[i]->AssignPipe(gInfo->ports[i]->PipePreference());
			continue;
		}

		if (gInfo->ports[i]->IsConnected()) {
			pipe_index currentPipe = INTEL_PIPE_A;
			if (assigned == 1)
				currentPipe = INTEL_PIPE_B;
			else if (assigned > 2) {
				ERROR("%s: No pipes left to assign to port %s!\n", __func__,
					gInfo->ports[i]->PortName());
				continue;
			}

			gInfo->ports[i]->AssignPipe(currentPipe);
			assigned++;
		}
    }

	return B_OK;
}


//	#pragma mark - public accelerant functions


/*! Init primary accelerant */
status_t
intel_init_accelerant(int device)
{
	CALLED();

	status_t status = init_common(device, false);
	if (status != B_OK)
		return status;

	intel_shared_info &info = *gInfo->shared_info;

	init_lock(&info.accelerant_lock, "intel extreme accelerant");
	init_lock(&info.engine_lock, "intel extreme engine");

	setup_ring_buffer(info.primary_ring_buffer, "intel primary ring buffer");

	TRACE("pipe control for: 0x%" B_PRIx32 " 0x%" B_PRIx32 "\n",
		read32(INTEL_PIPE_CONTROL), read32(INTEL_PIPE_CONTROL));

	// Probe all ports
	status = probe_ports();

	// On TRACE, dump ports and states
	dump_ports();

	if (status != B_OK)
		ERROR("Warning: zero active displays were found!\n");

	status = assign_pipes();

	if (status != B_OK)
		ERROR("Warning: error while assigning pipes!\n");

	status = create_mode_list();
	if (status != B_OK) {
		uninit_common();
		return status;
	}

	return B_OK;
}


ssize_t
intel_accelerant_clone_info_size(void)
{
	CALLED();
	// clone info is device name, so return its maximum size
	return B_PATH_NAME_LENGTH;
}


void
intel_get_accelerant_clone_info(void* info)
{
	CALLED();
	ioctl(gInfo->device, INTEL_GET_DEVICE_NAME, info, B_PATH_NAME_LENGTH);
}


status_t
intel_clone_accelerant(void* info)
{
	CALLED();

	// create full device name
	char path[B_PATH_NAME_LENGTH];
	strcpy(path, "/dev/");
#ifdef __HAIKU__
	strlcat(path, (const char*)info, sizeof(path));
#else
	strcat(path, (const char*)info);
#endif

	int fd = open(path, B_READ_WRITE);
	if (fd < 0)
		return errno;

	status_t status = init_common(fd, true);
	if (status != B_OK)
		goto err1;

	// get read-only clone of supported display modes
	status = gInfo->mode_list_area = clone_area(
		"intel extreme cloned modes", (void**)&gInfo->mode_list,
		B_ANY_ADDRESS, B_READ_AREA, gInfo->shared_info->mode_list_area);
	if (status < B_OK)
		goto err2;

	return B_OK;

err2:
	uninit_common();
err1:
	close(fd);
	return status;
}


/*! This function is called for both, the primary accelerant and all of
	its clones.
*/
void
intel_uninit_accelerant(void)
{
	CALLED();

	// delete accelerant instance data
	delete_area(gInfo->mode_list_area);
	gInfo->mode_list = NULL;

	intel_shared_info &info = *gInfo->shared_info;

	uninit_lock(&info.accelerant_lock);
	uninit_lock(&info.engine_lock);

	uninit_ring_buffer(info.primary_ring_buffer);

	uninit_common();
}


status_t
intel_get_accelerant_device_info(accelerant_device_info* info)
{
	CALLED();

	info->version = B_ACCELERANT_VERSION;

	DeviceType* type = &gInfo->shared_info->device_type;

	if (type->InFamily(INTEL_FAMILY_7xx))
		strcpy(info->name, "Intel Extreme Graphics");
	else if (type->InFamily(INTEL_FAMILY_8xx))
		strcpy(info->name, "Intel Extreme Graphics 2");
	else if (type->InFamily(INTEL_FAMILY_9xx))
		strcpy(info->name, "Intel Graphics Media Accelerator");
	else if (type->InFamily(INTEL_FAMILY_SER5))
		strcpy(info->name, "Intel HD/Iris Graphics");
	else if (type->InFamily(INTEL_FAMILY_POVR))
		strcpy(info->name, "Intel PowerVR Graphics");
	else if (type->InFamily(INTEL_FAMILY_SOC0))
		strcpy(info->name, "Intel Atom Graphics");

	strcpy(info->chipset, gInfo->shared_info->device_identifier);
	strcpy(info->serial_no, "None");

	info->memory = gInfo->shared_info->graphics_memory_size;
	info->dac_speed = gInfo->shared_info->pll_info.max_frequency;

	return B_OK;
}


sem_id
intel_accelerant_retrace_semaphore()
{
	CALLED();
	return gInfo->shared_info->vblank_sem;
}

