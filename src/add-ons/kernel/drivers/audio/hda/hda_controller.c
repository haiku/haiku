#include "driver.h"
#include "hda_controller_defs.h"
#include "hda_codec_defs.h"

#include "driver.h"

void
hda_stream_delete(hda_stream* s)
{
	if (s->buffer_ready_sem >= B_OK)
		delete_sem(s->buffer_ready_sem);
	
	if (s->buffer_area >= B_OK)
		delete_area(s->buffer_area);
	
	if (s->bdl_area >= B_OK)
		delete_area(s->bdl_area);

	free(s);
}

hda_stream*
hda_stream_new(hda_controller* ctrlr, int type)
{
	hda_stream* s = calloc(1, sizeof(hda_stream));
	if (s != NULL) {
		s->buffer_area = B_ERROR;
		s->bdl_area = B_ERROR;
		
		switch(type) {
			case STRM_PLAYBACK:
				s->buffer_ready_sem = create_sem(0, "hda_playback_sem");
				s->id = 1;
				s->off = (ctrlr->num_input_streams * HDAC_SDSIZE);
				ctrlr->streams[ctrlr->num_input_streams] = s;
				break;
				
			case STRM_RECORD:
				s->buffer_area = B_ERROR;
				s->bdl_area = B_ERROR;
				s->buffer_ready_sem = create_sem(0, "hda_record_sem");
				s->id = 2;
				s->off = 0;
				ctrlr->streams[0] = s;
				break;

			default:
				dprintf("%s: Unknown stream type %d!\n", __func__, type);
				free(s);
				s = NULL;
				break;
		}
	}

	return s;
}

status_t
hda_stream_start(hda_controller* ctrlr, hda_stream* s)
{
	OREG8(ctrlr,s->off,CTL0) |= CTL0_RUN;

	while (!(OREG8(ctrlr,s->off,CTL0) & CTL0_RUN))
		snooze(1);

	s->running = true;

	return B_OK;
}

status_t
hda_stream_check_intr(hda_controller* ctrlr, hda_stream* s)
{
	if (s->running) {
		uint8 sts = OREG8(ctrlr,s->off,STS);
		if (sts) {
			cpu_status status;

			OREG8(ctrlr,s->off,STS) = sts;
			status = disable_interrupts();
			acquire_spinlock(&s->lock);

			s->real_time = system_time();
			s->frames_count += s->buffer_length;
			s->buffer_cycle = (s->buffer_cycle +1) % s->num_buffers;

			release_spinlock(&s->lock);
			restore_interrupts(status);

			release_sem_etc(s->buffer_ready_sem, 1, B_DO_NOT_RESCHEDULE);
		}
	}

	return B_OK;
}

status_t
hda_stream_stop(hda_controller* ctrlr, hda_stream* s)
{
	OREG8(ctrlr,s->off,CTL0) &= ~CTL0_RUN;

	while (OREG8(ctrlr,s->off,CTL0) & CTL0_RUN)
		snooze(1);

	s->running = false;

	return B_OK;
}

status_t
hda_stream_setup_buffers(hda_afg* afg, hda_stream* s, const char* desc)
{
	uint32 buffer_size, buffer_pa, alloc;
	uint32 response[2], idx;
	physical_entry pe;
	bdl_entry_t* bdl;
	corb_t verb[2];
	uint8* buffer;
	status_t rc;
	uint16 wfmt;

	/* Clear previously allocated memory */
	if (s->buffer_area >= B_OK) {
		delete_area(s->buffer_area);
		s->buffer_area = B_ERROR;
	}

	if (s->bdl_area >= B_OK) {
		delete_area(s->bdl_area);
		s->bdl_area = B_ERROR;
	}

	/* Calculate size of buffer (aligned to 128 bytes) */		
	buffer_size = s->sample_size * s->num_channels * s->buffer_length;
	buffer_size = (buffer_size + 127) & (~127);
	
	/* Calculate total size of all buffers (aligned to size of B_PAGE_SIZE) */
	alloc = buffer_size * s->num_buffers;
	alloc = (alloc + B_PAGE_SIZE - 1) & (~(B_PAGE_SIZE -1));

	/* Allocate memory for buffers */
	s->buffer_area = create_area("hda_buffers", (void**)&buffer, B_ANY_KERNEL_ADDRESS, alloc, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (s->buffer_area < B_OK)
		return s->buffer_area;

	/* Get the physical address of memory */
	rc = get_memory_map(buffer, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(s->buffer_area);
		return rc;
	}
	
	buffer_pa = (uint32)pe.address;

	dprintf("%s(%s): Allocated %lu bytes for %ld buffers\n", __func__, desc,
		alloc, s->num_buffers);

	/* Store pointers (both virtual/physical) */	
	for (idx=0; idx < s->num_buffers; idx++) {
		s->buffers[idx] = buffer + (idx*buffer_size);
		s->buffers_pa[idx] = buffer_pa + (idx*buffer_size);
	}

	/* Now allocate BDL for buffer range */
	alloc = s->num_buffers * sizeof(bdl_entry_t);
	alloc = (alloc + B_PAGE_SIZE - 1) & (~(B_PAGE_SIZE -1));
	
	s->bdl_area = create_area("hda_bdl", (void**)&bdl, B_ANY_KERNEL_ADDRESS, alloc, B_CONTIGUOUS, 0);
	if (s->bdl_area < B_OK) {
		delete_area(s->buffer_area);
		return s->bdl_area;
	}

	/* Get the physical address of memory */
	rc = get_memory_map(bdl, alloc, &pe, 1);
	if (rc != B_OK) {
		delete_area(s->buffer_area);
		delete_area(s->bdl_area);
		return rc;
	}
	
	s->bdl_pa = (uint32)pe.address;

	dprintf("%s(%s): Allocated %ld bytes for %ld BDLEs\n", __func__, desc,
		alloc, s->num_buffers);
	
	/* Setup BDL entries */	
	for (idx=0; idx < s->num_buffers; idx++, bdl++) {
		bdl->address = buffer_pa + (idx*buffer_size);
		bdl->length = s->sample_size * s->num_channels * s->buffer_length;
		bdl->ioc = 1;
	}

	/* Configure stream registers */
	wfmt = s->num_channels -1;
	switch(s->sampleformat) {
		case B_FMT_8BIT_S:	wfmt |= (0 << 4); break;
		case B_FMT_16BIT:	wfmt |= (1 << 4); break;
		case B_FMT_24BIT:	wfmt |= (3 << 4); break;
		case B_FMT_32BIT:	wfmt |= (4 << 4); break;
		default:			dprintf("%s: Invalid sample format: 0x%lx\n", __func__, s->sampleformat); break;
	}
	switch(s->samplerate) {
		case B_SR_8000:		wfmt |= (7 << 8); break;
		case B_SR_11025:	wfmt |= (67 << 8); break;
		case B_SR_16000:	wfmt |= (2 << 8); break;
		case B_SR_22050:	wfmt |= (65 << 8); break;
		case B_SR_32000:	wfmt |= (10 << 8); break;
		case B_SR_44100:	wfmt |= (64 << 8); break;
		case B_SR_48000:	wfmt |= (0 << 8); break;
		case B_SR_88200:	wfmt |= (72 << 8); break;
		case B_SR_96000:	wfmt |= (8 << 8); break;
		case B_SR_176400:	wfmt |= (88 << 8); break;
		case B_SR_192000:	wfmt |= (24 << 8); break;
		default:			dprintf("%s: Invalid sample rate: 0x%lx\n", __func__, s->samplerate); break;
	}

	OREG16(afg->codec->ctrlr,s->off,FMT) = wfmt;
	OREG32(afg->codec->ctrlr,s->off,BDPL) = s->bdl_pa;
	OREG32(afg->codec->ctrlr,s->off,BDPU) = 0;
	OREG16(afg->codec->ctrlr,s->off,LVI) = s->num_buffers -1;
	OREG32(afg->codec->ctrlr,s->off,CBL) = s->num_channels * s->num_buffers;
	OREG8(afg->codec->ctrlr,s->off,CTL0) = CTL0_IOCE | CTL0_FEIE | CTL0_DEIE;
	OREG8(afg->codec->ctrlr,s->off,CTL2) = s->id << 4;

	verb[0] = MAKE_VERB(afg->codec->addr, s->io_wid, VID_SET_CONVFORMAT, wfmt);
	verb[1] = MAKE_VERB(afg->codec->addr, s->io_wid, VID_SET_CVTSTRCHN, s->id << 4);
	rc = hda_send_verbs(afg->codec, verb, response, 2);

	return rc;	
}

//#pragma mark -

status_t
hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses, int count)
{
	corb_t* corb = codec->ctrlr->corb;
	status_t rc;

	codec->response_count = 0;
	memcpy(corb+(codec->ctrlr->corbwp +1), verbs, sizeof(corb_t)*count);
	REG16(codec->ctrlr,CORBWP) = (codec->ctrlr->corbwp += count);

	rc = acquire_sem_etc(codec->response_sem, count, B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 1000ULL * 50);
	if (rc == B_OK && responses != NULL)
		memcpy(responses, codec->responses, count*sizeof(uint32));

	return rc;
}

static int32
hda_interrupt_handler(hda_controller* ctrlr)
{
	int32 rc = B_UNHANDLED_INTERRUPT;
	
	/* Check if this interrupt is ours */
	uint32 intsts = REG32(ctrlr, INTSTS);
	if (intsts & INTSTS_GIS) {
		rc = B_HANDLED_INTERRUPT;

		/* Controller or stream related? */
		if (intsts & INTSTS_CIS) {
			uint32 statests = REG16(ctrlr,STATESTS);
			uint8 rirbsts = REG8(ctrlr,RIRBSTS);
			uint8 corbsts = REG8(ctrlr,CORBSTS);

			if (statests) {
				/* Detected Codec state change */
				REG16(ctrlr,STATESTS) = statests;
				ctrlr->codecsts = statests;
			}

			/* Check for incoming responses */			
			if (rirbsts) {
				REG8(ctrlr,RIRBSTS) = rirbsts;

				if (rirbsts & RIRBSTS_RINTFL) {
					uint16 rirbwp = REG16(ctrlr,RIRBWP);
					while (ctrlr->rirbrp <= rirbwp) {
						uint32 resp_ex = ctrlr->rirb[ctrlr->rirbrp].resp_ex;
						uint32 cad = resp_ex & HDA_MAXCODECS;
						hda_codec* codec = ctrlr->codecs[cad];

						if (resp_ex & RESP_EX_UNSOL) {
							dprintf("%s: Usolicited response: %08lx/%08lx\n", __func__, 
								ctrlr->rirb[ctrlr->rirbrp].response, resp_ex);
						} else if (codec) {
							/* Store responses in codec */
							codec->responses[codec->response_count++] = ctrlr->rirb[ctrlr->rirbrp].response;
							release_sem_etc(codec->response_sem, 1, B_DO_NOT_RESCHEDULE);
							rc = B_INVOKE_SCHEDULER;
						} else {
							dprintf("%s: Response for unknown codec %ld: %08lx/%08lx\n", __func__, cad, 
								ctrlr->rirb[ctrlr->rirbrp].response, resp_ex);
						}

							
						++ctrlr->rirbrp;
					}			
				}
					
				if (rirbsts & RIRBSTS_OIS)
					dprintf("%s: RIRB Overflow\n", __func__);
			}

			/* Check for sending errors */
			if (corbsts) {
				REG8(ctrlr,CORBSTS) = corbsts;

				if (corbsts & CORBSTS_MEI)
					dprintf("%s: CORB Memory Error!\n", __func__);
			}			
		}

		if (intsts & ~(INTSTS_CIS|INTSTS_GIS)) {
			int idx;
			for (idx=0; idx < HDA_MAXSTREAMS; idx++) {
				if (intsts & (1 << idx)) {
					if (ctrlr->streams[idx])
						hda_stream_check_intr(ctrlr, ctrlr->streams[idx]);
					else
						dprintf("%s: Stream interrupt for unconfigured stream %d!\n", __func__, idx);
				}
			}
		}
		
		/* NOTE: See HDA001 => CIS/GIS cannot be cleared! */
	}

	return rc;
}

static status_t
hda_hw_start(hda_controller* ctrlr)
{
	int timeout = 10;

	/* Put controller out of reset mode */
	REG32(ctrlr,GCTL) |= GCTL_CRST;
	
	do {
		snooze(100);
	} while (--timeout && !(REG32(ctrlr,GCTL) & GCTL_CRST));	
	
	return timeout ? B_OK : B_TIMED_OUT;
}

static status_t
hda_hw_corb_rirb_init(hda_controller* ctrlr)
{
	uint32 memsz, rirboff;
	uint8 corbsz, rirbsz;
	status_t rc = B_OK;
	physical_entry pe;

	/* Determine and set size of CORB */
	corbsz = REG8(ctrlr,CORBSIZE);
	if (corbsz & CORBSIZE_CAP_256E) {
		ctrlr->corblen = 256;
		REG8(ctrlr,CORBSIZE) = CORBSIZE_SZ_256E;
	} else if (corbsz & CORBSIZE_CAP_16E) {
		ctrlr->corblen = 16;
		REG8(ctrlr,CORBSIZE) = CORBSIZE_SZ_16E;
	} else if (corbsz & CORBSIZE_CAP_2E) {
		ctrlr->corblen = 2;
		REG8(ctrlr,CORBSIZE) = CORBSIZE_SZ_2E;
	}

	/* Determine and set size of RIRB */
	rirbsz = REG8(ctrlr,RIRBSIZE);
	if (rirbsz & RIRBSIZE_CAP_256E) {
		ctrlr->rirblen = 256;
		REG8(ctrlr,RIRBSIZE) = RIRBSIZE_SZ_256E;
	} else if (rirbsz & RIRBSIZE_CAP_16E) {
		ctrlr->rirblen = 16;
		REG8(ctrlr,RIRBSIZE) = RIRBSIZE_SZ_16E;
	} else if (rirbsz & RIRBSIZE_CAP_2E) {
		ctrlr->rirblen = 2;
		REG8(ctrlr,RIRBSIZE) = RIRBSIZE_SZ_2E;
	}

	/* Determine rirb offset in memory and total size of corb+alignment+rirb */
	rirboff = (ctrlr->corblen * sizeof(corb_t) + 0x7f) & ~0x7f;
	memsz = ((B_PAGE_SIZE -1) + 
			rirboff + 
			(ctrlr->rirblen * sizeof(rirb_t))) & ~(B_PAGE_SIZE-1);

	/* Allocate memory area */
	ctrlr->rb_area = create_area("hda_corb_rirb", 
		(void**)&ctrlr->corb, B_ANY_KERNEL_ADDRESS, memsz, B_CONTIGUOUS, 0);
	if (ctrlr->rb_area < 0) {
		return ctrlr->rb_area;
	}

	/* Rirb is after corb+aligment */
	ctrlr->rirb = (rirb_t*)(((uint8*)ctrlr->corb)+rirboff);
	
	if ((rc=get_memory_map(ctrlr->corb, memsz, &pe, 1)) != B_OK) {
		delete_area(ctrlr->rb_area);
		return rc;
	}
	
	/* Program CORB/RIRB for these locations */
	REG32(ctrlr,CORBLBASE) = (uint32)pe.address;
	REG32(ctrlr,RIRBLBASE) = (uint32)pe.address + rirboff;
	
	/* Reset CORB read pointer */
	/* NOTE: See HDA011 for corrected procedure! */
	REG16(ctrlr,CORBRP) = CORBRP_RST;
	do {
		snooze(10);
	} while ( !(REG16(ctrlr,CORBRP) & CORBRP_RST) );
	REG16(ctrlr,CORBRP) = 0;
	
	/* Reset RIRB write pointer */
	REG16(ctrlr,RIRBWP) = RIRBWP_RST;
	
	/* Generate interrupt for every response */
	REG16(ctrlr,RINTCNT) = 1;

	/* Setup cached read/write indices */
	ctrlr->rirbrp = 1;
	ctrlr->corbwp = 0;
	
	/* Gentlemen, start your engines... */
	REG8(ctrlr,CORBCTL) = CORBCTL_RUN | CORBCTL_MEIE;
	REG8(ctrlr,RIRBCTL) = RIRBCTL_DMAEN | RIRBCTL_OIC | RIRBCTL_RINTCTL;

	return B_OK;
}



//#pragma mark -

/* Setup hardware for use; detect codecs; etc */
status_t
hda_hw_init(hda_controller* ctrlr)
{
	status_t rc;
	uint16 gcap;
	uint32 idx;

	/* Map MMIO registers */
	ctrlr->regs_area = 
		map_physical_memory("hda_hw_regs", (void*)ctrlr->pcii.u.h0.base_registers[0],
			ctrlr->pcii.u.h0.base_register_sizes[0], B_ANY_KERNEL_ADDRESS, 0, 
			(void**)&ctrlr->regs);

	if (ctrlr->regs_area < B_OK) {
		rc = ctrlr->regs_area;
		goto error;
	}

	/* Absolute minimum hw is online; we can now install interrupt handler */
	ctrlr->irq = ctrlr->pcii.u.h0.interrupt_line;
	rc = install_io_interrupt_handler(ctrlr->irq,
			(interrupt_handler)hda_interrupt_handler, ctrlr, 0);
	if (rc != B_OK)
		goto no_irq;

	/* show some hw features */
	gcap = REG16(ctrlr,GCAP);
	dprintf("HDA: HDA v%d.%d, O:%d/I:%d/B:%d, #SDO:%d, 64bit:%s\n", 
		REG8(ctrlr,VMAJ), REG8(ctrlr,VMIN),
		GCAP_OSS(gcap), GCAP_ISS(gcap), GCAP_BSS(gcap),
		GCAP_NSDO(gcap) ? GCAP_NSDO(gcap) *2 : 1, 
		gcap & GCAP_64OK ? "yes" : "no" );

	ctrlr->num_input_streams = GCAP_OSS(gcap);
	ctrlr->num_output_streams = GCAP_ISS(gcap);
	ctrlr->num_bidir_streams = GCAP_BSS(gcap);

	/* Get controller into valid state */
	rc = hda_hw_start(ctrlr);
	if (rc != B_OK)
		goto reset_failed;

	/* Setup CORB/RIRB */
	rc = hda_hw_corb_rirb_init(ctrlr);
	if (rc != B_OK)
		goto corb_rirb_failed;

	REG16(ctrlr,WAKEEN) = 0x7fff;

	/* Enable controller interrupts */
	REG32(ctrlr,INTCTL) = INTCTL_GIE | INTCTL_CIE | 0xffff;

	/* Wait for codecs to warm up */
	snooze(1000);

	if (!ctrlr->codecsts) {	
		rc = ENODEV;
		goto corb_rirb_failed;
	}

	for (idx=0; idx < HDA_MAXCODECS; idx++)
		if (ctrlr->codecsts & (1 << idx))
			hda_codec_new(ctrlr, idx);
	
	return B_OK;

corb_rirb_failed:
	REG32(ctrlr,INTCTL) = 0;

reset_failed:
	remove_io_interrupt_handler(ctrlr->irq,
		(interrupt_handler)hda_interrupt_handler,
		ctrlr);

no_irq:
	delete_area(ctrlr->regs_area);
	ctrlr->regs_area = B_ERROR;
	ctrlr->regs = NULL;

error:
	dprintf("ERROR: %s(%ld)\n", strerror(rc), rc);

	return rc;
}

/* Stop any activity */
void
hda_hw_stop(hda_controller* ctrlr)
{
	int idx;
	
	/* Stop all audio streams */
	for (idx=0; idx < HDA_MAXSTREAMS; idx++)
		if (ctrlr->streams[idx] && ctrlr->streams[idx]->running)
			hda_stream_stop(ctrlr, ctrlr->streams[idx]);
}

/* Free resources */
void
hda_hw_uninit(hda_controller* ctrlr)
{
	if (ctrlr != NULL) {
		uint32 idx;

		/* Stop all audio streams */
		hda_hw_stop(ctrlr);
		
		/* Stop CORB/RIRB */
		REG8(ctrlr,CORBCTL) = 0;
		REG8(ctrlr,RIRBCTL) = 0;

		/* Disable interrupts and remove interrupt handler */
		REG32(ctrlr,INTCTL) = 0;
		REG32(ctrlr,GCTL) &= ~GCTL_CRST;
		remove_io_interrupt_handler(ctrlr->irq,
			(interrupt_handler)hda_interrupt_handler,
			ctrlr);

		/* Delete corb/rirb area */
		if (ctrlr->rb_area >= 0) {
			delete_area(ctrlr->rb_area);
			ctrlr->rb_area = B_ERROR;
			ctrlr->corb = NULL;
			ctrlr->rirb = NULL;
		}
	
		/* Unmap registers */
		if (ctrlr->regs_area >= 0) {
			delete_area(ctrlr->regs_area);
			ctrlr->regs_area = B_ERROR;
			ctrlr->regs = NULL;
		}
		
		/* Now delete all codecs */
		for (idx=0; idx < HDA_MAXCODECS; idx++)
			if (ctrlr->codecs[idx] != NULL)
				hda_codec_delete(ctrlr->codecs[idx]);
	}
}
