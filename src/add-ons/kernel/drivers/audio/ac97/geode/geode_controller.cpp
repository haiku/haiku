/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jérôme Duval (korli@users.berlios.de)
 */


#include "driver.h"

#define ALIGN(size, align)		(((size) + align - 1) & ~(align - 1))
#define PAGE_ALIGN(size)	(((size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1))

#define STREAM_CMD			0x0		/* Command */
#define STREAM_STATUS			0x1		/* IRQ Status */
#define STREAM_PRD			0x4		/* PRD Table Address */

static const struct {
	uint32 multi_rate;
	uint32 rate;
} kRates[] = {
	{B_SR_8000, 8000},
	{B_SR_11025, 11025},
	{B_SR_16000, 16000},
	{B_SR_22050, 22050},
	{B_SR_32000, 32000},
	{B_SR_44100, 44100},
	{B_SR_48000, 48000},
	{B_SR_88200, 88200},
	{B_SR_96000, 96000},
	{B_SR_176400, 176400},
	{B_SR_192000, 192000},
};

static int
geode_codec_wait(geode_controller *controller)
{
	int i;

#define GCSCAUDIO_WAIT_READY_CODEC_TIMEOUT	500
	for (i = GCSCAUDIO_WAIT_READY_CODEC_TIMEOUT; (i >= 0)
	    && (controller->Read32(ACC_CODEC_CNTL) & ACC_CODEC_CNTL_CMD_NEW); i--)
		snooze(10);

	return (i < 0);
}

uint16
geode_codec_read(geode_controller *controller, uint8 regno)
{
	int i;
	uint32 v;
	ASSERT(regno >= 0);

	controller->Write32(ACC_CODEC_CNTL,
		ACC_CODEC_CNTL_READ_CMD | ACC_CODEC_CNTL_CMD_NEW |
		ACC_CODEC_REG2ADDR(regno));

	if (geode_codec_wait(controller) != B_OK) {
		dprintf("codec busy (2)\n");
		return 0xffff;
	}

#define GCSCAUDIO_READ_CODEC_TIMEOUT	50
	for (i = GCSCAUDIO_READ_CODEC_TIMEOUT; i >= 0; i--) {
		v = controller->Read32(ACC_CODEC_STATUS);
		if ((v & ACC_CODEC_STATUS_STS_NEW) &&
		    (ACC_CODEC_ADDR2REG(v) == regno))
			break;

		snooze(10);
	}

	if (i < 0) {
		dprintf("codec busy (3)\n");
		return 0xffff;
	}

	return v;
}

void
geode_codec_write(geode_controller *controller, uint8 regno, uint16 value)
{
	ASSERT(regno >= 0);

	controller->Write32(ACC_CODEC_CNTL,
	    ACC_CODEC_CNTL_WRITE_CMD |
	    ACC_CODEC_CNTL_CMD_NEW |
	    ACC_CODEC_REG2ADDR(regno) |
	    (value & ACC_CODEC_CNTL_CMD_DATA_MASK));

	if (geode_codec_wait(controller) != B_OK) {
		dprintf("codec busy (4)\n");
	}
}


//! Called with interrupts off
static void
stream_handle_interrupt(geode_controller* controller, geode_stream* stream)
{
	uint8 status;
	uint32 position, bufferSize;

	if (!stream->running)
		return;

	status = stream->Read8(STREAM_STATUS);

	if (status & ACC_BMx_STATUS_BM_EOP_ERR) {
		dprintf("geode: stream status bus master error\n");
	}
	if (status & ACC_BMx_STATUS_EOP) {
		dprintf("geode: stream status end of page\n");
	}

	position = controller->Read32(ACC_BM0_PNTR + stream->dma_offset);
	bufferSize = ALIGN(stream->sample_size * stream->num_channels * stream->buffer_length, 128);

	// Buffer Completed Interrupt
	acquire_spinlock(&stream->lock);

	stream->real_time = system_time();
	stream->frames_count += stream->buffer_length;
	stream->buffer_cycle = 1 - (position / (bufferSize + 1)); // added 1 to avoid having 2

	release_spinlock(&stream->lock);

	release_sem_etc(stream->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
}


static int32
geode_interrupt_handler(geode_controller* controller)
{
	uint16 intr = controller->Read16(ACC_IRQ_STATUS);
	if (intr == 0)
		return B_UNHANDLED_INTERRUPT;

	for (uint32 index = 0; index < GEODE_MAX_STREAMS; index++) {
		if (controller->streams[index]
			&& (intr & controller->streams[index]->status) != 0) {
			stream_handle_interrupt(controller,
				controller->streams[index]);
		}
	}

	return B_HANDLED_INTERRUPT;
}


static status_t
reset_controller(geode_controller* controller)
{
	controller->Write32(ACC_CODEC_CNTL, ACC_CODEC_CNTL_LNK_WRM_RST
	    | ACC_CODEC_CNTL_CMD_NEW);

	if (geode_codec_wait(controller) != B_OK) {
		dprintf("codec reset busy (1)\n");
	}

	// stop streams

	// stop DMA

	// reset DMA position buffer

	return B_OK;
}

//	#pragma mark - public stream functions


void
geode_stream_delete(geode_stream* stream)
{
	if (stream->buffer_ready_sem >= B_OK)
		delete_sem(stream->buffer_ready_sem);

	if (stream->buffer_area >= B_OK)
		delete_area(stream->buffer_area);

	if (stream->buffer_descriptors_area >= B_OK)
		delete_area(stream->buffer_descriptors_area);

	free(stream);
}


geode_stream*
geode_stream_new(geode_controller* controller, int type)
{
	geode_stream* stream = (geode_stream*)calloc(1, sizeof(geode_stream));
	if (stream == NULL)
		return NULL;

	stream->buffer_ready_sem = B_ERROR;
	stream->buffer_area = B_ERROR;
	stream->buffer_descriptors_area = B_ERROR;
	stream->type = type;
	stream->controller = controller;

	switch (type) {
		case STREAM_PLAYBACK:
			stream->status = ACC_IRQ_STATUS_BM0_IRQ_STS;
			stream->offset = 0;
			stream->dma_offset = 0;
			stream->ac97_rate_reg = AC97_PCM_FRONT_DAC_RATE;
			break;

		case STREAM_RECORD:
			stream->status = ACC_IRQ_STATUS_BM1_IRQ_STS;
			stream->offset = 0x8;
			stream->dma_offset = 0x4;
			stream->ac97_rate_reg = AC97_PCM_L_R_ADC_RATE;
			break;

		default:
			dprintf("%s: Unknown stream type %d!\n", __func__, type);
			free(stream);
			stream = NULL;
	}

	controller->streams[controller->num_streams++] = stream;
	return stream;
}


/*!	Starts a stream's DMA engine, and enables generating and receiving
	interrupts for this stream.
*/
status_t
geode_stream_start(geode_stream* stream)
{
	uint8 value;
	dprintf("geode_stream_start()\n");
	stream->buffer_ready_sem = create_sem(0, stream->type == STREAM_PLAYBACK
		? "geode_playback_sem" : "geode_record_sem");
	if (stream->buffer_ready_sem < B_OK)
		return stream->buffer_ready_sem;

	if (stream->type == STREAM_PLAYBACK)
		value = ACC_BMx_CMD_WRITE;
	else
		value = ACC_BMx_CMD_READ;

	stream->Write8(STREAM_CMD, value | ACC_BMx_CMD_BYTE_ORD_EL
		| ACC_BMx_CMD_BM_CTL_ENABLE);

	stream->running = true;
	return B_OK;
}


/*!	Stops the stream's DMA engine, and turns off interrupts for this
	stream.
*/
status_t
geode_stream_stop(geode_stream* stream)
{
	dprintf("geode_stream_stop()\n");
	stream->Write8(STREAM_CMD, ACC_BMx_CMD_BM_CTL_DISABLE);

	stream->running = false;
	delete_sem(stream->buffer_ready_sem);
	stream->buffer_ready_sem = -1;

	return B_OK;
}


status_t
geode_stream_setup_buffers(geode_stream* stream, const char* desc)
{
	uint32 bufferSize, alloc;
	uint32 index;
	physical_entry pe;
	struct acc_prd* bufferDescriptors;
	uint8* buffer;
	status_t rc;

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

	dprintf("geode: sample size %ld, num channels %ld, buffer length %ld ****************\n",
		stream->sample_size, stream->num_channels, stream->buffer_length);

	/* Calculate total size of all buffers (aligned to size of B_PAGE_SIZE) */
	alloc = bufferSize * stream->num_buffers;
	alloc = PAGE_ALIGN(alloc);

	/* Allocate memory for buffers */
	stream->buffer_area = create_area("geode buffers", (void**)&buffer,
		B_ANY_KERNEL_ADDRESS, alloc, B_32_BIT_CONTIGUOUS,
		B_READ_AREA | B_WRITE_AREA);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
	if (stream->buffer_area < B_OK)
		return stream->buffer_area;

	/* Get the physical address of memory */
	rc = get_memory_map(buffer, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(stream->buffer_area);
		return rc;
	}

	phys_addr_t bufferPhysicalAddress = pe.address;

	dprintf("%s(%s): Allocated %lu bytes for %ld buffers\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Store pointers (both virtual/physical) */
	for (index = 0; index < stream->num_buffers; index++) {
		stream->buffers[index] = buffer + (index * bufferSize);
		stream->physical_buffers[index] = bufferPhysicalAddress
			+ (index * bufferSize);
	}

	/* Now allocate BDL for buffer range */
	alloc = stream->num_buffers * sizeof(struct acc_prd) + 1;
	alloc = PAGE_ALIGN(alloc);

	stream->buffer_descriptors_area = create_area("geode buffer descriptors",
		(void**)&bufferDescriptors, B_ANY_KERNEL_ADDRESS, alloc,
		B_32_BIT_CONTIGUOUS, 0);
		// TODO: The rest of the code doesn't deal correctly with physical
		// addresses > 4 GB, so we have to force 32 bit addresses here.
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

	stream->physical_buffer_descriptors = pe.address;

	dprintf("%s(%s): Allocated %ld bytes for %ld BDLEs\n", __func__, desc,
		alloc, stream->num_buffers);

	/* Setup buffer descriptor list (BDL) entries */
	for (index = 0; index < stream->num_buffers; index++, bufferDescriptors++) {
		bufferDescriptors->address = stream->physical_buffers[index];
		bufferDescriptors->ctrlsize = bufferSize | ACC_BMx_PRD_CTRL_EOP;
			// we want an interrupt after every buffer
	}
	bufferDescriptors->address = stream->physical_buffer_descriptors;
	bufferDescriptors->ctrlsize = ACC_BMx_PRD_CTRL_JMP;

	for (index = 0; index < sizeof(kRates) / sizeof(kRates[0]); index++) {
		if (kRates[index].multi_rate == stream->sample_rate) {
			stream->rate = kRates[index].rate;
			break;
		}
	}

	/* Configure stream registers */
	dprintf("IRA: %s: setup stream SR=%ld\n", __func__, stream->rate);

	stream->Write32(STREAM_PRD, stream->physical_buffer_descriptors);

	ac97_set_rate(stream->controller->ac97, stream->ac97_rate_reg, stream->rate);
	snooze(1000);
	return B_OK;
}


//	#pragma mark - public controller functions


/*! Setup hardware for use; detect codecs; etc */
status_t
geode_hw_init(geode_controller* controller)
{
	uint16 cmd;
	status_t status;

	cmd = (gPci->read_pci_config)(controller->pci_info.bus,
		controller->pci_info.device, controller->pci_info.function, PCI_command, 2);
	if (!(cmd & PCI_command_master)) {
		(gPci->write_pci_config)(controller->pci_info.bus,
			controller->pci_info.device, controller->pci_info.function,
				PCI_command, 2, cmd | PCI_command_master);
		dprintf("geode: enabling PCI bus mastering\n");
	}

	controller->nabmbar = controller->pci_info.u.h0.base_registers[0];

	/* Absolute minimum hw is online; we can now install interrupt handler */
	controller->irq = controller->pci_info.u.h0.interrupt_line;
	status = install_io_interrupt_handler(controller->irq,
		(interrupt_handler)geode_interrupt_handler, controller, 0);
	if (status != B_OK)
		goto error;

	/* Get controller into valid state */
	status = reset_controller(controller);
	if (status != B_OK) {
		dprintf("geode: reset_controller failed\n");
		goto reset_failed;
	}

	/* attach the codec */
	ac97_attach(&controller->ac97, (codec_reg_read)geode_codec_read,
		(codec_reg_write)geode_codec_write, controller,
		controller->pci_info.u.h0.subsystem_vendor_id,
		controller->pci_info.u.h0.subsystem_id);

	snooze(1000);

	controller->multi = (geode_multi*)calloc(1, sizeof(geode_multi));
        if (controller->multi == NULL)
                return B_NO_MEMORY;

	controller->playback_stream = geode_stream_new(controller, STREAM_PLAYBACK);
        controller->record_stream = geode_stream_new(controller, STREAM_RECORD);

	return B_OK;

reset_failed:
	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)geode_interrupt_handler, controller);
error:
	dprintf("geode: ERROR: %s(%ld)\n", strerror(status), status);

	return status;
}


/*! Stop any activity */
void
geode_hw_stop(geode_controller* controller)
{
	int index;

	/* Stop all audio streams */
	for (index = 0; index < GEODE_MAX_STREAMS; index++) {
		if (controller->streams[index] && controller->streams[index]->running)
			geode_stream_stop(controller->streams[index]);
	}
}


/*! Free resources */
void
geode_hw_uninit(geode_controller* controller)
{
	if (controller == NULL)
		return;

	/* Stop all audio streams */
	geode_hw_stop(controller);

	reset_controller(controller);

	remove_io_interrupt_handler(controller->irq,
		(interrupt_handler)geode_interrupt_handler, controller);

	free(controller->multi);

	geode_stream_delete(controller->playback_stream);
        geode_stream_delete(controller->record_stream);

}

