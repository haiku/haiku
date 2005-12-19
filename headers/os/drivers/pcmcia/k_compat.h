/*
 * BeOS kernel compatibility layer
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 *  are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _BE_K_COMPAT_H
#define _BE_K_COMPAT_H

#include <KernelExport.h>
#include <ISA.h>
#include <PCI.h>
#include <Drivers.h>
#include <ByteOrder.h>
#include <SupportDefs.h>
#include <config_manager.h>
#include <stdio.h>
#include <string.h>

#define u32 uint32
#define u16 uint16
#define u8 uint8

#define __KERNEL__
#define __init
#define __exit

/* IO port access */
#define inb(p)			(isa->read_io_8)(p)
#define inw(p)			(isa->read_io_16)(p)
#define inl(p)			(isa->read_io_32)(p)
#define outb(d,p)		(isa->write_io_8)(p,d)
#define outw(d,p)		(isa->write_io_16)(p,d)
#define outl(d,p)		(isa->write_io_32)(p,d)

/* Memory-mapped IO access: unlike Linux, BeOS allows dereferencing
   pointers to mapped devices */
#define readb(p)		(*(volatile u_char *)(p))
#define readw(p)		(*(volatile u_short *)(p))
#define readl(p)		(*(volatile u_int *)(p))
#define writeb(b, p)		(*(volatile u_char *)(p) = (b))
#define writew(w, p)		(*(volatile u_short *)(p) = (w))
#define writel(l, p)		(*(volatile u_int *)(p) = (l))
#define memcpy_fromio(a, b, c)	memcpy((a), (void *)(b), (c))
#define memcpy_toio(a, b, c)	memcpy((void *)(a), (b), (c))

/* Byte swapping */
#define le16_to_cpu		B_LENDIAN_TO_HOST_INT16
#define le32_to_cpu		B_LENDIAN_TO_HOST_INT32
#define cpu_to_le16		B_HOST_TO_LENDIAN_INT16
#define cpu_to_le32		B_HOST_TO_LENDIAN_INT32
#define writew_ns		writew
#define readw_ns		readw

/* Copying data between kernel and user space: BeOS can directly
   dereference user pointers in kernel mode */
#define get_user(x, p)		((x) = *(p))
#define put_user(x, p)		(*(p) = (x))
#define copy_from_user		memcpy
#define copy_to_user		memcpy

/* Virtual memory mapping: this is somewhat inelegant, but lets us
   use drop-in replacements for the Linux equivalents */
// #define PAGE_SIZE		(0x1000)
static inline void *ioremap(u_long base, u_long size)
{
    char tag[B_OS_NAME_LENGTH];
    area_id id;
    void *virt;
    sprintf(tag, "pccard %08lx", base);
    id = map_physical_memory(tag, (void *)base,
			     size, B_ANY_KERNEL_ADDRESS,
			     B_READ_AREA | B_WRITE_AREA, &virt);
    return (id < 0) ? NULL : virt;
}
static inline void iounmap(void *virt)
{
    area_id id = area_for(virt);
    if (id >= 0) delete_area(id);
}

/* Resource management: use helper functions from the PCMCIA resource
   manager module.  RSRC_MGR needs to be defined appropriately if the
   calls are via a module_info structure. */
#define request_region(base, num, name) \
	(RSRC_MGR register_resource(B_IO_PORT_RESOURCE, (base), (num)))
#define vacate_region release_region
#define vacate_mem_region release_mem_region
#define release_region(base, num) \
	(RSRC_MGR release_resource(B_IO_PORT_RESOURCE, (base), (num)))
#define check_region(base, num) \
	(RSRC_MGR check_resource(B_IO_PORT_RESOURCE, (base), (num)))
#define request_mem_region(base, num, name) \
	(RSRC_MGR register_resource(B_MEMORY_RESOURCE, (base), (num)))
#define release_mem_region(base, num) \
	(RSRC_MGR release_resource(B_MEMORY_RESOURCE, (base), (num)))
#define check_mem_region(base, num) \
	(RSRC_MGR check_resource(B_MEMORY_RESOURCE, (base), (num)))
#define register_irq(irq) \
	(RSRC_MGR register_resource(B_IRQ_RESOURCE, (irq), 0))
#define release_irq(irq) \
	(RSRC_MGR release_resource(B_IRQ_RESOURCE, (irq), 0))
#define check_irq(irq) \
	(RSRC_MGR check_resource(B_IRQ_RESOURCE, (irq), 0))
#define ACQUIRE_RESOURCE_LOCK \
    do { module_info *m; \
    get_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME, &m); } while (0)
#define RELEASE_RESOURCE_LOCK \
    put_module(B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME)
 
/* Memory allocation.  BeOS doesn't have an atomic malloc. */
#define kmalloc(s,f)		malloc(s)
#define kfree(p)		free(p)
#define kfree_s(p,s)		free(p)
void *malloc();
void free(void *);

/* PCI configuration register access */
#define pcibios_present() (1)
#define pcibios_read_config_byte(b,df,o,v) \
	((*(v) = pci->read_pci_config(b,(df)>>3,((df)&7),o,1)),0)
#define pcibios_read_config_word(b,df,o,v) \
	((*(v) = pci->read_pci_config(b,(df)>>3,((df)&7),o,2)),0)
#define pcibios_read_config_dword(b,df,o,v) \
	((*(v) = pci->read_pci_config(b,(df)>>3,((df)&7),o,4)),0)
#define pcibios_write_config_byte(b,df,o,v) \
	(pci->write_pci_config(b,(df)>>3,((df)&7),o,1,v),0)
#define pcibios_write_config_word(b,df,o,v) \
	(pci->write_pci_config(b,(df)>>3,((df)&7),o,2,v),0)
#define pcibios_write_config_dword(b,df,o,v) \
	(pci->write_pci_config(b,(df)>>3,((df)&7),o,4,v),0)
#define PCI_VENDOR_ID		PCI_vendor_id
#define PCI_DEVICE_ID		PCI_device_id
#define PCI_COMMAND		PCI_command
#define  PCI_COMMAND_IO		PCI_command_io
#define  PCI_COMMAND_MEMORY	PCI_command_memory
#define  PCI_COMMAND_MASTER	PCI_command_master
#define  PCI_COMMAND_WAIT	PCI_command_address_step
#define PCI_STATUS		PCI_status
#define PCI_CLASS_REVISION  PCI_revision
#define PCI_CACHE_LINE_SIZE	PCI_line_size
#define PCI_LATENCY_TIMER	PCI_latency
#define PCI_INTERRUPT_LINE	PCI_interrupt_line
#define PCI_INTERRUPT_PIN	PCI_interrupt_pin
#define PCI_HEADER_TYPE		PCI_header_type
#define PCI_BASE_ADDRESS_0	PCI_base_registers
#define  PCI_BASE_ADDRESS_SPACE	PCI_address_space
#define  PCI_BASE_ADDRESS_MEM_MASK	PCI_address_memory_32_mask
#define  PCI_BASE_ADDRESS_IO_MASK	PCI_address_io_mask
#define  PCI_BASE_ADDRESS_MEM_PREFETCH	PCI_address_prefetchable
#define PCI_FUNC(devfn)		((devfn)&7)
#define PCI_SLOT(devfn)		((devfn)>>3)
#define PCI_DEVFN(dev,fn)	(((dev)<<3)|((fn)&7))
#define PCI_CLASS_BRIDGE_PCMCIA	0x0605
#define PCI_CLASS_BRIDGE_CARDBUS 0x0607

/* Atomic test-and-set */
#define test_and_set_bit(b,p)	(((atomic_or(p,(1<<(b))))>>(b))&1)

/* Spin locks */
#define __SMP__
#define spinlock_t		spinlock
#define USE_SPIN_LOCKS
#define SPIN_LOCK_UNLOCKED	0
#define spin_lock_irqsave(l,f) \
    do { f = disable_interrupts(); acquire_spinlock(l); } while (0)
#define spin_unlock_irqrestore(l,f) \
    do { release_spinlock(l); restore_interrupts(f); } while (0)

/* Interrupt handling */
#define request_irq(i,h,f,n,d)	install_io_interrupt_handler(i,h,d,0)
#define free_irq(i,h)	remove_io_interrupt(i,h)
//#define REQUEST_IRQ(i,h,f,n,d)	install_io_interrupt_handler(i,h,d,0)
//#define FREE_IRQ(i,h,d)		remove_io_interrupt(i,h)
//#define IRQ(i,d,r)		(d)
#define IRQ
#define DEV_ID			dev_id
#define NR_IRQS			16
#define SA_SHIRQ		1

#define init_waitqueue(w)	memset((w), 0, sizeof(*w))
#define init_waitqueue_head(w) memset((w), 0, sizeof(*w))
#define signal_pending(a)	has_signals_pending(NULL)

/* Miscellaneous services */
typedef long long k_time_t;
#define schedule_timeout(x) snooze(x)
#define udelay(d)		spin(d)
#define mdelay(d) \
    do { int i; for (i=0;i<d;i++) spin(1000); } while (0)
#define printk			dprintf
#define KERN_ERR		""
#define KERN_NOTICE		""
#define KERN_INFO		""
#define KERN_WARNING		""
#define KERN_DEBUG		""

#ifndef ENODATA
#define ENODATA ENOSPC
#endif

#include <pcmcia/cs_timer.h>
#define add_timer my_add_timer
#define del_timer my_del_timer

/* Module handling stuff */
#define MODULE_AUTHOR(x)
#define MODULE_DESCRIPTION(x)
#define MODULE_LICENSE(x)
#define MODULE_PARM(a,b)	extern int __dummy_decl
#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT

#endif /* _BE_K_COMPAT_H */
