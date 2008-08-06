/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "hda_controller_defs.h"
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

#define ALIGN(size, align)		(((size) + align - 1) & ~(align - 1))
#define PAGE_ALIGN(size)	(((size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1))

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
};


static inline rirb_t&
current_rirb(hda_controller *controller)
{
	return controller->rirb[controller->rirb_read_pos];
}


static inline uint32
next_rirb(hda_controller *controller)
{
	return (controller->rirb_read_pos + 1) % controller->rirb_length;
}


static inline uint32
next_corb(hda_controller *controller)
{
	return (controller->corb_write_pos + 1) % controller->corb_length;
}


//! Called with interrupts off
static void
stream_handle_interrupt(hda_controller* controller, hda_stream* stream)
{
	uint8 status;
	uint32 position, bufferSize;

	if (!stream->running)
		return;

	status = stream->Read8(HDAC_STREAM_STATUS);
	if (status == 0)
		return;

	stream->Write8(HDAC_STREAM_STATUS, status);

	if ((status & STATUS_BUFFER_COMPLETED) == 0) {
		dprintf("hda: stream status %x\n", status);
		return;
	}
	
	position = stream->Read32(HDAC_STREAM_POSITION);
	bufferSize = ALIGN(stream->sample_size * stream->num_channels * stream->buffer_length, 128);
	
	// Buffer Completed Interrupt
	acquire_spinlock(&stream->lock);

	stream->real_time = system_time();
	stream->frames_count += stream->buffer_length;
	stream->buffer_cycle = 1 - (position / bufferSize);

	release_spinlock(&stream->lock);
	
	release_sem_etc(stream->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}


static int32
hda_interrupt_handler(hda_controller* controller)
{
	int32 handled = B_HANDLED_INTERRUPT;

	/* Check if this interrupt is ours */
	uint32 intrStatus = controller->Read32(HDAC_INTR_STATUS);
	if ((intrStatus & INTR_STATUS_GLOBAL) == 0)
		return B_UNHANDLED_INTERRUPT;

	/* Controller or stream related? */
	if (intrStatus & INTR_STATUS_CONTROLLER) {
		uint8 rirbStatus = controller->Read8(HDAC_RIRB_STATUS);
		uint8 corbStatus = controller->Read8(HDAC_CORB_STATUS);

		/* Check for incoming responses */			
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

					if ((responseFlags & RESPONSE_FLAGS_UNSOLICITED) != 0) {
						dprintf("hda: Unsolicited response: %08lx/%08lx\n",
							response, responseFlags);
						continue;
					}
					if (codec == NULL) {
						dprintf("hda: Response for unknown codec %ld: "
							"%08lx/%08lx\n", cad, response, responseFlags);
						continue;
					}
					if (codec->response_count >= MAX_CODEC_RESPONSES) {
						dprintf("hda: too many responses received for codec %ld"
							": %08lx/%08lx!\n", cad, response, responseFlags);
						continue;
					}

					/* Store response in codec */
					codec->responses[codec->response_count++] = response;
					release_sem_etc(codec->response_sem, 1, B_DO_NOT_RESCHEDULE);
					handled = B_INVOKE_SCHEDULER;
				}			
			}

			if ((rirbStatus & RIRB_STATUS_OVERRUN) != 0)
				dprintf("hda: RIRB Overflow\n");
		}

		/* Check for sending errors */
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
					stream_handle_interrupt(controller,
						controller->streams[index]);
				} else {
					dprintf("hda: Stream interrupt for unconfigured stream "
						"%ld!\n", index);
				}
			}
		}
	}

	/* NOTE: See HDA001 => CIS/GIS cannot be cleared! */

	return handled;
}


static status_t
reset_controller(hda_controller* controller)
{
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
	controller->Write8(HDAC_CORB_CONTROL, 0);
	controller->Write8(HDAC_RIRB_CONTROL, 0);

	// reset DMA position buffer
	controller->Write32(HDAC_DMA_POSITION_BASE_LOWER, 0);
	controller->Write32(HDAC_DMA_POSITION_BASE_UPPER, 0);

	// Set reset bit - it must be asserted for at least 100us

	uint32 control = controller->Read32(HDAC_GLOBAL_CONTROL);
	controller->Write32(HDAC_GLOBAL_CONTROL, control & ~GLOBAL_CONTROL_RESET);

	for (int timeout = 0; timeout < 10; timeout++) {
		snooze(100);

		control = controller->Read32(HDAC_GLOBAL_CONTROL);
		if ((control & GLOBAL_CONTROL_RESET) == 0)
			break;
	}
	if ((control & GLOBAL_CONTROL_RESET) != 0) {
		dprintf("hda: unable to reset controller\n");
		return B_BUSY;
	}

	// Unset reset bit

	control = controller->Read32(HDAC_GLOBAL_CONTROL);
	controller->Write32(HDAC_GLOBAL_CONTROL, control | GLOBAL_CONTROL_RESET);

	for (int timeout = 0; timeout < 10; timeout++) {
		snooze(100);

		control = controller->Read32(HDAC_GLOBAL_CONTROL);
		if ((control & GLOBAL_CONTROL_RESET) != 0)
			break;
	}
	if ((control & GLOBAL_CONTROL_RESET) == 0) {
		dprintf("hda: unable to exit reset\n");
		return B_BUSY;
	}

	// Wait for codecs to finish their own reset (apparently needs more
	// time than documented in the specs)
	snooze(1000);
	return B_OK;
}


/*! Allocates and initializes the Command Output Ring Buffer (CORB), and
	Response Input Ring Buffer (RIRB) to the maximum supported size, and also
	the DMA position buffer.

	Programs the controller hardware to make use of these buffers (the DMA
	positioning is actually enabled in hda_stream_setup_buffers()).
*/
static status_t
init_corb_rirb_pos(hda_controller* controller)
{
	uint32 memSize, rirbOffset, posOffset;
	uint8 corbSize, rirbSize, posSize;
	status_t rc = B_OK;
	physical_entry pe;

	/* Determine and set size of CORB */
	corbSize = controller->Read8(HDAC_CORB_SIZE);
	if ((corbSize & CORB_SIZE_CAP_256_ENTRIES) != 0) {
		controller->corb_length = 256;
		controller->Write8(HDAC_CORB_SIZE, CORB_SIZE_256_ENTRIES);
	} else if (corbSize & CORB_SIZE_CAP_16_ENTRIES) {
		controller->corb_length = 16;
		controller->Write8(HDAC_CORB_SIZE, CORB_SIZE_16_ENTRIES);
	} else if (corbSize & CORB_SIZE_CAP_2_ENTRIES) {
		controller->corb_length = 2;
		controller->Write8(HDAC_CORB_SIZE, CORB_SIZE_2_ENTRIES);
	}

	/* Determine and set size of RIRB */
	rirbSize = controller->Read8(HDAC_RIRB_SIZE);
	if (rirbSize & RIRB_SIZE_CAP_256_ENTRIES) {
		controller->rirb_length = 256;
		controller->Write8(HDAC_RIRB_SIZE, RIRB_SIZE_256_ENTRIES);
	} else if (rirbSize & RIRB_SIZE_CAP_16_ENTRIES) {
		controller->rirb_length = 16;
		controller->Write8(HDAC_RIRB_SIZE, RIRB_SIZE_16_ENTRIES);
	} else if (rirbSize & RIRB_SIZE_CAP_2_ENTRIES) {
		controller->rirb_length = 2;
		controller->Write8(HDAC_RIRB_SIZE, RIRB_SIZE_2_ENTRIES);
	}

	/* Determine rirb offset in memory and total size of corb+alignment+rirb */
	rirbOffset = ALIGN(controller->corb_length * sizeof(corb_t), 128);
	posOffset = ALIGN(rirbOffset + controller->rirb_length * sizeof(rirb_t), 128);
	posSize = 8 * (controller->num_input_streams
		+ controller->num_output_streams + controller->num_bidir_streams);

	memSize = PAGE_ALIGN(posOffset + posSize);

	/* Allocate memory area */
	controller->corb_rirb_pos_area = create_area("hda corb/rirb/pos",
		(void**)&controller->corb, B_ANY_KERNEL_ADDRESS, memSize,
		B_FULL_LOCK | B_CONTIGUOUS, 0);
	if (controller->corb_rirb_pos_area < 0)
		return controller->corb_rirb_pos_area;

	/* Rirb is after corb+aligment */
	controller->rirb = (rirb_t*)(((uint8*)controller->corb) + rirbOffset);

	if ((rc = get_memory_map(controller->corb, memSize, &pe, 1)) != B_OK) {
		delete_area(controller->corb_rirb_pos_area);
		return rc;
	}

	/* Program CORB/RIRB for these locations */
	controller->Write32(HDAC_CORB_BASE_LOWER, (uint32)pe.address);
	controller->Write32(HDAC_CORB_BASE_UPPER, 0);
	controller->Write32(HDAC_RIRB_BASE_LOWER, (uint32)pe.address + rirbOffset);
	controller->Write32(HDAC_RIRB_BASE_UPPER, 0);

	/* Program DMA position update */
	controller->Write32(HDAC_DMA_POSITION_BASE_LOWER,
		(uint32)pe.address + posOffset);
	controller->Write32(HDAC_DMA_POSITION_BASE_UPPER, 0);

	controller->stream_positions = (uint32*)
		((uint8*)controller->corb + posOffset);

	/* Reset CORB read pointer */
	/* NOTE: See HDA011 for corrected procedure! */
	controller->Write16(HDAC_CORB_READ_POS, CORB_READ_POS_RESET);
	do {
		spin(10);
	} while ((controller->Read16(HDAC_CORB_READ_POS)
		& CORB_READ_POS_RESET) == 0);
	controller->Write16(HDAC_CORB_READ_POS, 0);

	/* Reset RIRB write pointer */
	controller->Write16(HDAC_RIRB_WRITE_POS, RIRB_WRITE_POS_RESET);

	/* Generate interrupt for every response */
	controller->Write16(HDAC_RESPONSE_INTR_COUNT, 1);

	/* Setup cached read/write indices */
	controller->rirb_read_pos = 1;
	controller->corb_write_pos = 0;

	/* Gentlemen, start your engines... */
	controller->Write8(HDAC_CORB_CONTROL,
		CORB_CONTROL_RUN | CORB_CONTROL_MEMORY_ERROR_INTR);
	controller->Write8(HDAC_RIRB_CONTROL, RIRB_CONTROL_DMA_ENABLE
		| RIRB_CONTROL_OVERRUN_INTR | RIRB_CONTROL_RESPONSE_INTR);

	return B_OK;
}


//	#pragma mark - public stream functions


void
hda_stream_delete(hda_stream* stream)
{
	if (stream->buffer_ready_sem >= B_OK)
		delete_sem(stream->buffer_ready_sem);

	if (stream->buffer_area >= B_OK)
		delete_area(stream->buffer_area);

	if (stream->buffer_descriptors_area >= B_OK)
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

	stream->buffer_ready_sem = B_ERROR;
	stream->buffer_area = B_ERROR;
	stream->buffer_descriptors_area = B_ERROR;
	stream->type = type;
	stream->controller = controller;

	switch (type) {
		case STREAM_PLAYBACK:
			stream->id = 1;
			stream->offset = HDAC_OUTPUT_STREAM_OFFSET(controller, 0);
			controller->streams[controller->num_input_streams] = stream;
			break;

		case STREAM_RECORD:
			stream->id = 2;
			stream->offset = HDAC_INPUT_STREAM_OFFSET(controller, 0);
			controller->streams[0] = stream;
			break;

		default:
			dprintf("%s: Unknown stream type %d!\n", __func__, type);
			free(stream);
			stream = NULL;
	}

	// find I/O and Pin widgets for this stream

	if (hda_audio_group_get_widgets(audioGroup, stream) == B_OK)
		return stream;

	free(stream);
	return NULL;
}


/*!	Starts a stream's DMA engine, and enables generating and receiving
	interrupts for this stream.
*/
status_t
hda_stream_start(hda_controller* controller, hda_stream* stream)
{
	stream->buffer_ready_sem = create_sem(0, stream->type == STREAM_PLAYBACK
		? "hda_playback_sem" : "hda_record_sem");
	if (stream->buffer_ready_sem < B_OK)
		return stream->buffer_ready_sem;

	controller->Write32(HDAC_INTR_CONTROL, controller->Read32(HDAC_INTR_CONTROL)
		| (1 << (stream->offset / HDAC_STREAM_SIZE)));
	stream->Write8(HDAC_STREAM_CONTROL0, stream->Read8(HDAC_STREAM_CONTROL0)
		| CONTROL0_BUFFER_COMPLETED_INTR | CONTROL0_FIFO_ERROR_INTR
		| CONTROL0_DESCRIPTOR_ERROR_INTR | CONTROL0_RUN);

	stream->running = true;
	return B_OK;
}


/*!	Stops the stream's DMA engine, and turns off interrupts for this
	stream.
*/
status_t
hda_stream_stop(hda_controller* controller, hda_stream* stream)
{
	stream->Write8(HDAC_STREAM_CONTROL0, stream->Read8(HDAC_STREAM_CONTROL0)
		& ~(CONTROL0_BUFFER_COMPLETED_INTR | CONTROL0_FIFO_ERROR_INTR
			| CONTROL0_DESCRIPTOR_ERROR_INTR | CONTROL0_RUN));
	controller->Write32(HDAC_INTR_CONTROL, controller->Read32(HDAC_INTR_CONTROL)
		& ~(1 << (stream->offset / HDAC_STREAM_SIZE)));

	stream->running = false;
	delete_sem(stream->buffer_ready_sem);
	stream->buffer_ready_sem = -1;

	return B_OK;
}


status_t
hda_stream_setup_buffers(hda_audio_group* audioGroup, hda_stream* stream,
	const char* desc)
{
	uint32 bufferSize, bufferPhysicalAddress, alloc;
	uint32 response[2], index;
	physical_entry pe;
	bdl_entry_t* bufferDescriptors;
	corb_t verb[2];
	uint8* buffer;
	status_t rc;
	uint16 format;

	/* Clear previously allocated memory */
	if (stream->buffer_area >= B_OK) {
		delete_area(stream->buffer_area);
		stream->buffer_area = B_ERROR;
	}

	if (stream->buffer_descriptors_area >= B_OK) {
		delete_area(stream->buffer_descriptors_area);
		stream->buffer_descriptors_area = B_ERROR;
	}

	/* Calculate size of buffer (aligned to 128 bytes) */		
	bufferSize = stream->sample_size * stream->num_channels
		* stream->buffer_length;
	bufferSize = ALIGN(bufferSize, 128);

	dprintf("HDA: sample size %ld, num channels %ld, buffer length %ld ****************\n",
		stream->sample_size, stream->num_channels, stream->buffer_length);

	/* Calculate total size of all buffers (aligned to size of B_PAGE_SIZE) */
	alloc = bufferSize * stream->num_buffers;
	alloc = PAGE_ALIGN(alloc);

	/* Allocate memory for buffers */
	stream->buffer_area = create_area("hda buffers", (void**)&buffer,
		B_ANY_KERNEL_ADDRESS, alloc, B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	/* Get the physical address of memory */
	rc = get_memory_map(buffer, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		return rc;
	}
	
	bufferPhysicalAddress = (uint32)pe.address;

	dprintf("%s(%s): Allocated %lu bytes for %ld buffers\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Store pointers (both virtual/physical) */	
	for (index = 0; index < stream->num_buffers; index++) {
		stream->buffers[index] = buffer + (index * bufferSize);
		stream->physical_buffers[index] = bufferPhysicalAddress
			+ (index * bufferSize);
	}

	/* Now allocate BDL for buffer range */
	alloc = stream->num_buffers * sizeof(bdl_entry_t);
	alloc = PAGE_ALIGN(alloc);

	stream->buffer_descriptors_area = create_area("hda buffer descriptors",
		(void**)&bufferDescriptors, B_ANY_KERNEL_ADDRESS, alloc,
		B_FULL_LOCK | B_CONTIGUOUS, 0);
	if (stream->buffer_descriptors_area < B_OK) {
		delete_area(stream->buffer_area);
		return stream->buffer_descriptors_area;
	}

	/* Get the physical address of memory */
	rc = get_memory_map(bufferDescriptors, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		delete_area(stream->buffer_descriptors_area);
		return rc;
	}

	stream->physical_buffer_descriptors = (uint32)pe.address;

	dprintf("%s(%s): Allocated %ld bytes for %ld BDLEs\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Setup buffer descriptor list (BDL) entries */	
	for (index = 0; index < stream->num_buffers; index++, bufferDescriptors++) {
		bufferDescriptors->lower = stream->physical_buffers[index];
		bufferDescriptors->upper = 0;
		bufferDescriptors->length = bufferSize;
		bufferDescriptors->ioc = 1;
			// we want an interrupt after every buffer
	}

	/* Configure stream registers */
	format = (stream->num_channels - 1) & 0xf;
	switch (stream->sample_format) {
		case B_FMT_8BIT_S:	format |= FORMAT_8BIT; stream->bps = 8; break;
		case B_FMT_16BIT:	format |= FORMAT_16BIT; stream->bps = 16; break;
		case B_FMT_20BIT:	format |= FORMAT_20BIT; stream->bps = 20; break;
		case B_FMT_24BIT:	format |= FORMAT_24BIT; stream->bps = 24; break;
		case B_FMT_32BIT:	format |= FORMAT_32BIT; stream->bps = 32; break;

		default:
			dprintf("hda: Invalid sample format: 0x%lx\n",
				stream->sample_format);
			break;
	}

	for (index = 0; index < sizeof(kRates) / sizeof(kRates[0]); index++) {
		if (kRates[index].multi_rate == stream->sample_rate) {
			format |= kRates[index].hw_rate;
			stream->rate = kRates[index].rate;
			break;
		}
	}

	dprintf("IRA: %s: setup stream %ld: SR=%ld, SF=%ld\n", __func__, stream->id,
		stream->rate, stream->bps);
	
	stream->Write16(HDAC_STREAM_FORMAT, format);
	stream->Write32(HDAC_STREAM_BUFFERS_BASE_LOWER,
		stream->physical_buffer_descriptors);
	stream->Write32(HDAC_STREAM_BUFFERS_BASE_UPPER, 0);
	stream->Write16(HDAC_STREAM_LAST_VALID, stream->num_buffers - 1);
	/* total cyclic buffer size in _bytes_ */
	stream->Write32(HDAC_STREAM_BUFFER_SIZE, bufferSize
		* stream->num_buffers);
	stream->Write8(HDAC_STREAM_CONTROL2, stream->id << 4);

	stream->controller->Write32(HDAC_DMA_POSITION_BASE_LOWER,
		stream->controller->Read32(HDAC_DMA_POSITION_BASE_LOWER)
		| DMA_POSITION_ENABLED);

	hda_codec* codec = audioGroup->codec;
	for (uint32 i = 0; i < stream->num_io_widgets; i++) {
		verb[0] = MAKE_VERB(codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_FORMAT, format);
		verb[1] = MAKE_VERB(codec->addr, stream->io_widgets[i],
			VID_SET_CONVERTER_STREAM_CHANNEL, stream->id << 4);
		hda_send_verbs(codec, verb, response, 2);
	}

	snooze(1000);
	return B_OK;
}


//	#pragma mark - public controller functions


status_t
hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, uint32 count)
{
	hda_controller *controller = codec->controller;
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
		if (status < B_OK)
			return status;
	}

	if (responses != NULL)
		memcpy(responses, codec->responses, count * sizeof(uint32));

	return B_OK;
}


/*! Setup hardware for use; detect codecs; etc */
status_t
hda_hw_init(hda_controller* controller)
{
	uint16 capabilities, stateStatus, cmd;
	status_t status;
	uint8 tcsel;
	
	/* Map MMIO registers */
	controller->regs_area = map_physical_memory("hda_hw_regs",
		(void*)controller->pci_info.u.h0.base_registers[0],
		controller->pci_info.u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS,
		0, (void**)&controller->regs);
	if (controller->regs_area < B_OK) {
		status = controller->regs_area;
		goto error;
	}

	cmd = (gPci->read_pci_config)(controller->pci_info.bus, 
		controller->pci_info.device, controller->pci_info.function, PCI_command, 2);
	if (!(cmd & PCI_command_master)) {
		(gPci->write_pci_config)(controller->pci_info.bus, 
			controller->pci_info.device, controller->pci_info.function, 
				PCI_command, 2, cmd | PCI_command_master);
		dprintf("hda: enabling PCI bus mastering\n");
	}
	
	/* Absolute minimum hw is online; we can now install interrupt handler */
	controller->irq = controller->pci_info.u.h0.interrupt_line;
	status = install_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller, 0);
	if (status != B_OK)
		goto no_irq;

	/* TCSEL is reset to TC0 (clear 0-2 bits) */
	tcsel = (gPci->read_pci_config)(controller->pci_info.bus, 
		controller->pci_info.device, controller->pci_info.function, PCI_HDA_TCSEL, 1);
	(gPci->write_pci_config)(controller->pci_info.bus, 
		controller->pci_info.device, controller->pci_info.function, 
		PCI_HDA_TCSEL, 1, tcsel & 0xf8);
	
	capabilities = controller->Read16(HDAC_GLOBAL_CAP);
	controller->num_input_streams = GLOBAL_CAP_INPUT_STREAMS(capabilities);
	controller->num_output_streams = GLOBAL_CAP_OUTPUT_STREAMS(capabilities);
	controller->num_bidir_streams = GLOBAL_CAP_BIDIR_STREAMS(capabilities);

	/* show some hw features */
	dprintf("hda: HDA v%d.%d, O:%ld/I:%ld/B:%ld, #SDO:%d, 64bit:%s\n", 
		controller->Read8(HDAC_VERSION_MAJOR),
		controller->Read8(HDAC_VERSION_MINOR),
		controller->num_output_streams, controller->num_input_streams,
		controller->num_bidir_streams,
		GLOBAL_CAP_NUM_SDO(capabilities),
		GLOBAL_CAP_64BIT(capabilities) ? "yes" : "no");

	/* Get controller into valid state */
	status = reset_controller(controller);
	if (status != B_OK) {
		dprintf("hda: reset_controller failed\n");
		goto reset_failed;
	}

	/* Setup CORB/RIRB/DMA POS */
	status = init_corb_rirb_pos(controller);
	if (status != B_OK) {
		dprintf("hda: init_corb_rirb_pos failed\n");
		goto corb_rirb_failed;
	}

	controller->Write16(HDAC_WAKE_ENABLE, 0x7fff);

	/* Enable controller interrupts */
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

	if (controller->active_codec != NULL)
		return B_OK;

	dprintf("hda: no active codec\n");
	status = ENODEV;

corb_rirb_failed:
	controller->Write32(HDAC_INTR_CONTROL, 0);

reset_failed:
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

no_irq:
	delete_area(controller->regs_area);
	controller->regs_area = B_ERROR;
	controller->regs = NULL;

error:
	dprintf("hda: ERROR: %s(%ld)\n", strerror(status), status);

	return status;
}


/*! Stop any activity */
void
hda_hw_stop(hda_controller* controller)
{
	int index;

	/* Stop all audio streams */
	for (index = 0; index < HDA_MAX_STREAMS; index++) {
		if (controller->streams[index] && controller->streams[index]->running)
			hda_stream_stop(controller, controller->streams[index]);
	}
}


/*! Free resources */
void
hda_hw_uninit(hda_controller* controller)
{
	uint32 index;

	if (controller == NULL)
		return;

	/* Stop all audio streams */
	hda_hw_stop(controller);

	reset_controller(controller);

	/* Disable interrupts, and remove interrupt handler */
	controller->Write32(HDAC_INTR_CONTROL, 0);

	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)hda_interrupt_handler, controller);

	/* Delete corb/rirb area */
	if (controller->corb_rirb_pos_area >= 0) {
		delete_area(controller->corb_rirb_pos_area);
		controller->corb_rirb_pos_area = B_ERROR;
		controller->corb = NULL;
		controller->rirb = NULL;
		controller->stream_positions = NULL;
	}

	/* Unmap registers */
	if (controller->regs_area >= 0) {
		delete_area(controller->regs_area);
		controller->regs_area = B_ERROR;
		controller->regs = NULL;
	}

	/* Now delete all codecs */
	for (index = 0; index < HDA_MAX_CODECS; index++) {
		if (controller->codecs[index] != NULL)
			hda_codec_delete(controller->codecs[index]);
	}
}

