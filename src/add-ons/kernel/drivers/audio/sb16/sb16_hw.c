/*
 * This code is very much based on the BeOS R4_ sb16 driver, which is:
 * Copyright (C) 1998 Carlos Hasan. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "driver.h"
#include "hardware.h"

#include <drivers/ISA.h>

static isa_module_info* gISA;

static void
hw_codec_write_byte(sb16_dev_t* dev, uint8 value)
{
	int i;

	/* wait until the DSP is ready to receive data */
	for (i = 0; i < SB16_CODEC_IO_DELAY; i++) {
		if (!(gISA->read_io_8(dev->port + SB16_CODEC_WRITE_STATUS) & 0x80))
			break;
	}

	/* write a byte to the DSP data port */
	gISA->write_io_8(dev->port + SB16_CODEC_WRITE_DATA, value);
}

static int
hw_codec_read_byte(sb16_dev_t* dev)
{
	int i;
	/* wait until the DSP has data available */
	for (i = 0; i < SB16_CODEC_IO_DELAY; i++) {
		if (gISA->read_io_8(dev->port + SB16_CODEC_READ_STATUS) & 0x80)
			break;
	}

	/* read a byte from the DSP data port */
	return gISA->read_io_8(dev->port + SB16_CODEC_READ_DATA);
}

static void
hw_codec_reg_write(sb16_dev_t* dev, uint8 index, uint8 value)
{
	/* write a Mixer indirect register */
	gISA->write_io_8(dev->port + SB16_MIXER_ADDRESS, index);
	gISA->write_io_8(dev->port + SB16_MIXER_DATA, value);
}

static int
hw_codec_reg_read(sb16_dev_t* dev, uint8 index)
{
	/* read a Mixer indirect register */
	gISA->write_io_8(dev->port + SB16_MIXER_ADDRESS, index);
	return gISA->read_io_8(dev->port + SB16_MIXER_DATA);
}


static int
hw_codec_read_version(sb16_dev_t* dev)
{
	int minor, major;

	/* query the DSP hardware version number */
	hw_codec_write_byte(dev, SB16_CODEC_VERSION);
	major = hw_codec_read_byte(dev);
	minor = hw_codec_read_byte(dev);

	dprintf("%s: SB16 version %d.%d\n", __func__, major, minor);

	return (major << 8) + minor;
}

static void
hw_codec_read_irq_setup(sb16_dev_t* dev)
{
	/* query the current IRQ line resource */
	int mask = hw_codec_reg_read(dev, SB16_IRQ_SETUP);

	dev->irq = 5;

	if (mask & 0x01)
		dev->irq = 2;
	if (mask & 0x02)
		dev->irq = 5;
	if (mask & 0x04)
		dev->irq = 7;
	if (mask & 0x08)
		dev->irq = 10;
}

static void
hw_codec_read_dma_setup(sb16_dev_t* dev)
{
    /* query the current DMA channel resources */
    int mask = hw_codec_reg_read(dev, SB16_DMA_SETUP);

    dev->dma8 = 1;

    if (mask & 0x01)
        dev->dma8 = 0;
    if (mask & 0x02)
        dev->dma8 = 1;
    if (mask & 0x08)
        dev->dma8 = 3;

    dev->dma16 = dev->dma8;
    if (mask & 0x20)
        dev->dma16 = 5;
    if (mask & 0x40)
        dev->dma16 = 6;
    if (mask & 0x80)
        dev->dma16 = 7;
}   
        
static void 
hw_codec_write_irq_setup(sb16_dev_t* dev)
{       
	/* change programmable IRQ line resource */
	int mask = 0x02;

	if (dev->irq == 2)
		mask = 0x01;
	if (dev->irq == 5)
		mask = 0x02;
	if (dev->irq == 7)
		mask = 0x04;
	if (dev->irq == 10)
		mask = 0x08;

	hw_codec_reg_write(dev, SB16_IRQ_SETUP, mask);
}

static void
hw_codec_write_dma_setup(sb16_dev_t* dev)
{
	/* change programmable DMA channel resources */
	hw_codec_reg_write(dev, SB16_DMA_SETUP, (1 << dev->dma8) | (1 << dev->dma16));
}

static int32
hw_codec_inth(void* cookie)
{
	sb16_dev_t* dev = (sb16_dev_t*)cookie;
	int32 rc = B_UNHANDLED_INTERRUPT;

	/* read the IRQ interrupt status register */
	int status = hw_codec_reg_read(dev, SB16_IRQ_STATUS);

	/* check if this hardware raised this interrupt */
	if (status & 0x03) {
		rc = B_HANDLED_INTERRUPT;

		/* acknowledge DMA memory transfers */
		if (status & 0x01)
			gISA->read_io_8(dev->port + SB16_CODEC_ACK_8_BIT);
		if (status & 0x02)
			gISA->read_io_8(dev->port + SB16_CODEC_ACK_16_BIT);

		/* acknowledge PIC interrupt signal */
		if (dev->irq >= 8)
			gISA->write_io_8(0xa0, 0x20);

		gISA->write_io_8(0x20, 0x20);

		/* handle buffer finished interrupt */
		if (((dev->playback_stream.bits >> 3) & status) != 0) {
			sb16_stream_buffer_done(&dev->playback_stream);
			rc = B_INVOKE_SCHEDULER;
		}
		if (((dev->record_stream.bits >> 3) & status) != 0) {
			sb16_stream_buffer_done(&dev->record_stream);
			rc = B_INVOKE_SCHEDULER;
		}

		if ((status & 0x04) != 0) {
			/* MIDI stream done */
			rc = B_INVOKE_SCHEDULER;
		}
	}

	return rc;
}



static status_t
hw_codec_reset(sb16_dev_t* dev)
{
	int times, delay;

	/* try to reset the DSP hardware */
	for (times = 0; times < 10; times++) {
		gISA->write_io_8(dev->port + SB16_CODEC_RESET, 1);

		for (delay = 0; delay < SB16_CODEC_RESET_DELAY; delay++)
			gISA->read_io_8(dev->port + SB16_CODEC_RESET);

		gISA->write_io_8(dev->port + SB16_CODEC_RESET, 0);

		if (hw_codec_read_byte(dev) == 0xaa)
			return B_OK;
	}

	return B_IO_ERROR;
}

static status_t
hw_codec_detect(sb16_dev_t* dev)
{
	status_t rc;

	if ((rc=hw_codec_reset(dev)) == B_OK) {
		if (hw_codec_read_version(dev) >= 0x400) {
			hw_codec_write_irq_setup(dev);
			hw_codec_write_dma_setup(dev);
			rc = B_OK;
		} else {
			rc = B_BAD_VALUE;
		}
	}

	return rc;
}

//#pragma mark -

status_t
sb16_stream_setup_buffers(sb16_dev_t* dev, sb16_stream_t* s, const char* desc)
{
	return B_OK;
}

status_t
sb16_stream_start(sb16_dev_t* dev, sb16_stream_t* s)
{
	return B_OK;
}

status_t 
sb16_stream_stop(sb16_dev_t* dev, sb16_stream_t* s)
{
	return B_OK;
}

void
sb16_stream_buffer_done(sb16_stream_t* stream)
{
}

//#pragma mark -

status_t
sb16_hw_init(sb16_dev_t* dev)
{
	status_t rc;

	/* First of all, grab the ISA module */
	if ((rc=get_module(B_ISA_MODULE_NAME, (module_info**)&gISA)) != B_OK)
		return rc;

	/* Check if the hardware is sensible... */
	if ((rc=hw_codec_detect(dev)) == B_OK) {
		if ((rc=gISA->lock_isa_dma_channel(dev->dma8)) == B_OK &&
			(rc=gISA->lock_isa_dma_channel(dev->dma16)) == B_OK) {
			rc = install_io_interrupt_handler(dev->irq, hw_codec_inth, dev, 0);
		}
	}

	return rc;
}

void
sb16_hw_stop(sb16_dev_t* dev)
{
}

void
sb16_hw_uninit(sb16_dev_t* dev)
{
	remove_io_interrupt_handler(dev->irq, hw_codec_inth, dev);

	if (gISA != NULL) {
		gISA->unlock_isa_dma_channel(dev->dma8);

		if (dev->dma8 != dev->dma16)
			gISA->unlock_isa_dma_channel(dev->dma16);

		put_module(B_ISA_MODULE_NAME);
	}

}

