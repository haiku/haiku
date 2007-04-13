/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "cx23882.h"
#include "util.h"
#include <KernelExport.h>
#include <ByteOrder.h>
#include <Drivers.h>


#define TRACE_CX23882
#ifdef TRACE_CX23882
  #define TRACE dprintf
#else
  #define TRACE(a...)
#endif

// settings for hardware stream sync
#define MPEG2_SYNC_BYTE		0x47
#define MPEG2_PACKET_SIZE	188
#define SYNC_PACKET_COUNT	7		// 0 and 5 don't seem to work

// Line size is also used as FIFO size!
// BYTES_PER_LINE must be a multiple of 8 and <= 4096 bytes
#define PACKETS_PER_LINE	20
#define BYTES_PER_LINE		(PACKETS_PER_LINE * MPEG2_PACKET_SIZE)

#define SRAM_START_ADDRESS	0x180000
#define SRAM_BASE_CMDS_TS	0x200
#define SRAM_BASE_RISC_PROG	0x400
#define SRAM_BASE_RISC_QUEUE 0x800
#define SRAM_BASE_CDT		0x900
#define SRAM_BASE_FIFO_0	0x1000
#define SRAM_BASE_FIFO_1	0x2000

// About 64 kByte DMA buffer size
#define LINES_PER_BUFFER	16
#define DMA_BUFFER_SIZE		(LINES_PER_BUFFER * BYTES_PER_LINE)


static status_t	cx23882_buffers_alloc(cx23882_device *device);
static void		cx23882_buffers_free(cx23882_device *device);
static void		cx23882_risc_ram_setup(cx23882_device *device);
static void		cx23882_sram_setup(cx23882_device *device);
static void		cx23882_via_sis_fixup(cx23882_device *device);


void
cx23882_reset(cx23882_device *device)
{
	// software reset (XXX Test)
	reg_write32(0x38c06c, 1);
	snooze(200000);
	
	// disable RISC controller
	reg_write32(REG_DEV_CNTRL2, 0);

	// disable TS interface DMA
	reg_write32(REG_TS_DMA_CNTRL, 0x0);
	
	// disable VIP interface up- & downstram DMA
	reg_write32(REG_VIP_STREAM_EN, 0x0);
	
	// disable host interface up- & downstram DMA
	reg_write32(REG_HST_STREAM_EN, 0x0);

	// stop all interrupts
	reg_write32(REG_PCI_INT_MSK, 0x0);
	reg_write32(REG_VID_INT_MSK, 0x0);
	reg_write32(REG_AUD_INT_MSK, 0x0);
	reg_write32(REG_TS_INT_MSK,  0x0);
	reg_write32(REG_VIP_INT_MSK, 0x0);
	reg_write32(REG_HST_INT_MSK, 0x0);
	reg_write32(REG_DMA_RISC_INT_MSK, 0x0);

	// clear all pending interrupts
	reg_write32(REG_PCI_INT_STAT, 0xffffffff);
	reg_write32(REG_VID_INT_STAT, 0xffffffff);
	reg_write32(REG_AUD_INT_STAT, 0xffffffff);
	reg_write32(REG_TS_INT_STAT,  0xffffffff);
	reg_write32(REG_VIP_INT_STAT, 0xffffffff);
	reg_write32(REG_HST_INT_STAT, 0xffffffff);
	reg_write32(REG_DMA_RISC_INT_MSK, 0xffffffff);
}	


status_t
cx23882_init(cx23882_device *device)
{
	// assumes that cx23882_reset() has already been called
	
	status_t err;
	
	if ((err = cx23882_buffers_alloc(device)) < B_OK) {
		dprintf("cx23882: Error, buffer alloc failed\n");
		return err;
	}
	
	device->capture_size = DMA_BUFFER_SIZE;
	
	cx23882_via_sis_fixup(device);

	// Set FIFO thresholds, should be 0 < x <= 7
	reg_write32(REG_PDMA_STHRSH, PDMA_ISBTHRSH_6 | PDMA_PCITHRSH_6);
	reg_write32(REG_PDMA_DTHRSH, PDMA_ISBTHRSH_6 | PDMA_PCITHRSH_6);

	// init risc programm
	cx23882_risc_ram_setup(device);

	// init sram
	cx23882_sram_setup(device);

	// Reset counter to 0
	reg_write32(REG_TS_GP_CNT_CNTRL, 0x3);

	// Line length for RISC DMA
	reg_write32(REG_TS_LNGTH, BYTES_PER_LINE);
	
	// Set serial interface mode
	reg_write32(REG_TS_GEN_CONTROL, reg_read32(REG_TS_GEN_CONTROL) | TS_GEN_CONTROL_IPB_SMODE);
	
	// Setup hardware MPEG2 fec interface
	reg_write32(REG_HW_SOP_CONTROL, (MPEG2_SYNC_BYTE << 16) | (MPEG2_PACKET_SIZE << 4) | SYNC_PACKET_COUNT);

	// Setup TSSTOP status, active low, rising and falling edge, single bit width
	reg_write32(REG_TS_SOP_STATUS, reg_read32(REG_TS_SOP_STATUS) |  0x18000);
	reg_write32(REG_TS_SOP_STATUS, reg_read32(REG_TS_SOP_STATUS) & ~0x06000);

	// Enable interrupts for MPEG TS and all errors
	reg_write32(REG_PCI_INT_MSK, reg_read32(REG_PCI_INT_MSK) | PCI_INT_STAT_TS_INT | 0x00fc00);
	reg_write32(REG_TS_INT_MSK, reg_read32(REG_TS_INT_MSK) | TS_INT_STAT_TS_RISC1 | TS_INT_STAT_TS_RISC2 | 0x1f1100);
	
	TRACE("cx23882_init done\n");
	return B_OK;
}


status_t
cx23882_terminate(cx23882_device *device)
{
	cx23882_reset(device);
	
	cx23882_buffers_free(device);
	return B_OK;
}


status_t
cx23882_start_capture(cx23882_device *device)
{
	TRACE("cx23882_start_capture\n");

	// start RISC processor and DMA
	reg_write32(REG_DEV_CNTRL2, reg_read32(REG_DEV_CNTRL2) | DEV_CNTRL2_RUN_RISC);
	reg_write32(REG_TS_DMA_CNTRL, reg_read32(REG_TS_DMA_CNTRL) | TS_DMA_CNTRL_TS_FIFO_EN | TS_DMA_CNTRL_TS_RISC_EN);
	return B_OK;
}


status_t
cx23882_stop_capture(cx23882_device *device)
{
	TRACE("cx23882_stop_capture\n");

	// stop RISC processor and DMA
	reg_write32(REG_TS_DMA_CNTRL, reg_read32(REG_TS_DMA_CNTRL) & ~(TS_DMA_CNTRL_TS_FIFO_EN | TS_DMA_CNTRL_TS_RISC_EN));
	reg_write32(REG_DEV_CNTRL2, reg_read32(REG_DEV_CNTRL2) & ~DEV_CNTRL2_RUN_RISC);
	return B_OK;
}


static inline void
cx23882_mpegts_int(cx23882_device *device)
{
	uint32 mstat = reg_read32(REG_TS_INT_MSTAT);
	reg_write32(REG_TS_INT_STAT, mstat);
	
//	dprintf("cx23882_mpegts_int got 0x%08lx\n", mstat);

	if (mstat & TS_INT_STAT_OPC_ERR) {
		dprintf("cx23882_mpegts_int RISC opcode error\n");
		reg_write32(REG_PCI_INT_MSK, 0);
		return;
	}

	if ((mstat & (TS_INT_STAT_TS_RISC1 | TS_INT_STAT_TS_RISC2)) == (TS_INT_STAT_TS_RISC1 | TS_INT_STAT_TS_RISC2)) {
		dprintf("cx23882_mpegts_int both buffers ready\n");
		mstat = TS_INT_STAT_TS_RISC1;
	}
	
	if (mstat & TS_INT_STAT_TS_RISC1) {
		int32 count;
//		dprintf("cx23882_mpegts_int buffer 1 at %Ld\n", system_time());
		device->capture_data = device->dma_buf1_virt;
		device->capture_end_time = system_time();
		get_sem_count(device->capture_sem, &count);
		if (count <= 0)
			release_sem_etc(device->capture_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	if (mstat & TS_INT_STAT_TS_RISC2) {
		int32 count;
//		dprintf("cx23882_mpegts_int buffer 2 at %Ld\n", system_time());
		device->capture_data = device->dma_buf2_virt;
		device->capture_end_time = system_time();
		get_sem_count(device->capture_sem, &count);
		if (count <= 0)
			release_sem_etc(device->capture_sem, 1, B_DO_NOT_RESCHEDULE);
	}
}


int32
cx23882_int(void *data)
{
	cx23882_device *device = data;
	uint32 mstat;
	uint32 wmstat;
	
	mstat = reg_read32(REG_PCI_INT_MSTAT);
	if (!mstat)
		return B_UNHANDLED_INTERRUPT;

	if (mstat & (PCI_INT_STAT_HST_INT | PCI_INT_STAT_VIP_INT | PCI_INT_STAT_AUD_INT | PCI_INT_STAT_VID_INT)) {
		// serious error, these bits should not be set
		dprintf("cx23882_int error: msk 0x%08lx, stat 0x%08lx, mstat 0x%08lx\n", reg_read32(REG_PCI_INT_MSK), reg_read32(REG_PCI_INT_STAT), mstat);
		reg_write32(REG_PCI_INT_MSK, 0);
		return B_HANDLED_INTERRUPT;
	}

	wmstat = mstat & ~(PCI_INT_STAT_HST_INT | PCI_INT_STAT_VIP_INT | PCI_INT_STAT_TS_INT | PCI_INT_STAT_AUD_INT | PCI_INT_STAT_VID_INT);
	if (wmstat)
		reg_write32(REG_PCI_INT_STAT, wmstat);

	if (wmstat)
		dprintf("cx23882_int got 0x%08lx\n", wmstat);

	if (mstat & PCI_INT_STAT_TS_INT) {
		cx23882_mpegts_int(device);
		return B_INVOKE_SCHEDULER;
	} else {
		return B_HANDLED_INTERRUPT;
	}
}


static status_t
cx23882_buffers_alloc(cx23882_device *device)
{
	device->dma_buf1_area = alloc_mem(&device->dma_buf1_virt, &device->dma_buf1_phys, DMA_BUFFER_SIZE, B_READ_AREA, "cx23882 dma buf 1");
	device->dma_buf2_area = alloc_mem(&device->dma_buf2_virt, &device->dma_buf2_phys, DMA_BUFFER_SIZE, B_READ_AREA, "cx23882 dma buf 2");
	if (device->dma_buf1_area < B_OK || device->dma_buf2_area < B_OK) {
		cx23882_buffers_free(device);
		return B_NO_MEMORY;
	}
	return B_OK;
}


static void
cx23882_buffers_free(cx23882_device *device)
{
	if (device->dma_buf1_area >= 0)
		delete_area(device->dma_buf1_area);
	if (device->dma_buf2_area >= 0)
		delete_area(device->dma_buf2_area);
	device->dma_buf1_area = -1;
	device->dma_buf2_area = -1;
}


static void
cx23882_sram_setup(cx23882_device *device)
{
	dprintf("cx23882_sram_setup enter\n");

	// setup CDT entries for both FIFOs
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CDT, SRAM_START_ADDRESS + SRAM_BASE_FIFO_0);
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CDT + 16, SRAM_START_ADDRESS + SRAM_BASE_FIFO_1);

	// setup CDMS	
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CMDS_TS + 0x00, SRAM_START_ADDRESS + SRAM_BASE_RISC_PROG);
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CMDS_TS + 0x04, SRAM_START_ADDRESS + SRAM_BASE_CDT);
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CMDS_TS + 0x08, (2 * 16) / 8); // FIFO count = 2
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CMDS_TS + 0x0c, SRAM_START_ADDRESS + SRAM_BASE_RISC_QUEUE);
	reg_write32(SRAM_START_ADDRESS + SRAM_BASE_CMDS_TS + 0x10, 0x80000000 | (0x100 / 4));

	// setup DMA registers
	reg_write32(REG_DMA28_PTR1, SRAM_START_ADDRESS + SRAM_BASE_FIFO_0);
	reg_write32(REG_DMA28_PTR2, SRAM_START_ADDRESS + SRAM_BASE_CDT);
	reg_write32(REG_DMA28_CNT1, BYTES_PER_LINE / 8);
	reg_write32(REG_DMA28_CNT2, (2 * 16) / 8); // FIFO count = 2

	dprintf("cx23882_sram_setup leave\n");
}


static void
cx23882_risc_ram_setup(cx23882_device *device)
{
	char *start = (char *)(device->regs) + SRAM_START_ADDRESS + SRAM_BASE_RISC_PROG;
	volatile uint32 *rp = (volatile uint32 *)start;
	int i;
	
	#define set_opcode(a) (*rp++) = B_HOST_TO_LENDIAN_INT32((a))

	dprintf("cx23882_risc_ram_setup enter\n");
	
	// sync
	set_opcode(RISC_RESYNC | 0);
	
	// copy buffer 1
	for (i = 0; i < LINES_PER_BUFFER; i++) {
		set_opcode(RISC_WRITE | RISC_SOL | RISC_EOL | BYTES_PER_LINE);
		set_opcode((unsigned long)device->dma_buf1_phys + i * BYTES_PER_LINE);
	}
	
	// execute IRQ 1
	set_opcode(RISC_SKIP | RISC_IRQ1 | RISC_SOL | 0);

	// copy buffer 2
	for (i = 0; i < LINES_PER_BUFFER; i++) {
		set_opcode(RISC_WRITE | RISC_SOL | RISC_EOL | BYTES_PER_LINE);
		set_opcode((unsigned long)device->dma_buf2_phys + i * BYTES_PER_LINE);
	}

	// execute IRQ 2
	set_opcode(RISC_SKIP | RISC_IRQ2 | RISC_SOL | 0);

	// jmp to start, but skip sync instruction
	set_opcode(RISC_JUMP | RISC_SRP);
	set_opcode(SRAM_START_ADDRESS + SRAM_BASE_RISC_PROG + 4);
	
	#undef set_opcode

	dprintf("cx23882_risc_ram_setup leave\n");
}


static void
cx23882_via_sis_fixup(cx23882_device *device)
{
	uint16 host_vendor;
	uint32 dev_cntrl1;
	
	host_vendor = gPci->read_pci_config(0, 0, 0, PCI_vendor_id, 2);
	dev_cntrl1 = reg_read32(REG_F2_DEV_CNTRL1);
	
	if (host_vendor == PCI_VENDOR_VIA || host_vendor == PCI_VENDOR_SIS) {
		dprintf("cx23882: enabling VIA/SIS compatibility mode\n");
		reg_write32(REG_F2_DEV_CNTRL1, dev_cntrl1 | F2_DEV_CNTRL1_EN_VSFX);
	} else {
		dprintf("cx23882: disabling VIA/SIS compatibility mode\n");
		reg_write32(REG_F2_DEV_CNTRL1, dev_cntrl1 & ~F2_DEV_CNTRL1_EN_VSFX);
	}
}
