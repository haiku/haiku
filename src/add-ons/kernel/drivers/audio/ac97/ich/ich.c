/*
 * BeOS Driver for Intel ICH AC'97 Link interface
 *
 * Copyright (c) 2002, Marcus Overhagen <marcus@overhagen.de>
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

//#define DEBUG 1

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <OS.h>
#include <PCI.h>
#include <malloc.h>
#include "debug.h"
#include "config.h"
#include "io.h"
#include "ich.h"
#include "util.h"
#include "ac97_multi.h"
#include "ac97.h"

int32 			api_version = B_CUR_DRIVER_API_VERSION;
pci_module_info *pci;

volatile bool	int_thread_exit = false;
thread_id 		int_thread_id = -1;
ich_chan *		chan_pi = 0;
ich_chan *		chan_po = 0;
ich_chan *		chan_mc = 0;

int32 		ich_int(void *data);
int32 		ich_test_int(void *check);
bool 		interrupt_test(void);
void 		init_chan(ich_chan *chan);
void 		reset_chan(ich_chan *chan);
void 		chan_free_resources(void);
status_t 	map_io_memory(void);
status_t 	unmap_io_memory(void);
int32 		int_thread(void *data);
void		stop_chan(ich_chan *chan);

#if DEBUG > 2
void dump_chan(ich_chan *chan)
{
	int i;
	LOG(("chan->regbase = %#08x\n",chan->regbase));
	LOG(("chan->buffer_ready_sem = %#08x\n",chan->buffer_ready_sem));

	LOG(("chan->buffer_log_base = %#08x\n",chan->buffer_log_base));
	LOG(("chan->buffer_phy_base = %#08x\n",chan->buffer_phy_base));
	LOG(("chan->bd_phy_base = %#08x\n",chan->bd_phy_base));
	LOG(("chan->bd_log_base = %#08x\n",chan->bd_log_base));
	for (i = 0; i < ICH_BD_COUNT; i++) {
		LOG(("chan->buffer[%d] = %#08x\n",i,chan->buffer[i]));
	}
	for (i = 0; i < ICH_BD_COUNT; i++) {
		LOG(("chan->bd[%d] = %#08x\n",i,chan->bd[i]));
		LOG(("chan->bd[%d]->buffer = %#08x\n",i,chan->bd[i]->buffer));
		LOG(("chan->bd[%d]->length = %#08x\n",i,chan->bd[i]->length));
		LOG(("chan->bd[%d]->flags = %#08x\n",i,chan->bd[i]->flags));
	}
}
#endif

void init_chan(ich_chan *chan)
{
	int i;
	ASSERT(BUFFER_COUNT <= ICH_BD_COUNT);
	LOG(("init chan\n"));

	chan->lastindex = 0;
	chan->played_frames_count = 0;
	chan->played_real_time = 0;
	chan->running = false;

	for (i = 0; i < BUFFER_COUNT; i++) {
		chan->userbuffer[i] = ((char *)chan->userbuffer_base) + i * BUFFER_SIZE;
	}
	for (i = 0; i < ICH_BD_COUNT; i++) {
		chan->buffer[i] = ((char *)chan->buffer_log_base) + (i % BUFFER_COUNT) * BUFFER_SIZE;
		chan->bd[i] = (ich_bd *) (((char *)chan->bd_log_base) + i * sizeof(ich_bd));	
		chan->bd[i]->buffer = ((uint32)chan->buffer_phy_base) + (i % BUFFER_COUNT) * BUFFER_SIZE;
		chan->bd[i]->length = BUFFER_SIZE / GET_HW_SAMPLE_SIZE(config);
		chan->bd[i]->flags = ICH_BDC_FLAG_IOC;
	}
	
	// set physical buffer descriptor base address
	ich_reg_write_32(chan->regbase, (uint32)chan->bd_phy_base);

	LOG(("init chan finished\n"));
}

void start_chan(ich_chan *chan)
{
	int32 civ;
	
	if (chan->running)
		return;

	#if DEBUG > 2
		dump_chan(chan);
	#endif

	civ = ich_reg_read_8(chan->regbase + ICH_REG_X_CIV);

	chan->lastindex = civ;
	chan->played_frames_count = -BUFFER_FRAMES_COUNT; /* gets bumped up to 0 in the first interrupt before playback starts */
	chan->played_real_time = 0;

	// step 1: clear status bits
	ich_reg_write_16(chan->regbase + GET_REG_X_SR(config), SR_FIFOE | SR_BCIS | SR_LVBCI); 
	// step 2: prepare buffer transfer
	ich_reg_write_8(chan->regbase + ICH_REG_X_LVI, (civ + 28) % ICH_BD_COUNT);
	// step 3: enable interrupts & busmaster transfer
	ich_reg_write_8(chan->regbase + ICH_REG_X_CR, CR_RPBM | CR_LVBIE | CR_IOCE);

	chan->running = true;
}

void stop_chan(ich_chan *chan)
{
	ich_reg_write_8(chan->regbase + ICH_REG_X_CR, ich_reg_read_8(chan->regbase + ICH_REG_X_CR) & ~CR_RPBM);
	ich_reg_read_8(chan->regbase + ICH_REG_X_CR); // force PCI-to-PCI bridge cache flush
	chan->running = false;
	snooze(10000); // 10 ms
}

void reset_chan(ich_chan *chan)
{
	int i, cr;
	ich_reg_write_8(chan->regbase + ICH_REG_X_CR, 0);
	ich_reg_read_8(chan->regbase + ICH_REG_X_CR); // force PCI-to-PCI bridge cache flush
	snooze(10000); // 10 ms

	chan->running = false;

	ich_reg_write_8(chan->regbase + ICH_REG_X_CR, CR_RR);
	for (i = 0; i < 10000; i++) {
		cr = ich_reg_read_8(chan->regbase + ICH_REG_X_CR);
		if (cr == 0) {
			LOG(("channel reset finished, %d\n",i));
			return;
		}
		snooze(1);
	}
	LOG(("channel reset failed after 10ms\n"));
}

bool interrupt_test(void)
{
	bool *testresult;
	bool result;
	bigtime_t duration;
	int i;

	LOG(("testing if interrupt is working\n"));
	
	// our stack is not mapped in interrupt context, we must use malloc
	// to have a valid pointer inside the interrupt handler
	testresult = malloc(sizeof(bool)); 
	*testresult = false; // assume it's not working
	
	install_io_interrupt_handler(config->irq, ich_test_int, testresult, 0);

	// clear status bits
	ich_reg_write_16(chan_po->regbase + GET_REG_X_SR(config), SR_FIFOE | SR_BCIS | SR_LVBCI); 
	// start transfer of 1 buffer
	ich_reg_write_8(chan_po->regbase + ICH_REG_X_LVI, (ich_reg_read_8(chan_po->regbase + ICH_REG_X_LVI) + 1) % ICH_BD_COUNT);
	// enable interrupts & busmaster transfer
	ich_reg_write_8(chan_po->regbase + ICH_REG_X_CR, CR_RPBM | CR_LVBIE | CR_IOCE);

	// the interrupt handler will set *testresult to true
	duration = system_time();
	for (i = 0; i < 500; i++) {
		if (*testresult)
			break;
		else
			snooze(1000); // 1 ms
	}
	duration = system_time() - duration;

	// disable interrupts & busmaster transfer
	ich_reg_write_8(chan_po->regbase + ICH_REG_X_CR, 0);
	ich_reg_read_8(chan_po->regbase + ICH_REG_X_CR); // force PCI-to-PCI bridge cache flush

	snooze(25000);

	remove_io_interrupt_handler(config->irq, ich_test_int, testresult);
	result = *testresult;
	free(testresult);

	#if DEBUG
		if (result) {
			LOG(("got interrupt after %Lu us\n",duration));
		} else {
			LOG(("no interrupt, timeout after %Lu us\n",duration));
		}
	#endif

	return result;
}

void chan_free_resources(void)
{
	if (chan_po) {
		if (chan_po->buffer_ready_sem > B_OK)
			delete_sem(chan_po->buffer_ready_sem);
		if (chan_po->bd_area > B_OK)
			delete_area(chan_po->bd_area);
		if (chan_po->buffer_area > B_OK)
			delete_area(chan_po->buffer_area);
		if (chan_po->userbuffer_area > B_OK)
			delete_area(chan_po->userbuffer_area);
		free(chan_po);
	}
	if (chan_pi) {
		if (chan_pi->buffer_ready_sem > B_OK)
			delete_sem(chan_pi->buffer_ready_sem);
		if (chan_pi->bd_area > B_OK)
			delete_area(chan_pi->bd_area);
		if (chan_pi->buffer_area > B_OK)
			delete_area(chan_pi->buffer_area);
		if (chan_pi->userbuffer_area > B_OK)
			delete_area(chan_pi->userbuffer_area);
		free(chan_pi);
	}
	if (chan_mc) {
		if (chan_mc->buffer_ready_sem > B_OK)
			delete_sem(chan_mc->buffer_ready_sem);
		if (chan_mc->bd_area > B_OK)
			delete_area(chan_mc->bd_area);
		if (chan_mc->buffer_area > B_OK)
			delete_area(chan_mc->buffer_area);
		if (chan_mc->userbuffer_area > B_OK)
			delete_area(chan_mc->userbuffer_area);
		free(chan_mc);
	}
}


status_t map_io_memory(void)
{
	if ((config->type & TYPE_ICH4) == 0)
		return B_OK;

	ASSERT(config->log_mmbar == 0);
	ASSERT(config->log_mbbar == 0);
	ASSERT(config->mmbar != 0);
	ASSERT(config->mbbar != 0);
	
	config->area_mmbar = map_mem(&config->log_mmbar, (void *)config->mmbar, ICH4_MMBAR_SIZE, "ich_ac97 mmbar io");
	if (config->area_mmbar <= B_OK) {
		LOG(("mapping of mmbar io failed, error = %#x\n",config->area_mmbar));
		return B_ERROR;
	}
	LOG(("mapping of mmbar: area %#x, phys %#x, log %#x\n", config->area_mmbar, config->mmbar, config->log_mmbar));

	config->area_mbbar = map_mem(&config->log_mbbar, (void *)config->mbbar, ICH4_MBBAR_SIZE, "ich_ac97 mbbar io");
	if (config->area_mbbar <= B_OK) {
		LOG(("mapping of mbbar io failed, error = %#x\n",config->area_mbbar));
		delete_area(config->area_mmbar);
		config->area_mmbar = -1;
		return B_ERROR;
	}
	LOG(("mapping of mbbar: area %#x, phys %#x, log %#x\n", config->area_mbbar, config->mbbar, config->log_mbbar));

	return B_OK;
}

status_t unmap_io_memory(void)
{
	status_t rv;
	if ((config->type & TYPE_ICH4) == 0)
		return B_OK;
	rv  = delete_area(config->area_mmbar);
	rv |= delete_area(config->area_mbbar);
	return rv;
}

int32 int_thread(void *data)
{
	cpu_status status;
	while (!int_thread_exit) {
		status = disable_interrupts();
		ich_int(data);
		restore_interrupts(status);
		snooze(1500);
	}
	return 0;
}

status_t
init_hardware(void)
{
	status_t err = ENODEV;
	LOG_CREATE();

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
		return ENOSYS;

	if (B_OK == probe_device()) {
		PRINT(("ALL YOUR BASE ARE BELONG TO US\n"));
		err = B_OK;
	} else {
		LOG(("hardware not found\n"));
	}
	
	put_module(B_PCI_MODULE_NAME);
	return err;
}


static void 
dump_hardware_regs()
{
	LOG(("GLOB_CNT = %#08x\n",ich_reg_read_32(ICH_REG_GLOB_CNT)));
	LOG(("GLOB_STA = %#08x\n",ich_reg_read_32(ICH_REG_GLOB_STA)));
	LOG(("PI ICH_REG_X_BDBAR = %#x\n",ich_reg_read_32(ICH_REG_X_BDBAR + ICH_REG_PI_BASE)));
	LOG(("PI ICH_REG_X_CIV = %#x\n",ich_reg_read_8(ICH_REG_X_CIV + ICH_REG_PI_BASE)));
	LOG(("PI ICH_REG_X_LVI = %#x\n",ich_reg_read_8(ICH_REG_X_LVI + ICH_REG_PI_BASE)));
	LOG(("PI     REG_X_SR = %#x\n",ich_reg_read_16(GET_REG_X_SR(config) + ICH_REG_PI_BASE)));
	LOG(("PI     REG_X_PICB = %#x\n",ich_reg_read_16(GET_REG_X_PICB(config) + ICH_REG_PI_BASE)));
	LOG(("PI ICH_REG_X_PIV = %#x\n",ich_reg_read_8(ICH_REG_X_PIV + ICH_REG_PI_BASE)));
	LOG(("PI ICH_REG_X_CR = %#x\n",ich_reg_read_8(ICH_REG_X_CR + ICH_REG_PI_BASE)));
	LOG(("PO ICH_REG_X_BDBAR = %#x\n",ich_reg_read_32(ICH_REG_X_BDBAR + ICH_REG_PO_BASE)));
	LOG(("PO ICH_REG_X_CIV = %#x\n",ich_reg_read_8(ICH_REG_X_CIV + ICH_REG_PO_BASE)));
	LOG(("PO ICH_REG_X_LVI = %#x\n",ich_reg_read_8(ICH_REG_X_LVI + ICH_REG_PO_BASE)));
	LOG(("PO     REG_X_SR = %#x\n",ich_reg_read_16(GET_REG_X_SR(config) + ICH_REG_PO_BASE)));
	LOG(("PO     REG_X_PICB = %#x\n",ich_reg_read_16(GET_REG_X_PICB(config) + ICH_REG_PO_BASE)));
	LOG(("PO ICH_REG_X_PIV = %#x\n",ich_reg_read_8(ICH_REG_X_PIV + ICH_REG_PO_BASE)));
	LOG(("PO ICH_REG_X_CR = %#x\n",ich_reg_read_8(ICH_REG_X_CR + ICH_REG_PO_BASE)));
}

static uint16
ac97_reg_read(void *cookie, uint8 reg)
{
	return ich_codec_read(config->codecoffset + reg);
}

static void
ac97_reg_write(void *cookie, uint8 reg, uint16 value)
{
	ich_codec_write(config->codecoffset + reg, value);
}

static uint32
ich_clock_get()
{
	uint32 civ1, civ2, picb;
	uint32 startframe;
	uint32 stopframe;
	uint32 rate;
	bigtime_t starttime;
	bigtime_t stoptime;

	do {
		civ1 = ich_reg_read_8 (ICH_REG_PO_BASE + ICH_REG_X_CIV);
		picb = ich_reg_read_16(ICH_REG_PO_BASE + GET_REG_X_PICB(config));
		civ2 = ich_reg_read_8 (ICH_REG_PO_BASE + ICH_REG_X_CIV);
	} while (civ1 != civ2);
	starttime = system_time();
	startframe = (civ1 * (BUFFER_SIZE / 4));
	picb = (picb * GET_HW_SAMPLE_SIZE(config)) / 4; /* convert picb into frames */
	if (picb <= (BUFFER_SIZE / 4))
		startframe += (BUFFER_SIZE / 4) - picb;
	else
		LOG(("wrong picb = %d, max %d\n", picb, BUFFER_SIZE / 4));
		
	snooze(40000);
	
	do {
		civ1 = ich_reg_read_8 (ICH_REG_PO_BASE + ICH_REG_X_CIV);
		picb = ich_reg_read_16(ICH_REG_PO_BASE + GET_REG_X_PICB(config));
		civ2 = ich_reg_read_8 (ICH_REG_PO_BASE + ICH_REG_X_CIV);
	} while (civ1 != civ2);
	stoptime = system_time();
	stopframe = (civ1 * (BUFFER_SIZE / 4));
	picb = (picb * GET_HW_SAMPLE_SIZE(config)) / 4; /* convert picb into frames */
	if (picb <= (BUFFER_SIZE / 4))
		stopframe += (BUFFER_SIZE / 4) - picb;
	else
		LOG(("wrong picb = %d, max %d\n", picb, BUFFER_SIZE / 4));

	if (stopframe < startframe)
		stopframe += ICH_BD_COUNT * (BUFFER_SIZE / 4);

	rate = (1000000LL * (stopframe - startframe)) / (stoptime - starttime);
	return rate;
}

static void
ich_clock_calibrate()
{
	uint32 rate;

	if (!ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000)) {
		LOG(("ich_clock_calibrate: can't set default clock rate\n"));
		return;
	}

	start_chan(chan_po);

	#if DEBUG > 0
		{
			int i;
			uint32 r = 0;
			for (i = 0; i < 20; i++) {
				uint32 c = ich_clock_get();
				LOG(("ich_clock_calibrate: rate %ld\n", c));
				r += c;
			}
			r /= 20;
			LOG(("ich_clock_calibrate: mean rate %ld\n", r));
		}
	#endif

	rate = ich_clock_get();
	LOG(("ich_clock_calibrate: rate %ld\n", rate));

	if (rate > (36898 - 1000) && rate < (36898 + 1000)) {
		LOG(("ich_clock_calibrate: setting clock 36898\n"));
		ac97_set_clock(config->ac97, 36898);
	} else if (rate > (44100 - 1000) && rate < (44100 + 1000)) {
		LOG(("ich_clock_calibrate: setting clock 44100\n"));
		ac97_set_clock(config->ac97, 44100);
	} else if (rate > (48000 - 1000) && rate < (48000 + 1000)) {
		LOG(("ich_clock_calibrate: setting clock 48000\n"));
		ac97_set_clock(config->ac97, 48000);
	} else if (rate > (55930 - 1000) && rate < (55930 + 1000)) {
		LOG(("ich_clock_calibrate: setting clock 55930\n"));
		ac97_set_clock(config->ac97, 55930);
	} else {
		LOG(("ich_clock_calibrate: NOT setting clock\n"));
	}

	if (	!ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 44100)
		&&	!ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000)) {
		LOG(("ich_clock_calibrate: setting rates doesn't work with clock %d\n", config->ac97->clock));
		LOG(("ich_clock_calibrate: reset clock\n"));
		ac97_set_clock(config->ac97, 48000);
		ac97_set_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, 48000);
	}

	stop_chan(chan_po);
}

status_t
init_driver(void)
{
	int i;
	uint32 val;
	status_t rv;
	bigtime_t start;
	bool s0cr, s1cr, s2cr;

	LOG_CREATE();
	
	LOG(("init_driver\n"));
	
	ASSERT(sizeof(ich_bd) == 8);

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci))
                return ENOSYS;
	
	rv = probe_device();
	if (rv != B_OK) {
		LOG(("No supported audio hardware found.\n"));
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	PRINT((INFO "\n"));
	PRINT(("found %s\n", config->name));
	PRINT(("IRQ = %ld, NAMBAR = %#lX, NABMBAR = %#lX, MMBAR = %#lX, MBBAR = %#lX\n",config->irq,config->nambar,config->nabmbar,config->mmbar,config->mbbar));

	/* before doing anything else, map the IO memory */
	rv = map_io_memory();
	if (rv != B_OK) {
		PRINT(("mapping of memory IO space failed\n"));
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	
	dump_hardware_regs();

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
	LOG(("reset starting, ICH_REG_GLOB_CNT = 0x%08x\n", ich_reg_read_32(ICH_REG_GLOB_CNT)));
	DEBUG_ONLY(start = system_time());
	// finish cold reset by writing a 1 and clear all other bits to 0
	ich_reg_write_32(ICH_REG_GLOB_CNT, CNT_COLD);
	ich_reg_read_32(ICH_REG_GLOB_CNT); // force PCI-to-PCI bridge cache flush
	snooze(20000);
	// request warm reset by setting the bit, it will clear when reset is done
	ich_reg_write_32(ICH_REG_GLOB_CNT, ich_reg_read_32(ICH_REG_GLOB_CNT) | CNT_WARM);
	ich_reg_read_32(ICH_REG_GLOB_CNT); // force PCI-to-PCI bridge cache flush
	snooze(20000);
	// wait up to 1 second for warm reset to be finished
	for (i = 0; i < 20; i++) {
		val = ich_reg_read_32(ICH_REG_GLOB_CNT);
		if ((val & CNT_WARM) == 0 && (val & CNT_COLD) != 0)
			break;
		snooze(50000);
	}
	if (i == 20) {
		LOG(("reset failed, ICH_REG_GLOB_CNT = 0x%08x\n", val));
		unmap_io_memory();
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}
	LOG(("reset finished after %Ld, ICH_REG_GLOB_CNT = 0x%08x\n", system_time() - start, val));

	/* detect which codecs are ready */
	s0cr = s1cr = s2cr = false;
	start = system_time();
	do {
		val = ich_reg_read_32(ICH_REG_GLOB_STA);
		if (!s0cr && (val & STA_S0CR)) {
			s0cr = true;
			LOG(("AC_SDIN0 codec ready after %Ld us\n",(system_time() - start)));
		}
		if (!s1cr && (val & STA_S1CR)) {
			s1cr = true;
			LOG(("AC_SDIN1 codec ready after %Ld us\n",(system_time() - start)));
		}
		if (!s2cr && (val & STA_S2CR)) {
			s2cr = true;
			LOG(("AC_SDIN2 codec ready after %Ld us\n",(system_time() - start)));
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

	dump_hardware_regs();

	if (!s0cr && !s1cr && !s2cr) {
		PRINT(("compatible chipset found, but no codec ready!\n"));
		unmap_io_memory();
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	if (config->type & TYPE_ICH4) {
		/* we are using a ICH4 chipset, and assume that the codec beeing ready
		 * is the primary one.
		 */
		uint8 sdin;
		uint16 reset;
		uint8 id;
		reset = ich_codec_read(0x00);	/* access the primary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("primary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(0x00 + 0x28) >> 14);
			LOG(("primary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		reset = ich_codec_read(0x80);	/* access the secondary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("secondary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(0x80 + 0x28) >> 14);
			LOG(("secondary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		reset = ich_codec_read(0x100);	/* access the tertiary codec */
		if (reset == 0 || reset == 0xFFFF) {
			LOG(("tertiary codec not present\n"));
		} else {
			sdin = 0x02 & ich_reg_read_8(ICH_REG_SDM);
			id = 0x02 & (ich_codec_read(0x100 + 0x28) >> 14);
			LOG(("tertiary codec id %d is connected to AC_SDIN%d\n", id, sdin));
		}
		
		/* XXX this may be wrong */
		ich_reg_write_8(ICH_REG_SDM, (ich_reg_read_8(ICH_REG_SDM) & 0x0F) | 0x08 | 0x90);
	} else {
		/* we are using a pre-ICH4 chipset, that has a fixed mapping of
		 * AC_SDIN0 = primary, AC_SDIN1 = secondary codec.
		 */
		if (!s0cr && s2cr) {
			// is is unknown if this really works, perhaps we should better abort here
			LOG(("primary codec doesn't seem to be available, using secondary!\n"));
			config->codecoffset = 0x80;
		}
	}

	dump_hardware_regs();

	/* allocate memory for channel info struct */
	chan_pi = (ich_chan *) malloc(sizeof(ich_chan));
	chan_po = (ich_chan *) malloc(sizeof(ich_chan));
	chan_mc = (ich_chan *) malloc(sizeof(ich_chan));

	if (0 == chan_pi || 0 == chan_po || 0 == chan_mc) {
		PRINT(("couldn't allocate memory for channel descriptors!\n"));
		chan_free_resources();
		unmap_io_memory();
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;	
	}

	/* allocate memory for buffer descriptors */
	chan_po->bd_area = alloc_mem(&chan_po->bd_log_base, &chan_po->bd_phy_base, ICH_BD_COUNT * sizeof(ich_bd), "ich_ac97 po buf desc");
	chan_pi->bd_area = alloc_mem(&chan_pi->bd_log_base, &chan_pi->bd_phy_base, ICH_BD_COUNT * sizeof(ich_bd), "ich_ac97 pi buf desc");
	chan_mc->bd_area = alloc_mem(&chan_mc->bd_log_base, &chan_mc->bd_phy_base, ICH_BD_COUNT * sizeof(ich_bd), "ich_ac97 mc buf desc");
	/* allocate memory buffers */
	chan_po->buffer_area = alloc_mem(&chan_po->buffer_log_base, &chan_po->buffer_phy_base, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 po buf");
	chan_pi->buffer_area = alloc_mem(&chan_pi->buffer_log_base, &chan_pi->buffer_phy_base, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 pi buf");
	chan_mc->buffer_area = alloc_mem(&chan_mc->buffer_log_base, &chan_mc->buffer_phy_base, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 mc buf");
	/* allocate memory exported userland buffers */
	chan_po->userbuffer_area = alloc_mem(&chan_po->userbuffer_base, NULL, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 po user buf");
	chan_pi->userbuffer_area = alloc_mem(&chan_pi->userbuffer_base, NULL, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 pi user buf");
	chan_mc->userbuffer_area = alloc_mem(&chan_mc->userbuffer_base, NULL, BUFFER_COUNT * BUFFER_SIZE, "ich_ac97 mc user buf");

	if (  chan_po->bd_area < B_OK || chan_po->buffer_area < B_OK || chan_po->userbuffer_area < B_OK ||
		  chan_pi->bd_area < B_OK || chan_pi->buffer_area < B_OK || chan_pi->userbuffer_area < B_OK ||
		  chan_mc->bd_area < B_OK || chan_mc->buffer_area < B_OK || chan_mc->userbuffer_area < B_OK) {
		PRINT(("couldn't allocate memory for DMA buffers!\n"));
		chan_free_resources();
		unmap_io_memory();
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;	
	}

	/* allocate all semaphores */
	chan_po->buffer_ready_sem = create_sem(0,"po buffer ready");			/* pcm out buffer available */
	chan_pi->buffer_ready_sem = create_sem(0,"pi buffer ready");			/* 0 available pcm in buffers */
	chan_mc->buffer_ready_sem = create_sem(0,"mc buffer ready");			/* 0 available mic in buffers */
	
	if (chan_po->buffer_ready_sem < B_OK || chan_pi->buffer_ready_sem < B_OK || chan_mc->buffer_ready_sem < B_OK) {
		PRINT(("couldn't create semaphores!\n"));
		chan_free_resources();
		unmap_io_memory();
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;	
	}

	/* setup channel register base address */
	chan_pi->regbase = ICH_REG_PI_BASE;
	chan_po->regbase = ICH_REG_PO_BASE;
	chan_mc->regbase = ICH_REG_MC_BASE;

	/* reset all channels */
	reset_chan(chan_pi);
	reset_chan(chan_po);
	reset_chan(chan_mc);

	/* init channels */
	init_chan(chan_pi);
	init_chan(chan_po);
	init_chan(chan_mc);

	ac97_attach(&config->ac97, ac97_reg_read, ac97_reg_write, NULL);

	LOG(("codec vendor id      = %#08x\n", config->ac97->codec_id));
	LOG(("codec descripton     = %s\n", config->ac97->codec_info));
	LOG(("codec 3d enhancement = %s\n", config->ac97->codec_3d_stereo_enhancement));

	/* first test if interrupts are working, on some Laptops they don't work :-( */
	if (config->irq != 0 && false == interrupt_test()) {
		LOG(("interrupt not working, using a kernel thread for polling\n"));
		config->irq = 0; /* don't use interrupts */
	}

	/* install interrupt or polling thread */
	if (config->irq != 0) {
		install_io_interrupt_handler(config->irq, ich_int, 0, 0);
	} else {
		int_thread_id = spawn_kernel_thread(int_thread, "ich_ac97 interrupt poller", B_REAL_TIME_PRIORITY, 0);
		resume_thread(int_thread_id);
	}
	
	/* Only if the codec supports continuous sample rates,
	   try to calibrate the clock... */
	if (ac97_has_capability(config->ac97, CAP_PCM_RATE_CONTINUOUS)) {
#if DEBUG
		uint32 rate;
		if (ac97_get_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, &rate)) {
			LOG(("AC97_PCM_FRONT_DAC_RATE was %d\n", rate));
		} else {
			LOG(("couldn't get current AC97_PCM_FRONT_DAC_RATE rate\n"));
		}
		start_chan(chan_po);
		snooze(23000);
		LOG(("codec supports continuous sample rates, clock before calibration is %d\n", ich_clock_get()));
		stop_chan(chan_po);
#endif

		/* calibrate the clock */
		ich_clock_calibrate();

#if DEBUG
		start_chan(chan_po);
		snooze(23000);
		LOG(("codec supports continuous sample rates, clock after calibration is %d\n", ich_clock_get()));
		stop_chan(chan_po);

		if (ac97_get_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, &rate)) {
			LOG(("AC97_PCM_FRONT_DAC_RATE is now %d\n", rate));
		} else {
			LOG(("couldn't get current AC97_PCM_FRONT_DAC_RATE rate\n"));
		}
#endif
	} else {
#if DEBUG
		uint32 rate;
		start_chan(chan_po);
		snooze(23000);
		LOG(("codec doesn't support continuous sample rates, running at clock %d\n", ich_clock_get()));
		stop_chan(chan_po);
		if (ac97_get_rate(config->ac97, AC97_PCM_FRONT_DAC_RATE, &rate)) {
			LOG(("current AC97_PCM_FRONT_DAC_RATE is %d\n", rate));
		} else {
			LOG(("couldn't get current AC97_PCM_FRONT_DAC_RATE rate\n"));
		}
#endif
	}
	
	LOG(("init_driver finished!\n"));
	return B_OK;
}

void
uninit_driver(void)
{
	LOG(("uninit_driver()\n"));

	#if DEBUG
		if (chan_po) LOG(("chan_po frames_count = %Ld\n",chan_po->played_frames_count));
		if (chan_pi) LOG(("chan_pi frames_count = %Ld\n",chan_pi->played_frames_count));
		if (chan_mc) LOG(("chan_mc frames_count = %Ld\n",chan_mc->played_frames_count));
	#endif
	
	/* reset all channels */
	if (chan_pi)
		reset_chan(chan_pi);
	if (chan_po)
		reset_chan(chan_po);
	if (chan_mc)
		reset_chan(chan_mc);

	snooze(50000);
	
	ac97_detach(config->ac97);
	
	/* remove the interrupt handler or thread */
	if (config->irq != 0) {
		remove_io_interrupt_handler(config->irq,ich_int,0);
	} else if (int_thread_id != -1) {
		status_t exit_value;
		int_thread_exit = true;
		wait_for_thread(int_thread_id, &exit_value);
	}

	/* invalidate hardware buffer descriptor base addresses */
	ich_reg_write_32(ICH_REG_PI_BASE, 0);
	ich_reg_write_32(ICH_REG_PO_BASE, 0);
	ich_reg_write_32(ICH_REG_MC_BASE, 0);

	/* free allocated channel semaphores and memory */
	chan_free_resources();

	/* the very last thing to do is unmap the io memory */
	unmap_io_memory();

	put_module(B_PCI_MODULE_NAME);

	LOG(("uninit_driver() finished\n"));
}

int32 ich_test_int(void *check)
{
	/*
	 * This interrupt handler is used once to test if interrupt handling is working.
	 * If it gets executed and the interrupt is from pcm-out, it will set the bool
	 * pointed to by check to true.
	 */
	uint32 sta = ich_reg_read_32(ICH_REG_GLOB_STA);
	uint16 sr = ich_reg_read_16(chan_po->regbase + GET_REG_X_SR(config));

	if ((sta & STA_INTMASK) == 0)
		return B_UNHANDLED_INTERRUPT;

	if ((sta & STA_POINT) && (sr & SR_BCIS)) // pcm-out buffer completed
		*(bool *)check = true; // notify

	sr &= SR_FIFOE | SR_BCIS | SR_LVBCI;
	ich_reg_write_16(chan_po->regbase + GET_REG_X_SR(config), sr);

	return B_HANDLED_INTERRUPT;
}

int32 ich_int(void *unused)
{
	uint32 sta;
	sta = ich_reg_read_32(ICH_REG_GLOB_STA);
	
	if ((sta & STA_INTMASK) == 0)
		return B_UNHANDLED_INTERRUPT;
	
	if (sta & (STA_S0RI | STA_S1RI | STA_S2RI)) {
		/* ignore and clear resume interrupt(s) */
		ich_reg_write_32(ICH_REG_GLOB_STA, sta & (STA_S0RI | STA_S1RI | STA_S2RI));
	}
	
	if (sta & STA_POINT) { // pcm-out
		uint16 sr = ich_reg_read_16(chan_po->regbase + GET_REG_X_SR(config));
		sr &= SR_FIFOE | SR_BCIS | SR_LVBCI;

		if (sr & SR_BCIS) { // a buffer completed
			int32 count;
			int32 civ;

			/* shedule playback of the next buffers */		
			civ = ich_reg_read_8(chan_po->regbase + ICH_REG_X_CIV);
			ich_reg_write_8(chan_po->regbase + ICH_REG_X_LVI, (civ + 28) % ICH_BD_COUNT);

			/* calculate played buffer count since last interrupt */
			if (civ <= chan_po->lastindex)
				count = civ + ICH_BD_COUNT - chan_po->lastindex;
			else
				count = civ - chan_po->lastindex;
			
			if (count != 1)
				dprintf(DRIVER_NAME ": lost %ld po interrupts\n",count - 1);
			
			acquire_spinlock(&slock);
			chan_po->played_real_time = system_time();
			chan_po->played_frames_count += count * BUFFER_FRAMES_COUNT;
			chan_po->lastindex = civ;
			chan_po->backbuffer = chan_po->buffer[(civ + 1) % ICH_BD_COUNT];
			release_spinlock(&slock);

			get_sem_count(chan_po->buffer_ready_sem, &count);
			if (count <= 0)
				release_sem_etc(chan_po->buffer_ready_sem,1,B_DO_NOT_RESCHEDULE);
		}

		ich_reg_write_16(chan_po->regbase + GET_REG_X_SR(config), sr);
	}

	if (sta & STA_PIINT) { // pcm-in
		uint16 sr = ich_reg_read_16(chan_pi->regbase + GET_REG_X_SR(config));
		sr &= SR_FIFOE | SR_BCIS | SR_LVBCI;

		if (sr & SR_BCIS) { // a buffer completed
			int32 count;
			int32 civ;

			/* shedule record of the next buffers */		
			civ = ich_reg_read_8(chan_pi->regbase + ICH_REG_X_CIV);
			ich_reg_write_8(chan_pi->regbase + ICH_REG_X_LVI, (civ + 28) % ICH_BD_COUNT);

			/* calculate recorded buffer count since last interrupt */
			if (civ <= chan_pi->lastindex)
				count = civ + ICH_BD_COUNT - chan_pi->lastindex;
			else
				count = civ - chan_pi->lastindex;
			
			if (count != 1)
				dprintf(DRIVER_NAME ": lost %ld pi interrupts\n",count - 1);
			
			acquire_spinlock(&slock);
			chan_pi->played_real_time = system_time();
			chan_pi->played_frames_count += count * BUFFER_FRAMES_COUNT;
			chan_pi->lastindex = civ;
			chan_pi->backbuffer = chan_pi->buffer[(ICH_BD_COUNT - (civ - 1)) % ICH_BD_COUNT];
			release_spinlock(&slock);

			get_sem_count(chan_pi->buffer_ready_sem, &count);
			if (count <= 0)
				release_sem_etc(chan_pi->buffer_ready_sem,1,B_DO_NOT_RESCHEDULE);
		}

		ich_reg_write_16(chan_pi->regbase + GET_REG_X_SR(config), sr);
	}

	if (sta & STA_MINT) { // mic-in
		uint16 sr = ich_reg_read_16(chan_mc->regbase + GET_REG_X_SR(config));
		sr &= SR_FIFOE | SR_BCIS | SR_LVBCI;
		if (sr & SR_BCIS) { // a buffer completed
			release_sem_etc(chan_mc->buffer_ready_sem,1,B_DO_NOT_RESCHEDULE);
		}
		ich_reg_write_16(chan_mc->regbase + GET_REG_X_SR(config), sr);
	}
	return B_HANDLED_INTERRUPT;
}

static status_t
ich_open(const char *name, uint32 flags, void** cookie)
{
	LOG(("open()\n"));
	return B_OK;
}

static status_t
ich_close(void* cookie)
{
	LOG(("close()\n"));
	return B_OK;
}

static status_t
ich_free(void* cookie)
{
	return B_OK;
}

static status_t
ich_control(void* cookie, uint32 op, void* arg, size_t len)
{
	return multi_control(cookie,op,arg,len);
}

static status_t
ich_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was read */
	return B_IO_ERROR;
}

static status_t
ich_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	*num_bytes = 0;				/* tell caller nothing was written */
	return B_IO_ERROR;
}


/* -----
	null-terminated array of device names supported by this driver
----- */

static const char *ich_name[] = {
	"audio/multi/ich_ac97/1",
	NULL
};

/* -----
	function pointers for the device hooks entry points
----- */

device_hooks ich_hooks = {
	ich_open, 			/* -> open entry point */
	ich_close, 			/* -> close entry point */
	ich_free,			/* -> free cookie */
	ich_control, 		/* -> control entry point */
	ich_read,			/* -> read entry point */
	ich_write,			/* -> write entry point */
	NULL,				/* start select */
	NULL,				/* stop select */
	NULL,				/* scatter-gather read from the device */
	NULL				/* scatter-gather write to the device */
};

/* ----------
	publish_devices - return a null-terminated array of devices
	supported by this driver.
----- */

const char**
publish_devices(void)
{
	return ich_name;
}

/* ----------
	find_device - return ptr to device hooks structure for a
	given device name
----- */

device_hooks*
find_device(const char* name)
{
	return &ich_hooks;
}
