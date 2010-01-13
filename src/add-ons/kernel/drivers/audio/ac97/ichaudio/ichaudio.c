#include "lala/lala.h"
#include "ichaudio.h"
#include "io.h"

status_t ichaudio_attach(audio_drv_t *drv, void **cookie);
status_t ichaudio_powerctl(audio_drv_t *drv, void *cookie, int op);
status_t ichaudio_detach(audio_drv_t *drv, void *cookie);
status_t ichaudio_stream_attach(audio_drv_t *drv, void *cookie, int stream_id);
status_t ichaudio_stream_detach(audio_drv_t *drv, void *cookie, int stream_id);
status_t ichaudio_stream_control(audio_drv_t *drv, void *cookie, int stream_id, int op);
status_t ichaudio_stream_set_buffers(audio_drv_t *drv, void *cookie, int stream_id, uint32 *buffer_size, stream_buffer_desc_t *desc);
status_t ichaudio_stream_set_frame_rate(audio_drv_t *drv, void *cookie, int stream_id, uint32 *frame_rate);

void stream_reset(ichaudio_cookie *cookie, uint32 regbase);


pci_id_table_t ichaudio_id_table[] = {
	{ 0x8086, 0x7195, -1, -1, -1, -1, -1, "Intel 82443MX AC97 audio" },
	{ 0x8086, 0x2415, -1, -1, -1, -1, -1, "Intel 82801AA (ICH) AC97 audio" },
	{ 0x8086, 0x2425, -1, -1, -1, -1, -1, "Intel 82801AB (ICH0) AC97 audio" },
	{ 0x8086, 0x2445, -1, -1, -1, -1, -1, "Intel 82801BA (ICH2) AC97 audio" },
	{ 0x8086, 0x2485, -1, -1, -1, -1, -1, "Intel 82801CA (ICH3) AC97 audio" },
	{ 0x8086, 0x24C5, -1, -1, -1, -1, -1, "Intel 82801DB (ICH4) AC97 audio", (void *)TYPE_ICH4 },
	{ 0x8086, 0x24D5, -1, -1, -1, -1, -1, "Intel 82801EB (ICH5)  AC97 audio", (void *)TYPE_ICH4 },
	{ 0x1039, 0x7012, -1, -1, -1, -1, -1, "SiS SI7012 AC97 audio", (void *)TYPE_SIS7012 },
	{ 0x10DE, 0x01B1, -1, -1, -1, -1, -1, "NVIDIA nForce (MCP)  AC97 audio" },
	{ 0x10DE, 0x006A, -1, -1, -1, -1, -1, "NVIDIA nForce 2 (MCP2)  AC97 audio" },
	{ 0x10DE, 0x00DA, -1, -1, -1, -1, -1, "NVIDIA nForce 3 (MCP3)  AC97 audio" },
	{ 0x1022, 0x764D, -1, -1, -1, -1, -1, "AMD AMD8111 AC97 audio" },
	{ 0x1022, 0x7445, -1, -1, -1, -1, -1, "AMD AMD768 AC97 audio" },
	{ 0x1103, 0x0004, -1, -1, -1, -1, -1, "Highpoint HPT372 RAID" },
	{ 0x1095, 0x3112, -1, -1, -1, -1, -1, "Silicon Image SATA Si3112" },
	{ 0x8086, 0x244b, -1, -1, -1, -1, -1, "Intel IDE controller" },
	{ 0x8979, 0x6456, -1, -1, -1, -1, -1, "does not exist" },
	{ /* empty = end */}
};


driver_info_t driver_info = {
	.basename		= "audio/lala/ichaudio",
	.pci_id_table	= ichaudio_id_table,
	.attach			= ichaudio_attach,
	.detach			= ichaudio_detach,
	.powerctl		= ichaudio_powerctl,
};

enum {
	STREAM_PO = 0,
	STREAM_PI = 1,
};
const uint32 STREAM_REGBASE[2] = { ICH_REG_PO_BASE, ICH_REG_PI_BASE };


stream_info_t po_stream = {
	.flags				= B_PLAYBACK_STREAM | B_BUFFER_SIZE_EXPONETIAL | B_SAMPLE_TYPE_INT16,
	.cookie_size		= sizeof(ichaudio_cookie),
	.sample_bits		= 16,
	.channel_count		= 2,
	.channel_mask		= B_CHANNEL_LEFT | B_CHANNEL_RIGHT,
	.frame_rate_min 	= 0, // is setup later after device probing
	.frame_rate_max		= 0, // is setup later after device probing
	.frame_rate_mask	= 0, // is setup later after device probing
	.buffer_size_min	= 256,
	.buffer_size_max	= 32768,
	.buffer_count		= 2,
	.attach				= ichaudio_stream_attach,
	.detach				= ichaudio_stream_detach,
	.control			= ichaudio_stream_control,
	.set_buffers		= ichaudio_stream_set_buffers,
	.set_frame_rate		= ichaudio_stream_set_frame_rate,
};


stream_info_t pi_stream = {
	.flags				= B_RECORDING_STREAM | B_BUFFER_SIZE_EXPONETIAL | B_SAMPLE_TYPE_INT16,
	.sample_bits		= 16,
	.channel_count		= 2,
	.channel_mask		= B_CHANNEL_LEFT | B_CHANNEL_RIGHT,
	.frame_rate_min 	= 0, // is setup later after device probing
	.frame_rate_max		= 0, // is setup later after device probing
	.frame_rate_mask	= 0, // is setup later after device probing
	.buffer_size_min	= 256,
	.buffer_size_max	= 32768,
	.buffer_count		= 2,
	.attach				= ichaudio_stream_attach,
	.detach				= ichaudio_stream_detach,
	.control			= ichaudio_stream_control,
	.set_buffers		= ichaudio_stream_set_buffers,
	.set_frame_rate		= ichaudio_stream_set_frame_rate,
};

status_t
ichaudio_attach(audio_drv_t *drv, void **_cookie)
{
	ichaudio_cookie *cookie;
	uint32 value;
	int i;
	bigtime_t start;
	bool s0cr, s1cr, s2cr;
	
	dprintf("ichaudio_attach\n");
	
	*_cookie = cookie = (ichaudio_cookie *) malloc(sizeof(ichaudio_cookie));
	
	cookie->pci = drv->pci;
	cookie->flags = (uint32)drv->param;

	// For old ICHs enable programmed IO and busmaster access,
	// for ICH4 and up enable memory mapped IO and busmaster access
	value = drv->pci->read_pci_config(drv->bus, drv->device, drv->function, PCI_command, 2);
	value |= PCI_PCICMD_BME | (cookie->flags & TYPE_ICH4) ? PCI_PCICMD_MSE : PCI_PCICMD_IOS;
	drv->pci->write_pci_config(drv->bus, drv->device, drv->function, PCI_command, 2, value);

	// get IRQ, we can compensate it later if IRQ is not available (hack!)
	cookie->irq = drv->pci->read_pci_config(drv->bus, drv->device, drv->function, PCI_interrupt_line, 1);
	if (cookie->irq == 0xff) cookie->irq = 0;
	if (cookie->irq == 0) dprintf("ichaudio_attach: no interrupt configured\n");

	if (cookie->flags & TYPE_ICH4) {
		// memory mapped access
		uint32 phys_mmbar = PCI_address_memory_32_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x18, 4);
		uint32 phys_mbbar = PCI_address_memory_32_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x1C, 4);
		if (!phys_mmbar || !phys_mbbar) {
			dprintf("ichaudio_attach: memory mapped io unconfigured\n");
			goto err;			
		}
		// map into memory
		cookie->area_mmbar = map_mem(&cookie->mmbar, (void *)phys_mmbar, ICH4_MMBAR_SIZE, 0, "ichaudio mmbar io");
		cookie->area_mbbar = map_mem(&cookie->mbbar, (void *)phys_mbbar, ICH4_MBBAR_SIZE, 0, "ichaudio mbbar io");
		if (cookie->area_mmbar < B_OK || cookie->area_mbbar < B_OK) {
			dprintf("ichaudio_attach: mapping io into memory failed\n");
			goto err;			
		}
	} else {
		// pio access
		cookie->nambar = PCI_address_io_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x10, 4);
		cookie->nabmbar = PCI_address_io_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x14, 4);
		if (!cookie->nambar || !cookie->nabmbar) {
			dprintf("ichaudio_attach: io unconfigured\n");
			goto err;			
		}
	}
	
	/* We don't do a cold reset here, because this would reset general purpose
	 * IO pins of the controller. These pins can be used by the machine vendor
	 * to control external amplifiers, and resetting them prevents audio output
	 * on some notebooks, like "Compaq Presario 2700".
	 * If a cold reset is still in progress, we need to finish it by writing
	 * a 1 to the cold reset bit (CNT_COLD). We do not preserve others bits,
	 * since this can have strange effects (at least on my system, playback
	 * speed is 320% in this case).
	 * Doing a warm reset it required to leave certain power down modes. Warm
	 * reset does also reset GPIO pins, but the ICH hardware does only execute
	 * the reset request if BIT_CLOCK is not running and if it's really required.
	 */
	LOG(("reset starting, ICH_REG_GLOB_CNT = 0x%08lx\n", ich_reg_read_32(cookie, ICH_REG_GLOB_CNT)));
	start = system_time();
	// finish cold reset by writing a 1 and clear all other bits to 0
	ich_reg_write_32(cookie, ICH_REG_GLOB_CNT, CNT_COLD);
	ich_reg_read_32(cookie, ICH_REG_GLOB_CNT); // force PCI-to-PCI bridge cache flush
	snooze(20000);
	// request warm reset by setting the bit, it will clear when reset is done
	ich_reg_write_32(cookie, ICH_REG_GLOB_CNT, ich_reg_read_32(cookie, ICH_REG_GLOB_CNT) | CNT_WARM);
	ich_reg_read_32(cookie, ICH_REG_GLOB_CNT); // force PCI-to-PCI bridge cache flush
	snooze(20000);
	// wait up to 1 second for warm reset to be finished
	for (i = 0; i < 20; i++) {
		value = ich_reg_read_32(cookie, ICH_REG_GLOB_CNT);
		if ((value & CNT_WARM) == 0 && (value & CNT_COLD) != 0)
			break;
		snooze(50000);
	}
	if (i == 20) {
		LOG(("reset failed, ICH_REG_GLOB_CNT = 0x%08lx\n", value));
		goto err;
	}
	LOG(("reset finished after %Ld, ICH_REG_GLOB_CNT = 0x%08lx\n", system_time() - start, value));

	/* detect which codecs are ready */
	s0cr = s1cr = s2cr = false;
	start = system_time();
	do {
		value = ich_reg_read_32(cookie, ICH_REG_GLOB_STA);
		if (!s0cr && (value & STA_S0CR)) {
			s0cr = true;
			LOG(("AC_SDIN0 codec ready after %Ld us\n", system_time() - start));
		}
		if (!s1cr && (value & STA_S1CR)) {
			s1cr = true;
			LOG(("AC_SDIN1 codec ready after %Ld us\n", system_time() - start));
		}
		if (!s2cr && (value & STA_S2CR)) {
			s2cr = true;
			LOG(("AC_SDIN2 codec ready after %Ld us\n", system_time() - start));
		}
		snooze(50000);
	} while ((system_time() - start) < 1000000);

	if (!s0cr) {
		LOG(("AC_SDIN0 codec not ready\n"));
	}
	if (!s1cr) {
		LOG(("AC_SDIN1 codec not ready\n"));
	}
	if (!s2cr) {
		LOG(("AC_SDIN2 codec not ready\n"));
	}

	if (!s0cr && !s1cr && !s2cr) {
		PRINT(("compatible chipset found, but no codec ready!\n"));
		goto err;
	}
	
	if (cookie->flags & TYPE_ICH4) {
		/* we are using a ICH4 chipset, and assume that the codec beeing ready
		 * is the primary one.
		 */
		uint8 sdin;
		uint16 reset;
		uint8 id;
		reset = ich_codec_read(cookie, 0x00);	/* access the primary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("primary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(cookie, ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(cookie, 0x00 + 0x28) >> 14);
			LOG(("primary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		reset = ich_codec_read(cookie, 0x80);	/* access the secondary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("secondary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(cookie, ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(cookie, 0x80 + 0x28) >> 14);
			LOG(("secondary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		reset = ich_codec_read(cookie, 0x100);	/* access the tertiary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("tertiary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(cookie, ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(cookie, 0x100 + 0x28) >> 14);
			LOG(("tertiary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		
		/* XXX this may be wrong */
		ich_reg_write_8(cookie, ICH_REG_SDM, (ich_reg_read_8(cookie, ICH_REG_SDM) & 0x0F) | 0x08 | 0x90);
	} else {
		/* we are using a pre-ICH4 chipset, that has a fixed mapping of
		 * AC_SDIN0 = primary, AC_SDIN1 = secondary codec.
		 */
		if (!s0cr && s2cr) {
			// is is unknown if this really works, perhaps we should better abort here
			LOG(("primary codec doesn't seem to be available, using secondary!\n"));
			cookie->codecoffset = 0x80;
		}
	}

	create_stream(drv, STREAM_PO, &po_stream);
	create_stream(drv, STREAM_PI, &pi_stream);

	return B_OK;

err:
	// delete streams
	delete_stream(drv, STREAM_PO);
	delete_stream(drv, STREAM_PI);
	// unmap io memory
	if (cookie->area_mmbar > 0) delete_area(cookie->area_mmbar);
	if (cookie->area_mbbar > 0) delete_area(cookie->area_mbbar);
	free(cookie);
	return B_ERROR;
}


status_t
ichaudio_detach(audio_drv_t *drv, void *_cookie)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;

	dprintf("ichaudio_detach\n");

	// delete streams
	delete_stream(drv, STREAM_PO);
	delete_stream(drv, STREAM_PI);
	// unmap io memory
	if (cookie->area_mmbar > 0) delete_area(cookie->area_mmbar);
	if (cookie->area_mbbar > 0) delete_area(cookie->area_mbbar);
	free(cookie);
	return B_OK;
}


status_t
ichaudio_powerctl(audio_drv_t *drv, void *_cookie, int op)
{
	return B_ERROR;
}

void
stream_reset(ichaudio_cookie *cookie, uint32 regbase)
{
	int i;

	ich_reg_write_8(cookie, regbase + ICH_REG_X_CR, 0);
	ich_reg_read_8(cookie, regbase + ICH_REG_X_CR); // force PCI-to-PCI bridge cache flush
	snooze(10000); // 10 ms
	ich_reg_write_8(cookie, regbase + ICH_REG_X_CR, CR_RR);
	for (i = 0; i < 100; i++) {
		uint8 cr = ich_reg_read_8(cookie, regbase + ICH_REG_X_CR);
		if (cr == 0) {
			LOG(("channel reset finished, %d\n",i));
			return;
		}
		snooze(100);
	}
	LOG(("channel reset failed after 10ms\n"));
}

status_t
ichaudio_stream_attach(audio_drv_t *drv, void *_cookie, int stream_id)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *) _cookie;
	ichaudio_stream *stream = &cookie->stream[stream_id];
	void *bd_virt_base;
	void *bd_phys_base;
	int i;
	
	stream->regbase = STREAM_REGBASE[stream_id];
	stream->lastindex = 0;
	stream->processed_samples = 0;
	
	stream_reset(cookie, stream->regbase);

	// allocate memory for buffer descriptors
	stream->bd_area = alloc_mem(&bd_virt_base, &bd_phys_base, ICH_BD_COUNT * sizeof(ich_bd), 0, "ich_ac97 buffer descriptor");
	if (stream->bd_area < B_OK)
		goto err;

	// set physical buffer descriptor base address
	ich_reg_write_32(cookie, stream->regbase, (uint32)bd_phys_base);

	// setup descriptor pointers
	for (i = 0; i < ICH_BD_COUNT; i++)
		stream->bd[i] = (ich_bd *) ((char *)bd_virt_base + i * sizeof(ich_bd));
		
	return B_OK;
	
err:
	return B_ERROR;
}


status_t
ichaudio_stream_detach(audio_drv_t *drv, void *_cookie, int stream_id)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;
	ichaudio_stream *stream = &cookie->stream[stream_id];

	stream_reset(cookie, stream->regbase);

	if (stream->buffer_area > 0) delete_area(stream->buffer_area);
	if (stream->bd_area > 0) delete_area(stream->bd_area);
	
	return B_OK;
}


status_t
ichaudio_stream_control(audio_drv_t *drv, void *_cookie, int stream_id, int op)
{
}


status_t
ichaudio_stream_set_buffers(audio_drv_t *drv, void *_cookie, int stream_id, uint32 *buffer_size, stream_buffer_desc_t *desc)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;
	ichaudio_stream *stream = &cookie->stream[stream_id];

	void *buffer_virt_base;
	void *buffer_phys_base;
	area_id buffer_area;
	int i;
	
	// round down (could force another size here)
	*buffer_size &= ~3;
	
	// allocate memory buffers
	buffer_area = alloc_mem(&buffer_virt_base, &buffer_phys_base, 2 * *buffer_size, B_READ_AREA | B_WRITE_AREA, "ich_ac97 buffers");

	// report buffers
	desc[0].base = (char *)buffer_virt_base;
	desc[0].stride = 4;
	desc[1].base = (char *)buffer_virt_base + 2;
	desc[1].stride = 4;

	// initalize buffer descriptors
	for (i = 0; i < ICH_BD_COUNT; i++) {
		stream->bd[i]->buffer = (uint32)buffer_phys_base + (i % 2) * *buffer_size;
		stream->bd[i]->length = *buffer_size / GET_HW_SAMPLE_SIZE(cookie);
		stream->bd[i]->flags = ICH_BDC_FLAG_IOC;
	}

	// free old buffer memory	
	if (stream->buffer_area > 0)
		delete_area(stream->buffer_area);
		
	stream->buffer_area = buffer_area;
	return B_OK;
}


status_t
ichaudio_stream_set_frame_rate(audio_drv_t *drv, void *_cookie, int stream_id, uint32 *frame_rate)
{
}

