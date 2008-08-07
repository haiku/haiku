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
#include "if_em_osdep.h"
#include "debug.h"
#include "util.h"

#undef malloc
#undef free


void *
driver_malloc(int size, int p2, int p3)
{
	return malloc(size);
}


void
driver_free(void *p, int p2)
{
	free(p);
}


void *
contigmalloc(int size, int p1, int p2, int p3, int p4, int p5, int p6)
{
	void *adr;
	if (create_area("contigmalloc", &adr, B_ANY_KERNEL_ADDRESS, size,
			B_CONTIGUOUS, 0) < B_OK)
		return NULL;
	return adr;
}


void
contigfree(void *p, int p1, int p2)
{
	delete_area(area_for(p));
}


void
callout_handle_init(struct callout_handle *handle)
{
	handle->timer = -1;
}


struct callout_handle
timeout(timer_function func, void *cookie, bigtime_t timeout)
{
	struct callout_handle handle;
	handle.timer = create_timer(func, cookie, timeout, B_ONE_SHOT_RELATIVE_TIMER);
	return handle;
}


void
untimeout(timer_function func, void *cookie, struct callout_handle handle)
{
	delete_timer(handle.timer);
}


struct resource *
bus_alloc_resource(device_t dev, int type, int *rid, int d, int e, int f, int g)
{
	switch (type) {
		case SYS_RES_IOPORT:
		{
			uint32 v = pci_read_config(dev, *rid, 4) & PCI_address_io_mask;
			INIT_DEBUGOUT2("bus_alloc_resource SYS_RES_IOPORT, reg 0x%x, adr %p\n", *rid, (void *)v);
			return (struct resource *) v;
		}

		case SYS_RES_MEMORY:
		{
			uint32 v = pci_read_config(dev, *rid, 4) & PCI_address_memory_32_mask;
			uint32 size = 128 * 1024; // XXX get size from BAR
			void *virt;
			INIT_DEBUGOUT2("bus_alloc_resource SYS_RES_MEMORY, reg 0x%x, adr %p\n", *rid, (void *)v);
			if (map_mem(&virt, (void *)v, size, 0, "SYS_RES_MEMORY") < 0)
				return 0;
			return (struct resource *) virt;
		}

		case SYS_RES_IRQ:
		{
			uint8 v = pci_read_config(dev, PCI_interrupt_line, 1);
			if (v == 0 || v == 0xff) {
				ERROROUT("bus_alloc_resource SYS_RES_IRQ: no irq");
				return 0;
			}
			return (struct resource *)(int)v;
		}

		default:
			INIT_DEBUGOUT("bus_alloc_resource default!\n");
			return 0;
	}
}


void
bus_release_resource(device_t dev, int type, int reg, struct resource *res)
{
	switch (type) {
		case SYS_RES_IOPORT:
		case SYS_RES_IRQ:
			return;

		case SYS_RES_MEMORY:
			delete_area(area_for(res));
			return;

		default:
			INIT_DEBUGOUT("bus_release_resource default!\n");
			return;
	}
}


uint32
rman_get_start(struct resource *res)
{
	return (uint32)res;
}


struct int_tag {
	interrupt_handler int_func;
	void *cookie;
	int irq;
};


int
bus_setup_intr(device_t dev, struct resource *res, int p3, interrupt_handler int_func, void *cookie, void **tag)
{
	int irq = (int)res;

	struct int_tag *int_tag = (struct int_tag *) malloc(sizeof(struct int_tag));
	int_tag->int_func = int_func;
	int_tag->cookie = cookie;
	int_tag->irq = irq;
	*tag = int_tag;

	return install_io_interrupt_handler(irq, int_func, cookie, 0);
}


void
bus_teardown_intr(device_t dev, struct resource *res, void *tag)
{
	struct int_tag *int_tag = (struct int_tag *) tag;
	remove_io_interrupt_handler(int_tag->irq, int_tag->int_func, int_tag->cookie);
	free(int_tag);
}
