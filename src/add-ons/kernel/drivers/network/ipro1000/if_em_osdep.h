/**************************************************************************

Copyright (c) 2001-2003, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of the Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _IF_EM_OSDEP_H_
#define _IF_EM_OSDEP_H_

#include "driver.h"
#include "device.h"
#include "timer.h"
#include "debug.h"

#include <OS.h>
#include <KernelExport.h>
#include <ByteOrder.h>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#define usec_delay(x) snooze(x)
#define msec_delay(x) snooze(1000*(x))

// no longer used in FreeBSD
#define splx(s)
#define splimp() 0

#ifndef bzero
  #define bzero(d,c)		memset((d), (0), (c))
#endif
#ifndef bcopy
  #define bcopy(s,d,c)		memmove((d), (s), (c))
#endif
#ifndef bcmp
  #define bcmp(p1, p2, s)	memcmp((p1), (p2), (s)) // XXX correct order?
#endif

#define TUNABLE_INT(a, b)	/* ignored */

#define FALSE               0
#define TRUE                1

#define hz					1000000LL

typedef bool	boolean_t;
typedef unsigned long vm_offset_t;


struct em_osdep
{
	ipro1000_device *dev;
};

typedef ipro1000_device * device_t;

#define PCIR_COMMAND			PCI_command
#define PCIR_REVID				PCI_revision
#define PCIR_SUBVEND_0			PCI_subsystem_vendor_id
#define PCIR_SUBDEV_0			PCI_subsystem_id
#define PCIM_CMD_IOEN			0x0001
#define PCIM_CMD_MEMEN			0x0002
#define PCIM_CMD_BUSMASTEREN	0x0004
#define CMD_MEM_WRT_INVALIDATE          0x0010  /* BIT_4 */

#define pci_read_config(dev, offset, size) \
	gPci->read_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc, offset, size)

#define pci_write_config(dev, offset, value, size) \
	gPci->write_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc, offset, size, value)

#define pci_get_vendor(dev) \
	gPci->read_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc, PCI_vendor_id, 2)

#define pci_get_device(dev) \
	gPci->read_pci_config(dev->pciBus, dev->pciDev, dev->pciFunc, PCI_device_id, 2)

#define inl(port) \
	gPci->read_io_32(port)

#define outl(port, value) \
	gPci->write_io_32(port, value)


void *contigmalloc(int size, int p1, int p2, int p3, int p4, int p5, int p6);
void contigfree(void *p, int p1, int p2);

static inline u_int64_t vtophys(unsigned long virtual_addr)
{
	physical_entry pe;
	status_t err;
	// vtophys is called for the start of any page aligned contiguous large buffer,
	// and also with offset 2 into 2048 byte aligned (non-contiguous) rx/tx buffer.
	// Thus we only ask for 2046 bytes to get exactly one physical_entry. If the
	// next physical page is non-contiguous, using 2048 may return B_BUFFER_OVERFLOW.
	err = get_memory_map((void *)virtual_addr, 2046, &pe, 1);
	if (err < 0)
		panic("ipro1000: get_memory_map failed for %p, error %08lx\n", (void *)virtual_addr, err);
	return pe.address;
}

#define M_DEVBUF 1
#define M_NOWAIT 2
#define PAGE_SIZE 4096


struct callout_handle
{
	timer_id timer;
};

void callout_handle_init(struct callout_handle *handle);
struct callout_handle timeout(timer_function func, void *cookie, bigtime_t timeout);
void untimeout(timer_function func, void *cookie, struct callout_handle handle);


// resource management
struct resource * bus_alloc_resource(device_t dev, int type, int *rid, int d, int e, int f, int g);
void bus_release_resource(device_t dev, int type, int reg, struct resource *res);
uint32 rman_get_start(struct resource *res);

#define	bus_generic_detach(dev)

#define SYS_RES_IOPORT	0x01
#define SYS_RES_MEMORY	0x02
#define SYS_RES_IRQ		0x04
#define RF_SHAREABLE 0
#define RF_ACTIVE 0


#define INTR_TYPE_NET 0
int bus_setup_intr(device_t dev, struct resource *res, int p3, interrupt_handler int_func, void *cookie, void **tag);
void bus_teardown_intr(device_t dev, struct resource *res, void *tag);


#undef malloc
#define malloc driver_malloc
#undef free
#define free driver_free

void *driver_malloc(int size, int p2, int p3);
void  driver_free(void *p, int p2);

/* GCC will always emit a read opcode when reading from */
/* a volatile pointer, even if the result is unused */
#define E1000_WRITE_FLUSH(a) \
	E1000_READ_REG(a, STATUS)

/* Read from an absolute offset in the adapter's memory space */
#define E1000_READ_OFFSET(hw, offset) \
	(*(volatile uint32 *)((char *)(((struct em_osdep *)(hw)->back)->dev->regAddr) + offset))

/* Write to an absolute offset in the adapter's memory space */
#define E1000_WRITE_OFFSET(hw, offset, value) \
	(*(volatile uint32 *)((char *)(((struct em_osdep *)(hw)->back)->dev->regAddr) + offset) = value)

/* Convert a register name to its offset in the adapter's memory space */
#define E1000_REG_OFFSET(hw, reg) \
    ((hw)->mac_type >= em_82543 ? E1000_##reg : E1000_82542_##reg)

#define E1000_READ_REG(hw, reg) \
    E1000_READ_OFFSET(hw, E1000_REG_OFFSET(hw, reg))

#define E1000_WRITE_REG(hw, reg, value) \
    E1000_WRITE_OFFSET(hw, E1000_REG_OFFSET(hw, reg), value)

#define E1000_READ_REG_ARRAY(hw, reg, index) \
    E1000_READ_OFFSET(hw, E1000_REG_OFFSET(hw, reg) + ((index) << 2))

#define E1000_WRITE_REG_ARRAY(hw, reg, index, value) \
    E1000_WRITE_OFFSET(hw, E1000_REG_OFFSET(hw, reg) + ((index) << 2), value)

#define TAILQ_HEAD(name, type) \
	struct name { struct type *tqh_first, **tqh_last; }

#define TAILQ_ENTRY(type) \
	struct { struct type *tqe_next, **tqe_prev; }

#define TAILQ_FIRST(head) \
	((head)->tqh_first)

#define TAILQ_NEXT(elm, field) \
	((elm)->field.tqe_next)

#define TAILQ_FOREACH(var, head, field) \
	for ((var) = TAILQ_FIRST((head)); (var); (var) = TAILQ_NEXT((var), field))

#define TAILQ_FOREACH_SAFE(var, head, field, tvar) \
	for ((var) = TAILQ_FIRST((head)); \
		(var) && ((tvar) = TAILQ_NEXT((var), field), 1); (var) = (tvar))

#define TAILQ_INIT(head) do { \
	TAILQ_FIRST((head)) = NULL; \
	(head)->tqh_last = &TAILQ_FIRST((head)); \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do { \
	if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL) \
		TAILQ_FIRST((head))->field.tqe_prev = &TAILQ_NEXT((elm), field); \
	else \
		(head)->tqh_last = &TAILQ_NEXT((elm), field); \
	TAILQ_FIRST((head)) = (elm); \
	(elm)->field.tqe_prev = &TAILQ_FIRST((head)); \
} while (0)

#define TAILQ_REMOVE(head, elm, field) do { \
	if ((TAILQ_NEXT((elm), field)) != NULL) \
		TAILQ_NEXT((elm), field)->field.tqe_prev = (elm)->field.tqe_prev; \
	else \
		(head)->tqh_last = (elm)->field.tqe_prev; \
	*(elm)->field.tqe_prev = TAILQ_NEXT((elm), field); \
} while (0)

#endif
