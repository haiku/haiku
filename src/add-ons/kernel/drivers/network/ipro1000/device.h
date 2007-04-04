/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef __DEVICE_H
#define __DEVICE_H

#include <PCI.h>
#include "setup.h"

extern pci_module_info *gPci;

status_t ipro1000_open(const char *name, uint32 flags, void** cookie);
status_t ipro1000_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
status_t ipro1000_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);
status_t ipro1000_control(void *cookie, uint32 op, void *arg, size_t len);
status_t ipro1000_close(void* cookie);
status_t ipro1000_free(void* cookie);

struct adapter;

typedef struct device {
	int 				devId;
	pci_info *			pciInfo;
	
	struct adapter *	adapter;
	uint8				pciBus;
	uint8				pciDev;
	uint8				pciFunc;
	void *				regAddr;
	
	volatile bool		nonblocking;
	volatile bool		closed;

	uint32				maxframesize;	// 14 bytes header + MTU
	uint8				macaddr[6];

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	sem_id				linkChangeSem;
#endif
} ipro1000_device;

#endif
