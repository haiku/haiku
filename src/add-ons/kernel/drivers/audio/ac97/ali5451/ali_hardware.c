/*
 * Copyright 2009, Krzysztof Ćwiertnia (krzysiek.bmkx_gmail_com).
 *
 * All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <hmulti_audio.h>

#include "ac97.h"
#include "queue.h"
#include "driver.h"
#include "util.h"
#include "ali_hardware.h"
#include "debug.h"


#define ALI_5451_V02 0x02

// ALI registers

#define ALI_MPUR0 0x20
#define ALI_MPUR1 0x21
#define ALI_MPUR2 0x22
#define ALI_MPUR3 0x23

#define ALI_ACR0 0x40
#define ALI_ACR1 0x44
#define ALI_ACR2 0x48

#define ALI_START_A 0x80
#define ALI_STOP_A 0x84

#define ALI_AIN_A 0x98
#define ALI_LFO_A 0xa0
	#define ALI_LFO_A_ENDLP_IE 0x1000
	#define ALI_LFO_A_MIDLP_IE 0x2000
#define ALI_AINTEN_A 0xa4
#define ALI_VOL_A 0xa8
#define ALI_STIMER 0xc8
#define ALI_T_DIGIMIXER 0xd4
#define ALI_AIN_B 0xd8

#define ALI_CSO_ALPHA_FMS 0xe0
#define ALI_PPTR_LBA 0xe4
#define ALI_ESO_DELTA 0xe8
#define ALI_FMC_RVOL_CVOL 0xec
#define ALI_GVSEL_MISC 0xf0
#define ALI_EBUF1 0xf4
#define ALI_EBUF2 0xf8

extern pci_module_info *gPCI;


/* card i/o */


static uint32
io_read32(ali_dev *card, ulong offset)
{
	return (*gPCI->read_io_32)(card->io_base + offset);
}


static uint8
io_read8(ali_dev *card, ulong offset)
{
	return (*gPCI->read_io_8)(card->io_base + offset);
}


static void
io_write32(ali_dev *card, ulong offset, uint32 value)
{
	(*gPCI->write_io_32)(card->io_base + offset, value);
}


static void
io_write16(ali_dev *card, ulong offset, uint16 value)
{
	(*gPCI->write_io_16)(card->io_base + offset, value);
}


static void
io_write8(ali_dev *card, ulong offset, uint8 value)
{
	(*gPCI->write_io_8)(card->io_base + offset, value);
}


/* codec i/o */


static bool
ac97_is_ready(ali_dev *card, uint32 offset)
{
	uint8 cnt = 200;
	uint32 val;

	while (cnt--) {
		val = io_read32(card, offset);
		if (!(val & 0x8000))
			return true;
		spin(1);
	}

	io_write32(card, offset, val & ~0x8000);
	TRACE("ac97 not ready\n");
	return false;
}


static bool
ac97_is_stimer_ready(ali_dev *card)
{
	uint8 cnt = 200;
	uint32 t1, t2;

	t1 = io_read32(card, ALI_STIMER);

	while (cnt--) {
		t2 = io_read32(card, ALI_STIMER);
		if (t2 != t1)
			return true;
		spin(1);
	}

	TRACE("ac97 stimer not ready\n");
	return false;
}


static uint16
ac97_read(void *cookie, uint8 reg)
{
	ali_dev *card = (ali_dev *) cookie;
	uint32 ac_reg;

	ac_reg = (card->info.revision == ALI_5451_V02)?ALI_ACR0:ALI_ACR1;

	if (!ac97_is_ready(card, ac_reg) || !ac97_is_stimer_ready(card))
		return 0;

	io_write16(card, ac_reg, (uint16) reg | 0x8000);

	if (!ac97_is_stimer_ready(card) || !ac97_is_ready(card, ac_reg))
		return 0;

	return (uint16) (io_read32(card, ac_reg) >> 16);
}


static void
ac97_write(void *cookie, uint8 reg, uint16 val)
{
	ali_dev *card = (ali_dev *) cookie;
	uint32 dw_val;

	if (!ac97_is_ready(card, ALI_ACR0) || !ac97_is_stimer_ready(card))
		return;

	dw_val = reg | 0x8000 | (uint32) val << 16;
	if (card->info.revision == ALI_5451_V02)
		dw_val |= 0x0100;

	io_write32(card, ALI_ACR0, dw_val);
}


static bool
hardware_reset(ali_dev *card)
{
	uint32 cmd_reg;
	uint8 bval;

	// pci reset
	cmd_reg = (*gPCI->read_pci_config)(card->info.bus, card->info.device,
		card->info.function, 0x44, 4);
	(*gPCI->write_pci_config)(card->info.bus, card->info.device,
		card->info.function, 0x44, 4, cmd_reg | 0x08000000);
	snooze(500);
	cmd_reg = (*gPCI->read_pci_config)(card->info.bus, card->info.device,
		card->info.function, 0x44, 4);
	(*gPCI->write_pci_config)(card->info.bus, card->info.device,
		card->info.function, 0x44, 4, cmd_reg & 0xfffbffff);
	snooze(5000);

	bval = io_read8(card, ALI_ACR2);
	bval |= 2;
	io_write8(card, ALI_ACR2, bval);
	snooze(5000);
	bval = io_read8(card, ALI_ACR2);
	bval &= 0xfd;
	io_write8(card, ALI_ACR2, bval);
	snooze(15000);

	return true;
}


status_t
hardware_init(ali_dev *card)
{
	if (!hardware_reset(card))
		return B_ERROR;

	LOCK(card->lock_hw);

	io_write32(card, ALI_AINTEN_A, 0);
		// disable voice interrupts
	io_write32(card, ALI_AIN_A, 0xffffffff);
		// clr voice interrupt flags
	io_write32(card, ALI_VOL_A, 0);
		// master & wave volumes
	io_write8(card, ALI_MPUR2, 0x10);

	io_write32(card, ALI_LFO_A, ALI_LFO_A_ENDLP_IE | ALI_LFO_A_MIDLP_IE);

	UNLOCK(card->lock_hw);

	ac97_attach(&card->codec, ac97_read, ac97_write, card,
		card->info.u.h0.subsystem_vendor_id, card->info.u.h0.subsystem_id);

	TRACE("hardware init ok\n");
	return B_OK;
}


void
hardware_terminate(ali_dev *card)
{
	LOCK(card->lock_hw);

	io_write32(card, ALI_AINTEN_A, 0);
		// disable voice interrupts
	io_write32(card, ALI_AIN_A, 0xffffffff);
		// clr voice interrupt flags

	UNLOCK(card->lock_hw);

	ac97_detach(card->codec);
}


uint32
ali_read_int(ali_dev *card)
{
	return io_read32(card, ALI_AIN_A);
}


/* voices handling */


static uint16
ali_delta_from_rate(uint32 rate, bool rec)
{
	uint32 delta;

	if (rate < 8000)
		rate = 8000;
	if (48000 < rate)
		rate = 48000;

	switch (rate) {
		case 8000: delta = (rec?0x6000:0x2ab); break;
		case 44100: delta = (rec?0x116a:0xeb3); break;
		case 48000: delta = 0x1000; break;
		default:
			delta = (rec?((48000 << 12) / rate):(((rate << 12) + rate) / 48000));
	}

	return (uint16) delta;
}


static uint32
ali_control_mode(ali_stream *stream)
{
	uint32 ctrl = 1;

	if (util_format_is_signed(stream->format.format))
		ctrl |= 2;
	if (1 < stream->channels)
		ctrl |= 4;
	if (util_format_to_sample_size(stream->format.format) == 2)
		ctrl |= 8;

	return ctrl;
}


bool
ali_voice_start(ali_stream *stream)
{
	LOCK(stream->card->lock_hw);

	// setup per-voice registers

	io_write8(stream->card, ALI_LFO_A, stream->used_voice);
		// select voice

	io_write32(stream->card, ALI_CSO_ALPHA_FMS, 0);
		// cso = 0, alpha = 0
	io_write32(stream->card, ALI_PPTR_LBA, (uint32) stream->buffer->phy_base);
		// pptr & lba
	io_write32(stream->card, ALI_ESO_DELTA,
		((uint32) stream->buf_frames * stream->buf_count - 1) << 16
		| ali_delta_from_rate(stream->format.rate, stream->rec));
		// eso & delta

	io_write32(stream->card, ALI_GVSEL_MISC,
		ali_control_mode(stream) << 12);
		// gvsel=0, pan=0, vol=0, ctrl, ec=0

	// reset envelopes
	io_write32(stream->card, ALI_EBUF1, 0);
	io_write32(stream->card, ALI_EBUF2, 0);

	UNLOCK(stream->card->lock_hw);

	io_write32(stream->card, ALI_AIN_A, (uint32) 1 << stream->used_voice);
		// clr voice interrupt
	io_write32(stream->card, ALI_AINTEN_A, stream->card->used_voices);
		// enable interupt for voice

	io_write32(stream->card, ALI_START_A, (uint32) 1 << stream->used_voice);
		// start voice

	if (stream->rec)
		io_write32(stream->card, ALI_T_DIGIMIXER,
			(uint32) 1 << stream->used_voice);
				// enable special channel for recording

	return true;
}


void
ali_voice_stop(ali_stream *stream)
{
	io_write32(stream->card, ALI_STOP_A, (uint32) 1 << stream->used_voice);

	if (stream->rec)
		io_write32(stream->card, ALI_T_DIGIMIXER, 0);
}


void
ali_voice_clr_int(ali_stream *stream)
{
	io_write32(stream->card, ALI_AIN_A, (uint32) 1 << stream->used_voice);
}
