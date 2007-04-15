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

#ifndef __CX23882_H
#define __CX23882_H

#include <SupportDefs.h>
#include "driver.h"
#include "cx23882_regs.h"
#include "i2c_core.h"

typedef struct {
	const pci_info *	pci_info;
	int					irq;
	void *				regs;
	area_id				regs_area;
	uint32				i2c_reg;
	i2c_bus *			i2c_bus;
	
	area_id				dma_buf1_area;
	void *				dma_buf1_virt;
	void *				dma_buf1_phys;
	area_id				dma_buf2_area;
	void *				dma_buf2_virt;
	void *				dma_buf2_phys;
	
	sem_id				capture_sem;
	void *				capture_data;
	size_t				capture_size;
	bigtime_t			capture_end_time;
} cx23882_device;


#define reg_read8(offset)			(*(volatile uint8 *) ((char *)(device->regs) + (offset)))
#define reg_read16(offset)			(*(volatile uint16 *)((char *)(device->regs) + (offset)))
#define reg_read32(offset)			(*(volatile uint32 *)((char *)(device->regs) + (offset)))
#define reg_write8(offset, value)	(*(volatile uint8 *) ((char *)(device->regs) + (offset)) = value)
#define reg_write16(offset, value)	(*(volatile uint16 *)((char *)(device->regs) + (offset)) = value)
#define reg_write32(offset, value)	(*(volatile uint32 *)((char *)(device->regs) + (offset)) = value)


void	 cx23882_reset(cx23882_device *device);
status_t cx23882_init(cx23882_device *device);
status_t cx23882_terminate(cx23882_device *device);

status_t cx23882_start_capture(cx23882_device *device);
status_t cx23882_stop_capture(cx23882_device *device);

int32	 cx23882_int(void *data);

#endif
