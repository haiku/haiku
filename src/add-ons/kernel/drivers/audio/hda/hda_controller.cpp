/*
 * Copyright 2007-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "hda_controller_defs.h"

#include <algorithm>

#include <vm/vm.h>

#include "driver.h"
#include "hda_codec_defs.h"


#define MAKE_RATE(base, multiply, divide) \
	((base == 44100 ? FORMAT_44_1_BASE_RATE : 0) \
		| ((multiply - 1) << FORMAT_MULTIPLY_RATE_SHIFT) \
		| ((divide - 1) << FORMAT_DIVIDE_RATE_SHIFT))

#define HDAC_INPUT_STREAM_OFFSET(controller, index) \
	((index) * HDAC_STREAM_SIZE)
#define HDAC_OUTPUT_STREAM_OFFSET(controller, index) \
	(((controller)->num_input_streams + (index)) * HDAC_STREAM_SIZE)
#define HDAC_BIDIR_STREAM_OFFSET(controller, index) \
	(((controller)->num_input_streams + (controller)->num_output_streams \
		+ (index)) * HDAC_STREAM_SIZE)

#define ALIGN(size, align)	(((size) + align - 1) & ~(align - 1))


#define PCI_VENDOR_ATI			0x1002
#define PCI_VENDOR_AMD			0x1022
#define PCI_VENDOR_CREATIVE		0x1102
#define PCI_VENDOR_CMEDIA		0x13f6
#define PCI_VENDOR_INTEL		0x8086
#define PCI_VENDOR_NVIDIA		0x10de
#define PCI_VENDOR_VMWARE		0x15ad
#define PCI_VENDOR_SIS			0x1039
#define PCI_ALL_DEVICES			0xffffffff

#define HDA_QUIRK_SNOOP					0x0001
#define HDA_QUIRK_NO_MSI				0x0002
#define HDA_QUIRK_NO_CORBRP_RESET_ACK	0x0004
#define HDA_QUIRK_NOTCSEL				0x0008


static const struct {
	uint32 vendor_id, device_id;
	uint32 quirks;
} kControllerQuirks[] = {
	{ PCI_VENDOR_INTEL, 0x02c8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x06c8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x080a, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x0a0c, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x0c0c, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x0d0c, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x0f04, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x160c, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x1c20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x1d20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x1e20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x2284, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x3198, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x34c8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x38c8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x3b56, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x3b57, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x4b55, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x4d55, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x4dc8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x5a98, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x811b, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x8c20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x8ca0, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x8d20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x8d21, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9c20, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9c21, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9ca0, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9d70, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9d71, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0x9dc8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa0c8, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa170, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa171, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa1f0, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa270, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa2f0, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa348, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_INTEL, 0xa3f0, HDA_QUIRK_SNOOP },
	{ PCI_VENDOR_ATI, 0x437b, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_ATI, 0x4383, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_AMD, 0x157a, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_AMD, 0x780d, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_AMD, 0x1457, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_AMD, 0x1487, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	{ PCI_VENDOR_AMD, 0x15e3, HDA_QUIRK_SNOOP | HDA_QUIRK_NOTCSEL },
	// Enable snooping for Nvidia, right now for all their hda-devices,
	// but only based on guessing.
	{ PCI_VENDOR_NVIDIA, PCI_ALL_DEVICES, HDA_QUIRK_SNOOP | HDA_QUIRK_NO_MSI
		| HDA_QUIRK_NO_CORBRP_RESET_ACK },
	{ PCI_VENDOR_CMEDIA, 0x5011, HDA_QUIRK_NO_MSI },
	{ PCI_VENDOR_CREATIVE, 0x0010, HDA_QUIRK_NO_MSI },
	{ PCI_VENDOR_CREATIVE, 0x0012, HDA_QUIRK_NO_MSI },
	{ PCI_VENDOR_VMWARE, PCI_ALL_DEVICES, HDA_QUIRK_NO_CORBRP_RESET_ACK },
	{ PCI_VENDOR_SIS, 0x7502, HDA_QUIRK_NO_CORBRP_RESET_ACK },
};


static const struct {
	uint32 multi_rate;
	uint32 hw_rate;
	uint32 rate;
} kRates[] = {
	{B_SR_8000, MAKE_RATE(48000, 1, 6), 8000},
	{B_SR_11025, MAKE_RATE(44100, 1, 4), 11025},
	{B_SR_16000, MAKE_RATE(48000, 1, 3), 16000},
	{B_SR_22050, MAKE_RATE(44100, 1, 2), 22050},
	{B_SR_32000, MAKE_RATE(48000, 2, 3), 32000},
	{B_SR_44100, MAKE_RATE(44100, 1, 1), 44100},
	{B_SR_48000, MAKE_RATE(48000, 1, 1), 48000},
	{B_SR_88200, MAKE_RATE(44100, 2, 1), 88200},
	{B_SR_96000, MAKE_RATE(48000, 2, 1), 96000},
	{B_SR_176400, MAKE_RATE(44100, 4, 1), 176400},
	{B_SR_192000, MAKE_RATE(48000, 4, 1), 192000},
	// this one is not supported by hardware.
	// {B_SR_384000, MAKE_RATE(44100, ??, ??), 384000},
};


static uint32
get_controller_quirks(const pci_info& info)
{
	for (size_t i = 0;
			i < sizeof(kControllerQuirks) / sizeof(kControllerQuirks[0]); i++) {
		if (info.vendor_id == kControllerQuirks[i].vendor_id
			&& (kControllerQuirks[i].device_id == PCI_ALL_DEVICES
				|| kControllerQuirks[i].device_id == info.device_id))
			return kControllerQuirks[i].quirks;
	}
	return 0;
}


template<int bits, typename base_type>
bool
wait_for_bits(base_type base, uint32 reg, uint32 mask, bool set,
	bigtime_t delay = 100, int timeout = 10)
{
	STATIC_ASSERT(bits == 8 || bits == 16 || bits == 32);

	for (; timeout >= 0; timeout--) {
		snooze(delay);

		uint32 value;
		switch (bits) {
			case 8:
				value = base->Read8(reg);
				break;
			case 16:
				value = base->Read16(reg);
				break;
			case 32:
				value = base->Read32(reg);
				break;
		}

		if (((value & mask) != 0) == set)
			return true;
	}

	return false;
}


static inline bool
update_pci_register(hda_controller* controller, uint8 reg, uint32 mask,
	uint32 value, uint8 size, bool check = false)
{
	uint32 originalValue = (gPci->read_pci_config)(controller->pci_info.bus,
		controller->pci_info.device, controller->pci_info.function, reg, size);
	(gPci->write_pci_config)(controller->pci_info.bus,
		controller->pci_info.device, controller->pci_info.function,
		reg, size, (originalValue & mask) | value);

	if (!check)
		return true;

	uint32 newValue = (gPci->read_pci_config)(controller->pci_info.bus,
		controller->pci_info.device, controller->pci_info.function, reg, size);
	return (newValue & ~mask) == value;
}


static inline rirb_t&
current_rirb(hda_controller* controller)
{
	return controller->rirb[controller->rirb_read_pos];
}


static inline uint32
next_rirb(hda_controller* controller)
{
	return (controller->rirb_read_pos + 1) % controller->rirb_length;
}


static inline uint32
next_corb(hda_controller* controller)
{
	return (controller->corb_write_pos + 1) % controller->corb_length;
}


/*! Called with interrupts off.
	Returns \c true, if the scheduler shall be invoked.
*/
static bool
stream_handle_interrupt(hda_controller* controller, hda_stream* stream,
	uint32 index)
{
	if (!stream->running)
		return false;

	uint8 status = stream->Read8(HDAC_STREAM_STATUS);
	if (status == 0)
		return false;

	stream->Write8(HDAC_STREAM_STATUS, status);

	if ((status & STATUS_FIFO_ERROR) != 0)
		dprintf("hda: stream fifo error (id:%" B_PRIu32 ")\n", stream->id);
	if ((status & STATUS_DESCRIPTOR_ERROR) != 0) {
		dprintf("hda: stream descriptor error (id:%" B_PRIu32 ")\n",
			stream->id);
	}

	if ((status & STATUS_BUFFER_COMPLETED) == 0) {
		dprintf("hda: stream buffer not completed (id:%" B_PRIu32 ")\n",
			stream->id);
		return false;
	}

	// Normally we should use the DMA position for the stream. Apparently there
	// are broken chipsets, which don't support it correctly. If we detect this,
	// we switch to using the LPIB instead. The link position is ahead of the
	// DMA position for recording and behind for playback streams, but just
	// for determining the currently active buffer, it should be good enough.
	if (stream->use_dma_position && stream->incorrect_position_count >= 32) {
		dprintf("hda: DMA position for stream (id:%" B_PRIu32 ") seems to be "
			"broken. Switching to using LPIB.\n", stream->id);
		stream->use_dma_position = false;
	}

	// Determine the buffer we're switching to. Some chipsets seem to trigger
	// the interrupt before the DMA position in memory has been updated. We
	// round it, so we still get the right buffer.
	uint32 dmaPosition = stream->use_dma_position
		? controller->stream_positions[index * 2]
		: stream->Read32(HDAC_STREAM_POSITION);
	uint32 bufferIndex = ((dmaPosition + stream->buffer_size / 2)
		/ stream->buffer_size) % stream->num_buffers;

	// get the current recording/playing position and the system time
	uint32 linkBytePosition = stream->Read32(HDAC_STREAM_POSITION);
	bigtime_t now = system_time();

	// compute the frame position for the byte position
	uint32 linkFramePosition = 0;
	while (linkBytePosition >= stream->buffer_size) {
		linkFramePosition += stream->buffer_length;
		linkBytePosition -= stream->buffer_size;
	}
	linkFramePosition += std::min(
		linkBytePosition / (stream->num_channels * stream->sample_size),
		stream->buffer_length);

	// compute the number of frames processed since the previous interrupt
	int32 framesProcessed = (int32)linkFramePosition
		- (int32)stream->last_link_frame_position;
	if (framesProcessed < 0)
		framesProcessed += stream->num_buffers * stream->buffer_length;
	stream->last_link_frame_position = linkFramePosition;

	// update stream playing/recording state and notify buffer_exchange()
	acquire_spinlock(&stream->lock);

	if (bufferIndex == (stream->buffer_cycle + 1) % stream->num_buffers)
		stream->incorrect_position_count = 0;
	else
		stream->incorrect_position_count++;

	stream->real_time = now;
	stream->frames_count += framesProcessed;
	stream->buffer_cycle = bufferIndex;

	release_spinlock(&stream->lock);

	release_sem_etc(controller->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);

	return true;
}


static int32
hda_interrupt_handler(hda_controller* controller)
{
	int32 handled = B_HANDLED_INTERRUPT;

	// Check if this interrupt is ours
	uint32 intrStatus = controller->Read32(HDAC_INTR_STATUS);
	if ((intrStatus & INTR_STATUS_GLOBAL) == 0)
		return B_UNHANDLED_INTERRUPT;

	// Controller or stream related?
	if (intrStatus & INTR_STATUS_CONTROLLER) {
		uint8 rirbStatus = controller->Read8(HDAC_RIRB_STATUS);
		uint8 corbStatus = controller->Read8(HDAC_CORB_STATUS);

		// Check for incoming responses
		if (rirbStatus) {
			controller->Write8(HDAC_RIRB_STATUS, rirbStatus);

			if ((rirbStatus & RIRB_STATUS_RESPONSE) != 0) {
				uint16 writePos = (controller->Read16(HDAC_RIRB_WRITE_POS) + 1)
					% controller->rirb_length;

				for (; controller->rirb_read_pos != writePos;
						controller->rirb_read_pos = next_rirb(controller)) {
					uint32 response = current_rirb(controller).response;
					uint32 responseFlags = current_rirb(controller).flags;
					uint32 cad = responseFlags & RESPONSE_FLAGS_CODEC_MASK;
					hda_codec* codec = controller->codecs[cad];

					if (codec == NULL) {
						dprintf("hda: Response for unknown codec %" B_PRIu32
							": %08" B_PRIx32 "/%08" B_PRIx32 "\n", cad,
							response, responseFlags);
						continue;
					}

					if ((responseFlags & RESPONSE_FLAGS_UNSOLICITED) != 0) {
						dprintf("hda: Unsolicited response: %08" B_PRIx32
							"/%08" B_PRIx32 "\n", response, responseFlags);
						codec->unsol_responses[codec->unsol_response_write++] =
							response;
						codec->unsol_response_write %= MAX_CODEC_UNSOL_RESPONSES;
						release_sem_etc(codec->unsol_response_sem, 1,
							B_DO_NOT_RESCHEDULE);
						handled = B_INVOKE_SCHEDULER;
						continue;
					}
					if (codec->response_count >= MAX_CODEC_RESPONSES) {
						dprintf("hda: too many responses received for codec %"
							B_PRIu32 ": %08" B_PRIx32 "/%08" B_PRIx32 "!\n",
							cad, response, responseFlags);
						continue;
					}

					// Store response in codec
					codec->responses[codec->response_count++] = response;
					release_sem_etc(codec->response_sem, 1, B_DO_NOT_RESCHEDULE);
					handled = B_INVOKE_SCHEDULER;
				}
			}

			if ((rirbStatus & RIRB_STATUS_OVERRUN) != 0)
				dprintf("hda: RIRB Overflow\n");
		}

		// Check for sending errors
		if (corbStatus) {
			controller->Write8(HDAC_CORB_STATUS, corbStatus);

			if ((corbStatus & CORB_STATUS_MEMORY_ERROR) != 0)
				dprintf("hda: CORB Memory Error!\n");
		}
	}

	if ((intrStatus & INTR_STATUS_STREAM_MASK) != 0) {
		for (uint32 index = 0; index < HDA_MAX_STREAMS; index++) {
			if ((intrStatus & (1 << index)) != 0) {
				if (controller->streams[index]) {
					if (stream_handle_interrupt(controller,
							controller->streams[index], index)) {
						handled = B_INVOKE_SCHEDULER;
					}
				} else {
					dprintf("hda: Stream interrupt for unconfigured stream "
						"%" B_PRIu32 "!\n", index);
				}
			}
		}
	}

	// NOTE: See HDA001 => CIS/GIS cannot be cleared!

	return handled;
}


static status_t
reset_controller(hda_controller* controller)
{
	uint32 control = controller->Read32(HDAC_GLOBAL_CONTROL);
	if ((control & GLOBAL_CONTROL_RESET) != 0) {
		controller->Write32(HDAC_INTR_CONTROL, 0);

		// stop streams

		for (uint32 i = 0; i < controller->num_input_streams; i++) {
			controller->Write8(HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE
				+ HDAC_INPUT_STREAM_OFFSET(controller, i), 0);
			controller->Write8(HDAC_STREAM_STATUS + HDAC_STREAM_BASE
				+ HDAC_INPUT_STREAM_OFFSET(controller, i), 0);
		}
		for (uint32 i = 0; i < controller->num_output_streams; i++) {
			controller->Write8(HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE
				+ HDAC_OUTPUT_STREAM_OFFSET(controller, i), 0);
			controller->Write8(HDAC_STREAM_STATUS + HDAC_STREAM_BASE
				+ HDAC_OUTPUT_STREAM_OFFSET(controller, i), 0);
		}
		for (uint32 i = 0; i < controller->num_bidir_streams; i++) {
			controller->Write8(HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE
				+ HDAC_BIDIR_STREAM_OFFSET(controller, i), 0);
			controller->Write8(HDAC_STREAM_STATUS + HDAC_STREAM_BASE
				+ HDAC_BIDIR_STREAM_OFFSET(controller, i), 0);
		}

		// stop DMA
		controller->ReadModifyWrite8(HDAC_CORB_CONTROL, HDAC_CORB_CONTROL_MASK,
			0);
		controller->ReadModifyWrite8(HDAC_RIRB_CONTROL, HDAC_RIRB_CONTROL_MASK,
			0);

		if (!wait_for_bits<8>(controller, HDAC_CORB_CONTROL, ~0, false)
			|| !wait_for_bits<8>(controller, HDAC_RIRB_CONTROL, ~0, false)) {
			dprintf("hda: unable to stop dma\n");
			return B_BUSY;
		}

		// reset DMA position buffer
		controller->Write32(HDAC_DMA_POSITION_BASE_LOWER, 0);
		controller->Write32(HDAC_DMA_POSITION_BASE_UPPER, 0);

		control = controller->Read32(HDAC_GLOBAL_CONTROL);
	}

	// Set reset bit - it must be asserted for at least 100us
	controller->Write32(HDAC_GLOBAL_CONTROL, control & ~GLOBAL_CONTROL_RESET);
	if (!wait_for_bits<32>(controller, HDAC_GLOBAL_CONTROL,
			GLOBAL_CONTROL_RESET, false)) {
		dprintf("hda: unable to reset controller\n");
		return B_BUSY;
	}

	// Wait for codec PLL to lock at least 100us, section 5.5.1.2
	snooze(1000);

	// Unset reset bit

	control = controller->Read32(HDAC_GLOBAL_CONTROL);
	controller->Write32(HDAC_GLOBAL_CONTROL, control | GLOBAL_CONTROL_RESET);
	if (!wait_for_bits<32>(controller, HDAC_GLOBAL_CONTROL,
			GLOBAL_CONTROL_RESET, true)) {
		dprintf("hda: unable to exit reset\n");
		return B_BUSY;
	}

	// Wait for codecs to finish their own reset (apparently needs more
	// time than documented in the specs)
	snooze(1000);

	// Enable unsolicited responses
	control = controller->Read32(HDAC_GLOBAL_CONTROL);
	controller->Write32(HDAC_GLOBAL_CONTROL,
		control | GLOBAL_CONTROL_UNSOLICITED);

	return B_OK;
}


/*! Allocates and initializes the Command Output Ring Buffer (CORB), and
	Response Input Ring Buffer (RIRB) to the maximum supported size, and also
	the DMA position buffer.

	Programs the controller hardware to make use of these buffers (the DMA
	positioning is actually enabled in hda_stream_setup_buffers()).
*/
static status_t
init_corb_rirb_pos(hda_controller* controller, uint32 quirks)
{
	// Determine and set size of CORB
	uint8 corbSize = controller->Read8(HDAC_CORB_SIZE);
	if ((corbSize & CORB_SIZE_CAP_256_ENTRIES) != 0) {
		controller->corb_length = 256;
		controller->ReadModifyWrite8(
			HDAC_CORB_SIZE, HDAC_CORB_SIZE_MASK,
			CORB_SIZE_256_ENTRIES);
	} else if (corbSize & CORB_SIZE_CAP_16_ENTRIES) {
		controller->corb_length = 16;
		controller->ReadModifyWrite8(
			HDAC_CORB_SIZE, HDAC_CORB_SIZE_MASK,
			CORB_SIZE_16_ENTRIES);
	} else if (corbSize & CORB_SIZE_CAP_2_ENTRIES) {
		controller->corb_length = 2;
		controller->ReadModifyWrite8(
			HDAC_CORB_SIZE, HDAC_CORB_SIZE_MASK,
			CORB_SIZE_2_ENTRIES);
	}

	// Determine and set size of RIRB
	uint8 rirbSize = controller->Read8(HDAC_RIRB_SIZE);
	if (rirbSize & RIRB_SIZE_CAP_256_ENTRIES) {
		controller->rirb_length = 256;
		controller->ReadModifyWrite8(
			HDAC_RIRB_SIZE, HDAC_RIRB_SIZE_MASK,
			RIRB_SIZE_256_ENTRIES);
	} else if (rirbSize & RIRB_SIZE_CAP_16_ENTRIES) {
		controller->rirb_length = 16;
		controller->ReadModifyWrite8(
			HDAC_RIRB_SIZE, HDAC_RIRB_SIZE_MASK,
			RIRB_SIZE_16_ENTRIES);
	} else if (rirbSize & RIRB_SIZE_CAP_2_ENTRIES) {
		controller->rirb_length = 2;
		controller->ReadModifyWrite8(
			HDAC_RIRB_SIZE, HDAC_RIRB_SIZE_MASK,
			RIRB_SIZE_2_ENTRIES);
	}

	// Determine rirb offset in memory and total size of corb+alignment+rirb
	uint32 rirbOffset = ALIGN(controller->corb_length * sizeof(corb_t), 128);
	uint32 posOffset = ALIGN(rirbOffset
		+ controller->rirb_length * sizeof(rirb_t), 128);
	uint8 posSize = 8 * (controller->num_input_streams
		+ controller->num_output_streams + controller->num_bidir_streams);

	uint32 memSize = PAGE_ALIGN(posOffset + posSize);

	// Allocate memory area
	controller->corb_rirb_pos_area = create_area("hda corb/rirb/pos",
		(void**)&controller->corb, B_ANY_KERNEL_ADDRESS, memSize,
		controller->is_64_bit ? B_CONTIGUOUS : B_32_BIT_CONTIGUOUS, 0);
	if (controller->corb_rirb_pos_area < 0)
		return controller->corb_rirb_pos_area;

	// Rirb is after corb+aligment
	controller->rirb = (rirb_t*)(((uint8*)controller->corb) + rirbOffset);

	physical_entry pe;
	status_t status = get_memory_map(controller->corb, memSize, &pe, 1);
	if (status != B_OK) {
		delete_area(controller->corb_rirb_pos_area);
		return status;
	}

	if (!controller->dma_snooping) {
		vm_set_area_memory_type(controller->corb_rirb_pos_area,
			pe.address, B_MTR_UC);
	}

	// Program CORB/RIRB for these locations
	controller->Write32(HDAC_CORB_BASE_LOWER, (uint32)pe.address);
	if (controller->is_64_bit) {
		controller->Write32(HDAC_CORB_BASE_UPPER,
			(uint32)((uint64)pe.address >> 32));
	}

	controller->Write32(HDAC_RIRB_BASE_LOWER, (uint32)pe.address + rirbOffset);
	if (controller->is_64_bit) {
		controller->Write32(HDAC_RIRB_BASE_UPPER,
			(uint32)(((uint64)pe.address + rirbOffset) >> 32));
	}

	// Program DMA position update
	controller->Write32(HDAC_DMA_POSITION_BASE_LOWER,
		(uint32)pe.address + posOffset);
	if (controller->is_64_bit) {
		controller->Write32(HDAC_DMA_POSITION_BASE_UPPER,
			(uint32)(((uint64)pe.address + posOffset) >> 32));
	}

	controller->stream_positions = (uint32*)
		((uint8*)controller->corb + posOffset);

	controller->ReadModifyWrite16(HDAC_CORB_WRITE_POS,
		HDAC_CORB_WRITE_POS_MASK, 0);

	// Reset CORB read pointer. Preseve bits marked as RsvdP.
	// After setting the reset bit, we must wait for the hardware
	// to acknowledge it, then manually unset it and wait for that
	// to be acknowledged as well.
	uint16 corbReadPointer = controller->Read16(HDAC_CORB_READ_POS);

	corbReadPointer |= CORB_READ_POS_RESET;
	controller->Write16(HDAC_CORB_READ_POS, corbReadPointer);
	if (!wait_for_bits<16>(controller, HDAC_CORB_READ_POS, CORB_READ_POS_RESET,
			true)) {
		dprintf("hda: CORB read pointer reset not acknowledged\n");

		// According to HDA spec v1.0a ch3.3.21, software must read the
		// bit as 1 to verify that the reset completed, but not all HDA
		// controllers follow that...
		if ((quirks & HDA_QUIRK_NO_CORBRP_RESET_ACK) == 0)
			return B_BUSY;
	}

	corbReadPointer &= ~CORB_READ_POS_RESET;
	controller->Write16(HDAC_CORB_READ_POS, corbReadPointer);
	if (!wait_for_bits<16>(controller, HDAC_CORB_READ_POS, CORB_READ_POS_RESET,
			false)) {
		dprintf("hda: CORB read pointer reset failed\n");
		return B_BUSY;
	}

	// Reset RIRB write pointer
	controller->ReadModifyWrite16(HDAC_RIRB_WRITE_POS,
		RIRB_WRITE_POS_RESET, RIRB_WRITE_POS_RESET);

	// Generate interrupt for every response
	controller->ReadModifyWrite16(HDAC_RESPONSE_INTR_COUNT,
		HDAC_RESPONSE_INTR_COUNT_MASK, 1);

	// Setup cached read/write indices
	controller->rirb_read_pos = 1;
	controller->corb_write_pos = 0;

	// Gentlemen, start your engines...
	controller->ReadModifyWrite8(HDAC_CORB_CONTROL,
		HDAC_CORB_CONTROL_MASK,
		CORB_CONTROL_RUN | CORB_CONTROL_MEMORY_ERROR_INTR);
	controller->ReadModifyWrite8(HDAC_RIRB_CONTROL,
		HDAC_RIRB_CONTROL_MASK,
		RIRB_CONTROL_DMA_ENABLE | RIRB_CONTROL_OVERRUN_INTR
		| RIRB_CONTROL_RESPONSE_INTR);

	return B_OK;
}


//	#pragma mark - public stream functions


void
hda_stream_delete(hda_stream* stream)
{
	if (stream->buffer_area >= 0)
		delete_area(stream->buffer_area);

	if (stream->buffer_descriptors_area >= 0)
		delete_area(stream->buffer_descriptors_area);

	free(stream);
}


hda_stream*
hda_stream_new(hda_audio_group* audioGroup, int type)
{
	hda_controller* controller = audioGroup->codec->controller;

	hda_stream* stream = (hda_stream*)calloc(1, sizeof(hda_stream));
	if (stream == NULL)
		return NULL;

	stream->buffer_area = B_ERROR;
	stream->buffer_descriptors_area = B_ERROR;
	stream->type = type;
	stream->controller = controller;
	stream->incorrect_position_count = 0;
	stream->use_dma_position = true;

	switch (type) {
		case STREAM_PLAYBACK:
			stream->id = 1;
			stream->offset = HDAC_OUTPUT_STREAM_OFFSET(controller, 0);
			break;

		case STREAM_RECORD:
			stream->id = 2;
			stream->offset = HDAC_INPUT_STREAM_OFFSET(controller, 0);
			break;

		default:
			dprintf("%s: Unknown stream type %d!\n", __func__, type);
			free(stream);
			return NULL;
	}

	// find I/O and Pin widgets for this stream

	if (hda_audio_group_get_widgets(audioGroup, stream) == B_OK) {
		switch (type) {
			case STREAM_PLAYBACK:
				controller->streams[controller->num_input_streams] = stream;
				break;
			case STREAM_RECORD:
				controller->streams[0] = stream;
				break;
		}

		return stream;
	}

	dprintf("hda: hda_audio_group_get_widgets failed for %s stream\n",
		type == STREAM_PLAYBACK ? " playback" : "record");

	free(stream);
	return NULL;
}


/*!	Starts a stream's DMA engine, and enables generating and receiving
	interrupts for this stream.
*/
status_t
hda_stream_start(hda_controller* controller, hda_stream* stream)
{
	dprintf("hda_stream_start() offset %" B_PRIx32 "\n", stream->offset);

	stream->frames_count = 0;
	stream->last_link_frame_position = 0;

	controller->Write32(HDAC_INTR_CONTROL, controller->Read32(HDAC_INTR_CONTROL)
		| (1 << (stream->offset / HDAC_STREAM_SIZE)));
	stream->Write8(HDAC_STREAM_CONTROL0, stream->Read8(HDAC_STREAM_CONTROL0)
		| CONTROL0_BUFFER_COMPLETED_INTR | CONTROL0_FIFO_ERROR_INTR
		| CONTROL0_DESCRIPTOR_ERROR_INTR | CONTROL0_RUN);

	if (!wait_for_bits<8>(stream, HDAC_STREAM_CONTROL0, CONTROL0_RUN, true)) {
		dprintf("hda: unable to start stream\n");
		return B_BUSY;
	}

	stream->running = true;
	return B_OK;
}


/*!	Stops the stream's DMA engine, and turns off interrupts for this
	stream.
*/
status_t
hda_stream_stop(hda_controller* controller, hda_stream* stream)
{
	dprintf("hda_stream_stop()\n");
	stream->Write8(HDAC_STREAM_CONTROL0, stream->Read8(HDAC_STREAM_CONTROL0)
		& ~(CONTROL0_BUFFER_COMPLETED_INTR | CONTROL0_FIFO_ERROR_INTR
			| CONTROL0_DESCRIPTOR_ERROR_INTR | CONTROL0_RUN));
	controller->Write32(HDAC_INTR_CONTROL, controller->Read32(HDAC_INTR_CONTROL)
		& ~(1 << (stream->offset / HDAC_STREAM_SIZE)));

	if (!wait_for_bits<8>(stream, HDAC_STREAM_CONTROL0, CONTROL0_RUN, false)) {
		dprintf("hda: unable to stop stream\n");
		return B_BUSY;
	}

	stream->running = false;
	return B_OK;
}


/*! Runs a stream through a reset cycle.
*/
status_t
hda_stream_reset(hda_stream* stream)
{
	if (stream->running)
		hda_stream_stop(stream->controller, stream);

	stream->Write8(HDAC_STREAM_CONTROL0,
		stream->Read8(HDAC_STREAM_CONTROL0) | CONTROL0_RESET);
	if (!wait_for_bits<8>(stream, HDAC_STREAM_CONTROL0, CONTROL0_RESET, true)) {
		dprintf("hda: unable to start stream reset\n");
		return B_BUSY;
	}

	stream->Write8(HDAC_STREAM_CONTROL0,
		stream->Read8(HDAC_STREAM_CONTROL0) & ~CONTROL0_RESET);
	if (!wait_for_bits<8>(stream, HDAC_STREAM_CONTROL0, CONTROL0_RESET, false))
	{
		dprintf("hda: unable to stop stream reset\n");
		return B_BUSY;
	}

	return B_OK;
}


status_t
hda_stream_setup_buffers(hda_audio_group* audioGroup, hda_stream* stream,
	const char* desc)
{
	hda_stream_reset(stream);

	// Clear previously allocated memory
	if (stream->buffer_area >= 0) {
		delete_area(stream->buffer_area);
		stream->buffer_area = B_ERROR;
	}

	if (stream->buffer_descriptors_area >= 0) {
		delete_area(stream->buffer_descriptors_area);
		stream->buffer_descriptors_area = B_ERROR;
	}

	// Find out stream format and sample rate
	uint16 format = (stream->num_channels - 1) & 0xf;
	switch (stream->sample_format) {
		case B_FMT_8BIT_S:	format |= FORMAT_8BIT; stream->bps = 8; break;
		case B_FMT_16BIT:	format |= FORMAT_16BIT; stream->bps = 16; break;
		case B_FMT_20BIT:	format |= FORMAT_20BIT; stream->bps = 20; break;
		case B_FMT_24BIT:	format |= FORMAT_24BIT; stream->bps = 24; break;
		case B_FMT_32BIT:	format |= FORMAT_32BIT; stream->bps = 32; break;

		default:
			dprintf("hda: Invalid sample format: 0x%" B_PRIx32 "\n",
				stream->sample_format);
			break;
	}

	for (uint32 index = 0; index < sizeof(kRates) / sizeof(kRates[0]); index++) {
		if (kRates[index].multi_rate == stream->sample_rate) {
			format |= kRates[index].hw_rate;
			stream->rate = kRates[index].rate;
			break;
		}
	}

	// Calculate size of buffer (aligned to 128 bytes)
	stream->buffer_size = ALIGN(stream->buffer_length * stream->num_channels
		* stream->sample_size, 128);

	dprintf("hda: sample size %" B_PRIu32 ", num channels %" B_PRIu32 ", "
		"buffer length %" B_PRIu32 "\n", stream->sample_size,
		stream->num_channels, stream->buffer_length);
	dprintf("hda: %s: setup stream %" B_PRIu32 ": SR=%" B_PRIu32 ", SF=%"
		B_PRIu32 " F=0x%x (0x%" B_PRIx32 ")\n", __func__, stream->id,
		stream->rate, stream->bps, format, stream->sample_format);

	// Calculate total size of all buffers (aligned to size of B_PAGE_SIZE)
	uint32 alloc = stream->buffer_size * stream->num_buffers;
	alloc = PAGE_ALIGN(alloc);

	// Allocate memory for buffers
	uint8* buffer;
	stream->buffer_area = create_area("hda buffers", (void**)&buffer,
		B_ANY_KERNEL_ADDRESS, alloc,
		stream->controller->is_64_bit ? B_CONTIGUOUS : B_32_BIT_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	// Get the physical address of memory
	physical_entry pe;
	status_t status = get_memory_map(buffer, alloc, &pe, 1);
	if (status != B_OK) {
		delete_area(stream->buffer_area);
		return status;
	}

	phys_addr_t bufferPhysicalAddress = pe.address;

	if (!stream->controller->dma_snooping) {
		vm_set_area_memory_type(stream->buffer_area,
			bufferPhysicalAddress, B_MTR_UC);
	}

	dprintf("hda: %s(%s): Allocated %" B_PRIu32 " bytes for %" B_PRIu32
		" buffers\n", __func__, desc, alloc, stream->num_buffers);

	// Store pointers (both virtual/physical)
	for (uint32 index = 0; index < stream->num_buffers; index++) {
		stream->buffers[index] = buffer + (index * stream->buffer_size);
		stream->physical_buffers[index] = bufferPhysicalAddress
			+ (index * stream->buffer_size);
	}

	// Now allocate BDL for buffer range
	uint32 bdlCount = stream->num_buffers;
	alloc = bdlCount * sizeof(bdl_entry_t);
	alloc = PAGE_ALIGN(alloc);

	bdl_entry_t* bufferDescriptors;
	stream->buffer_descriptors_area = create_area("hda buffer descriptors",
		(void**)&bufferDescriptors, B_ANY_KERNEL_ADDRESS, alloc,
		stream->controller->is_64_bit ? B_CONTIGUOUS : B_32_BIT_CONTIGUOUS, 0);
	if (stream->buffer_descriptors_area < B_OK) {
		delete_area(stream->buffer_area);
		return stream->buffer_descriptors_area;
	}

	// Get the physical address of memory
	status = get_memory_map(bufferDescriptors, alloc, &pe, 1);
	if (status != B_OK) {
		delete_area(stream->buffer_area);
		delete_area(stream->buffer_descriptors_area);
		return status;
	}

	stream->physical_buffer_descriptors = pe.address;

	if (!stream->controller->dma_snooping) {
		vm_set_area_memory_type(stream->buffer_descriptors_area,
			stream->physical_buffer_descriptors, B_MTR_UC);
	}

	dprintf("hda: %s(%s): Allocated %" B_PRIu32 " bytes for %" B_PRIu32
		" BDLEs\n", __func__, desc, alloc, bdlCount);

	// Setup buffer descriptor list (BDL) entries
	uint32 fragments = 0;
	for (uint32 index = 0; index < stream->num_buffers;
			index++, bufferDescriptors++) {
		bufferDescriptors->lower = (uint32)stream->physical_buffers[index];
		bufferDescriptors->upper
			= (uint32)((uint64)stream->physical_buffers[index] >> 32);
		fragments++;
		bufferDescriptors->length = stream->buffer_size;
		bufferDescriptors->ioc = 1;
			// we want an interrupt after every buffer
	}

	// Configure stream registers
	stream->Write16(HDAC_STREAM_FORMAT, format);
	stream->Write32(HDAC_STREAM_BUFFERS_BASE_LOWER,
		(uint32)stream->physical_buffer_descriptors);
	if (stream->controller->is_64_bit) {
		stream->Write32(HDAC_STREAM_BUFFERS_BASE_UPPER,
			(uint32)((uint64)stream->physical_buffer_descriptors >> 32));
	}

	stream->Write16(HDAC_STREAM_LAST_VALID, fragments - 1);
	// total cyclic buffer size in _bytes_
	stream->Write32(HDAC_STREAM_BUFFER_SIZE, stream->buffer_size
		* stream->num_buffers);
	stream->Write8(HDAC_STREAM_CONTROL2, stream->id << CONTROL2_STREAM_SHIFT);

	stream->controller->Write32(HDAC_DMA_POSITION_BASE_LOWER,
		stream->controller->Read32(HDAC_DMA_POSITION_BASE_LOWER)
		| DMA_POSITION_ENABLED);

	dprintf("hda: stream: %" B_PRIu32 " fifo size: %d num_io_widgets: %"
		B_PRIu32 "\n", stream->id, stream->Read16(HDAC_STREAM_FIFO_SIZE),
		stream->num_io_widgets);
	dprintf("hda: widgets: ");

	hda_codec* codec = audioGroup->codec;
	uint32 channelNum = 0;
	for (uint32 i = 0; i < stream->num_io_widgets; i++) {
		corb_t verb[2];
		verb[0] = MAKE_VERB(codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_FORMAT, format);
		uint32 val = stream->id << 4;
		if (channelNum < stream->num_channels)
			val |= channelNum;
		else
			val = 0;
		verb[1] = MAKE_VERB(codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_STREAM_CHANNEL, val);

		uint32 response[2];
		hda_send_verbs(codec, verb, response, 2);
		//channelNum += 2; // TODO stereo widget ? Every output gets the same stream for now
		dprintf("%" B_PRIu32 " ", stream->io_widgets[i]);

		hda_widget* widget = hda_audio_group_get_widget(audioGroup,
			stream->io_widgets[i]);
		if ((widget->capabilities.audio & AUDIO_CAP_DIGITAL) != 0) {
			verb[0] = MAKE_VERB(codec->addr, stream->io_widgets[i],
				VID_SET_DIGITAL_CONVERTER_CONTROL1, format);
			hda_send_verbs(codec, verb, response, 1);
		}
	}
	dprintf("\n");

	snooze(1000);
	return B_OK;
}


//	#pragma mark - public controller functions


status_t
hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, uint32 count)
{
	hda_controller* controller = codec->controller;
	uint32 sent = 0;

	codec->response_count = 0;

	while (sent < count) {
		uint32 readPos = controller->Read16(HDAC_CORB_READ_POS);
		uint32 queued = 0;

		while (sent < count) {
			uint32 writePos = next_corb(controller);

			if (writePos == readPos) {
				// There is no space left in the ring buffer; execute the
				// queued commands and wait until
				break;
			}

			controller->corb[writePos] = verbs[sent++];
			controller->corb_write_pos = writePos;
			queued++;
		}

		controller->Write16(HDAC_CORB_WRITE_POS, controller->corb_write_pos);
		status_t status = acquire_sem_etc(codec->response_sem, queued,
			B_RELATIVE_TIMEOUT, 50000ULL);
		if (status != B_OK)
			return status;
	}

	if (responses != NULL)
		memcpy(responses, codec->responses, count * sizeof(uint32));

	return B_OK;
}


status_t
hda_verb_write(hda_codec* codec, uint32 nid, uint32 vid, uint16 payload)
{
	corb_t verb = MAKE_VERB(codec->addr, nid, vid, payload);
	return hda_send_verbs(codec, &verb, NULL, 1);
}


status_t
hda_verb_read(hda_codec* codec, uint32 nid, uint32 vid, uint32* response)
{
	corb_t verb = MAKE_VERB(codec->addr, nid, vid, 0);
	return hda_send_verbs(codec, &verb, response, 1);
}


/*! Setup hardware for use; detect codecs; etc */
status_t
hda_hw_init(hda_controller* controller)
{
	uint16 capabilities;
	uint16 stateStatus;
	uint16 cmd;
	status_t status;
	const pci_info& pciInfo = controller->pci_info;
	uint32 quirks = get_controller_quirks(pciInfo);

	// map the registers (low + high for 64-bit when requested)
	phys_addr_t physicalAddress = pciInfo.u.h0.base_registers[0];
	if ((pciInfo.u.h0.base_register_flags[0] & PCI_address_type)
			== PCI_address_type_64) {
		physicalAddress |= (uint64)pciInfo.u.h0.base_registers[1] << 32;
	}

	// Map MMIO registers
	controller->regs_area = map_physical_memory("hda_hw_regs",
		physicalAddress, pciInfo.u.h0.base_register_sizes[0],
		B_ANY_KERNEL_ADDRESS, 0, (void**)&controller->regs);
	if (controller->regs_area < B_OK) {
		status = controller->regs_area;
		goto error;
	}

	cmd = gPci->read_pci_config(pciInfo.bus, pciInfo.device, pciInfo.function,
		PCI_command, 2);
	if (!(cmd & PCI_command_master)) {
		dprintf("hda: enabling PCI bus mastering\n");
		cmd |= PCI_command_master;
	}
	if (!(cmd & PCI_command_memory)) {
		dprintf("hda: enabling PCI memory access\n");
		cmd |= PCI_command_memory;
	}
	gPci->write_pci_config(pciInfo.bus, pciInfo.device, pciInfo.function,
		PCI_command, 2, cmd);

	// Disable interrupt generation
	controller->Write32(HDAC_INTR_CONTROL, 0);

	// Absolute minimum hw is online; we can now install interrupt handler

	controller->irq = pciInfo.u.h0.interrupt_line;
	controller->msi = false;

	if (gPCIx86Module != NULL && (quirks & HDA_QUIRK_NO_MSI) == 0
			&& gPCIx86Module->get_msi_count(pciInfo.bus, pciInfo.device,
				pciInfo.function) >= 1) {
		// Try MSI first
		uint8 vector;
		if (gPCIx86Module->configure_msi(pciInfo.bus, pciInfo.device,
			pciInfo.function, 1, &vector) == B_OK && gPCIx86Module->enable_msi(
				pciInfo.bus, pciInfo.device, pciInfo.function) == B_OK) {
			dprintf("hda: using MSI vector %u\n", vector);
			controller->irq = vector;
			controller->msi = true;
		}
	}

	status = install_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller, 0);
	if (status != B_OK)
		goto no_irq_handler;

	cmd = gPci->read_pci_config(pciInfo.bus, pciInfo.device, pciInfo.function,
		PCI_command, 2);
	if (controller->msi != ((cmd & PCI_command_int_disable) != 0)) {
		if ((cmd & PCI_command_int_disable) != 0) {
			dprintf("hda: enabling PCI interrupts\n");
			cmd &= ~PCI_command_int_disable;
		} else {
			dprintf("hda: disabling PCI interrupts for MSI use\n");
			cmd |= PCI_command_int_disable;
		}

		gPci->write_pci_config(pciInfo.bus, pciInfo.device, pciInfo.function,
			PCI_command, 2, cmd);
	}

	// TCSEL is reset to TC0 (clear 0-2 bits)
	if ((quirks & HDA_QUIRK_NOTCSEL) == 0) {
		update_pci_register(controller, PCI_HDA_TCSEL, PCI_HDA_TCSEL_MASK, 0,
			1);
	}

	controller->dma_snooping = false;

	if ((quirks & HDA_QUIRK_SNOOP) != 0) {
		switch (pciInfo.vendor_id) {
			case PCI_VENDOR_NVIDIA:
			{
				controller->dma_snooping = update_pci_register(controller,
					NVIDIA_HDA_TRANSREG, NVIDIA_HDA_TRANSREG_MASK,
					NVIDIA_HDA_ENABLE_COHBITS, 1, true);
				if (!controller->dma_snooping)
					break;

				controller->dma_snooping = update_pci_register(controller,
					NVIDIA_HDA_ISTRM_COH, ~NVIDIA_HDA_ENABLE_COHBIT,
					NVIDIA_HDA_ENABLE_COHBIT, 1, true);
				if (!controller->dma_snooping)
					break;

				controller->dma_snooping = update_pci_register(controller,
					NVIDIA_HDA_OSTRM_COH, ~NVIDIA_HDA_ENABLE_COHBIT,
					NVIDIA_HDA_ENABLE_COHBIT, 1, true);

				break;
			}

			case PCI_VENDOR_AMD:
			case PCI_VENDOR_ATI:
			{
				controller->dma_snooping = update_pci_register(controller,
					ATI_HDA_MISC_CNTR2, ATI_HDA_MISC_CNTR2_MASK,
					ATI_HDA_ENABLE_SNOOP, 1, true);
				break;
			}

			case PCI_VENDOR_INTEL:
				controller->dma_snooping = update_pci_register(controller,
					INTEL_SCH_HDA_DEVC, ~INTEL_SCH_HDA_DEVC_SNOOP, 0, 2, true);
				break;
		}
	}

	dprintf("hda: DMA snooping: %s\n",
		controller->dma_snooping ? "yes" : "no");

	capabilities = controller->Read16(HDAC_GLOBAL_CAP);
	controller->num_input_streams = GLOBAL_CAP_INPUT_STREAMS(capabilities);
	controller->num_output_streams = GLOBAL_CAP_OUTPUT_STREAMS(capabilities);
	controller->num_bidir_streams = GLOBAL_CAP_BIDIR_STREAMS(capabilities);
	controller->is_64_bit = GLOBAL_CAP_64BIT(capabilities);

	// show some hw features
	dprintf("hda: HDA v%d.%d, O:%" B_PRIu32 "/I:%" B_PRIu32 "/B:%" B_PRIu32
		", #SDO:%d, 64bit:%s\n",
		controller->Read8(HDAC_VERSION_MAJOR),
		controller->Read8(HDAC_VERSION_MINOR),
		controller->num_output_streams, controller->num_input_streams,
		controller->num_bidir_streams,
		GLOBAL_CAP_NUM_SDO(capabilities),
		controller->is_64_bit ? "yes" : "no");

	// Get controller into valid state
	status = reset_controller(controller);
	if (status != B_OK) {
		dprintf("hda: reset_controller failed\n");
		goto reset_failed;
	}

	// Setup CORB/RIRB/DMA POS
	status = init_corb_rirb_pos(controller, quirks);
	if (status != B_OK) {
		dprintf("hda: init_corb_rirb_pos failed\n");
		goto corb_rirb_failed;
	}

	// Don't enable codec state change interrupts. We don't handle
	// them, as we want to use the STATE_STATUS register to identify
	// available codecs. We'd have to clear that register in the interrupt
	// handler to 'ack' the codec change.
	controller->ReadModifyWrite16(HDAC_WAKE_ENABLE, HDAC_WAKE_ENABLE_MASK, 0);

	// Enable controller interrupts
	controller->Write32(HDAC_INTR_CONTROL, INTR_CONTROL_GLOBAL_ENABLE
		| INTR_CONTROL_CONTROLLER_ENABLE);

	snooze(1000);

	stateStatus = controller->Read16(HDAC_STATE_STATUS);
	if (!stateStatus) {
		dprintf("hda: bad codec status\n");
		status = ENODEV;
		goto corb_rirb_failed;
	}
	controller->Write16(HDAC_STATE_STATUS, stateStatus);

	// Create codecs
	for (uint32 index = 0; index < HDA_MAX_CODECS; index++) {
		if ((stateStatus & (1 << index)) != 0)
			hda_codec_new(controller, index);
	}
	for (uint32 index = 0; index < HDA_MAX_CODECS; index++) {
		if (controller->codecs[index]
			&& controller->codecs[index]->num_audio_groups > 0) {
			controller->active_codec = controller->codecs[index];
			break;
		}
	}

	controller->buffer_ready_sem = create_sem(0, "hda_buffer_sem");
	if (controller->buffer_ready_sem < B_OK) {
		dprintf("hda: failed to create semaphore\n");
		status = ENODEV;
		goto corb_rirb_failed;
	}

	if (controller->active_codec != NULL)
		return B_OK;

	dprintf("hda: no active codec\n");
	status = ENODEV;

	delete_sem(controller->buffer_ready_sem);

corb_rirb_failed:
	controller->Write32(HDAC_INTR_CONTROL, 0);

reset_failed:
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

no_irq_handler:
	if (controller->msi) {
		gPCIx86Module->disable_msi(controller->pci_info.bus,
			controller->pci_info.device, controller->pci_info.function);
		gPCIx86Module->unconfigure_msi(controller->pci_info.bus,
			controller->pci_info.device, controller->pci_info.function);
	}

	delete_area(controller->regs_area);
	controller->regs_area = B_ERROR;
	controller->regs = NULL;

error:
	dprintf("hda: ERROR: %s(%" B_PRIx32 ")\n", strerror(status), status);

	return status;
}


/*! Stop any activity */
void
hda_hw_stop(hda_controller* controller)
{
	// Stop all audio streams
	for (uint32 index = 0; index < HDA_MAX_STREAMS; index++) {
		if (controller->streams[index] && controller->streams[index]->running)
			hda_stream_stop(controller, controller->streams[index]);
	}
}


/*! Free resources */
void
hda_hw_uninit(hda_controller* controller)
{
	if (controller == NULL)
		return;

	// Stop all audio streams
	hda_hw_stop(controller);

	if (controller->buffer_ready_sem >= B_OK) {
		delete_sem(controller->buffer_ready_sem);
		controller->buffer_ready_sem = B_ERROR;
	}

	reset_controller(controller);

	// Disable interrupts, and remove interrupt handler
	controller->Write32(HDAC_INTR_CONTROL, 0);

	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

	if (controller->msi) {
		// Disable MSI
		gPCIx86Module->disable_msi(controller->pci_info.bus,
			controller->pci_info.device, controller->pci_info.function);
		gPCIx86Module->unconfigure_msi(controller->pci_info.bus,
			controller->pci_info.device, controller->pci_info.function);
	}

	// Delete corb/rirb area
	if (controller->corb_rirb_pos_area >= 0) {
		delete_area(controller->corb_rirb_pos_area);
		controller->corb_rirb_pos_area = B_ERROR;
		controller->corb = NULL;
		controller->rirb = NULL;
		controller->stream_positions = NULL;
	}

	// Unmap registers
	if (controller->regs_area >= 0) {
		delete_area(controller->regs_area);
		controller->regs_area = B_ERROR;
		controller->regs = NULL;
	}

	// Now delete all codecs
	for (uint32 index = 0; index < HDA_MAX_CODECS; index++) {
		if (controller->codecs[index] != NULL)
			hda_codec_delete(controller->codecs[index]);
	}
	controller->active_codec = NULL;
}
