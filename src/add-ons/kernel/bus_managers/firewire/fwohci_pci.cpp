/*-
 * Copyright (c) 2003 Hidetoshi Shimokawa
 * Copyright (c) 1998-2002 Katsushi Kobayashi and Hidetoshi Shimokawa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the acknowledgement as bellow:
 *
 *    This product includes software developed by K. Kobayashi and H. SHimokawa
 *
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * $FreeBSD: src/sys/dev/firewire/fwohci_pci.c,v 1.60 2007/06/06 14:31:36 simokawa Exp $
 */

#include <OS.h>
#include <KernelExport.h>
#include <lock.h>
#include <SupportDefs.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "util.h"
#include "fwdebug.h"
#include "fwglue.h"
#include "queue.h"
#include "firewire.h"
#include "iec13213.h"
#include "firewirereg.h"

#include "fwdma.h"
#include "fwohcireg.h"
#include "fwohcivar.h"

#define PCIM_CMD_IOS		0x0001 
#define	PCIM_CMD_MEMEN		0x0002
#define	PCIM_CMD_BUSMASTEREN	0x0004
#define	PCIM_CMD_MWRICEN	0x0010
#define	PCIM_CMD_PERRESPEN	0x0040
#define	PCIM_CMD_SERRESPEN	0x0100

extern pci_module_info	*gPci;
extern pci_info *pciInfo[MAX_CARDS];
extern fwohci_softc_t *gFwohci_softc[MAX_CARDS];
extern struct firewire_softc *gFirewire_softc[MAX_CARDS];

status_t
fwohci_pci_detach(int index)
{
	fwohci_softc_t *sc = gFwohci_softc[index];
	int s;
	
	s = splfw();

	fwohci_stop(sc);

//	bus_generic_detach(self);
	firewire_detach(gFirewire_softc[index]);
/*	if (sc->fc.bdev) {
		device_delete_child(self, sc->fc.bdev);
		sc->fc.bdev = NULL;
	}*/

	/* disable interrupts that might have been switched on */
	OWRITE(sc, FWOHCI_INTMASKCLR, OHCI_INT_EN);
	remove_io_interrupt_handler (sc->irq, fwohci_intr, sc);
	delete_area(sc->regArea);
	
	fwohci_detach(sc);
	mtx_destroy(FW_GMTX(&sc->fc));
	splx(s);
	return B_OK;
}

static void
fwohci_pci_add_child(int index)
{
	struct fwohci_softc *sc;
	int err = 0;

	sc = gFwohci_softc[index];
	
/*	child = device_add_child(dev, name, unit);
	if (child == NULL)
		return (child);

	sc->fc.bdev = child;
	device_set_ivars(child, (void *)&sc->fc);*/

//	err = device_probe_and_attach(child);
	err = firewire_attach(&sc->fc, gFirewire_softc[index]);

	if (err) {
		device_printf(dev, "firewire_attach failed with err=%d\n",
		    err);
		fwohci_pci_detach(index);
//		device_delete_child(dev, child);
		return;
	}
	/* XXX
	 * Clear the bus reset event flag to start transactions even when
	 * interrupt is disabled during the boot process.
	 */
//	if (cold) {
//		int s;
//		DELAY(250); /* 2 cycles */
//		s = splfw();
//		fwohci_poll((void *)sc, 0, -1);
//		splx(s);
//	}

}

status_t
fwohci_pci_attach(int index)
{
	fwohci_softc_t *sc = gFwohci_softc[index];
	pci_info *info = pciInfo[index];
	uint32 olatency, latency, ocache_line, cache_line;
	uint32 val;

	mtx_init(FW_GMTX(&sc->fc), "firewire", NULL, MTX_DEF);

	val = gPci->read_pci_config(info->bus, info->device, info->function, 
			PCI_command, 2);
	val |= PCIM_CMD_MEMEN | PCIM_CMD_BUSMASTEREN | PCIM_CMD_MWRICEN;

#if 1  /* for broken hardware */
	val &= ~PCIM_CMD_MWRICEN; 
	val &= ~(PCIM_CMD_SERRESPEN | PCIM_CMD_PERRESPEN);
#endif
	gPci->write_pci_config(info->bus, info->device, info->function, 
			PCI_command, 2, val);

	/*
	 * Some Sun PCIO-2 FireWire controllers have their intpin register
	 * bogusly set to 0, although it should be 3. Correct that.
	 */
	if (info->vendor_id == FW_VENDORID_SUN && info->device_id == (FW_DEVICE_PCIO2FW >> 16) && 
			info->u.h0.interrupt_pin == 0)
		info->u.h0.interrupt_pin = 3;

	latency = olatency = gPci->read_pci_config(info->bus, info->device, info->function, 
			PCI_latency, 1);
#define DEF_LATENCY 0x20
	if (olatency < DEF_LATENCY) {
		latency = DEF_LATENCY;
		gPci->write_pci_config(info->bus, info->device, info->function, 
				PCI_latency, 1, latency);
	}

	cache_line = ocache_line = gPci->read_pci_config(info->bus, info->device, 
			info->function, PCI_line_size, 1);
#define DEF_CACHE_LINE 8
	if (ocache_line < DEF_CACHE_LINE) {
		cache_line = DEF_CACHE_LINE;
		gPci->write_pci_config(info->bus, info->device, info->function, 
				PCI_line_size, 1, cache_line);
	}
	TRACE("latency timer %lx -> %lx.\n", olatency, latency);
	TRACE("cache size %lx -> %lx.\n", ocache_line, cache_line);

	// get IRQ
	sc->irq = gPci->read_pci_config(info->bus, info->device, info->function, 
			PCI_interrupt_line, 1);
	if (sc->irq == 0 || sc->irq == 0xff) {
		ERROR("no IRQ assigned\n");
		goto err;
	}
	TRACE("IRQ %d\n", sc->irq);

	// map registers into memory
//	val = gPci->read_pci_config(info->bus, info->device, info->function, 0x14, 4);
//	val &= PCI_address_memory_32_mask;
//	TRACE("hardware register address %p\n", (void *) val);
	TRACE("hardware register address %lx\n", info->u.h0.base_registers[0]);
	sc->regArea = map_mem(&sc->regAddr, (void *)info->u.h0.base_registers[0], 0x800, 
			B_READ_AREA | B_WRITE_AREA, "fw ohci register");
	if (sc->regArea < B_OK) {
		ERROR("can't map hardware registers\n");
		goto err;
	}	
	TRACE("mapped registers to %p\n", sc->regAddr);

	// setup interrupt handler
	if (install_io_interrupt_handler(sc->irq, fwohci_intr, 
				sc, 0) < B_OK) {
		ERROR("can't install interrupt handler\n");
		goto err;
	}	

	if (fwohci_init(sc) < B_OK){

		ERROR("fwohci_init failed");
		goto err;
	}
	fwohci_pci_add_child(index);
	return B_OK;
err:
	delete_area(sc->regArea);
	mtx_destroy(FW_GMTX(&sc->fc));
	return B_ERROR;
}
