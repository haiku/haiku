#ifndef B44MM_H
#define B44MM_H

#include <PCI.h>
#include <Drivers.h>
#include <OS.h>
#include <ByteOrder.h>
#include <KernelExport.h>

#include <stdio.h>

typedef vint32 MM_ATOMIC_T;

//#define MM_SWAP_LE16(x) B_SWAP_INT16(x)
#define MM_SWAP_LE16(x) x

/*#define MM_ATOMIC_SET(ptr, val) atomic_and(ptr, 0); atomic_add(ptr,val)
#define MM_ATOMIC_READ(ptr) atomic_add(ptr,0)
#define MM_ATOMIC_INC(ptr) atomic_add(ptr,1)
#define MM_ATOMIC_ADD(ptr, val) atomic_add(ptr,val)
#define MM_ATOMIC_DEC(ptr) atomic_add(ptr,-1)
#define MM_ATOMIC_SUB(ptr, val) atomic_add(ptr,0-val)*/

#define MM_ATOMIC_SET(ptr, val) *(ptr)=val
#define MM_ATOMIC_READ(ptr) *(ptr)
#define MM_ATOMIC_INC(ptr) (*(ptr))++
#define MM_ATOMIC_ADD(ptr, val) *(ptr)+=val
#define MM_ATOMIC_DEC(ptr) (*(ptr))--
#define MM_ATOMIC_SUB(ptr, val) *(ptr)-=val

/* All critical sections are protected by locking mechanisms already */

#define __io_virt(x) ((void *)(x))
#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))
#define writel(b,addr) (*(volatile unsigned int *) __io_virt(addr) = (b))
#define __raw_readl readl
#define __raw_writel writel

#define udelay spin

#define MM_MEMWRITEL(ptr, val) __raw_writel(val, ptr)
#define MM_MEMREADL(ptr) __raw_readl(ptr)

#ifdef __INTEL__
#define mb()    __asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")
#else
#ifdef __HAIKU__
#define mb()	memory_write_barrier()
#else
#warning no memory barrier function defined.
#define mb()
#endif
#endif
#define wmb()    mb()

#define readl(addr) (*(volatile unsigned int *) __io_virt(addr))

#define MM_MB() mb()
#define MM_WMB() wmb()

#define STATIC static

extern int b44_Packet_Desc_Size;

#define B44_MM_PACKET_DESC_SIZE b44_Packet_Desc_Size

#include "b44lm.h"
#include "b44queue.h"
#include "b44.h"

struct be_b44_dev {
	LM_DEVICE_BLOCK lm_dev;

	struct pci_info pci_data;

	sem_id packet_release_sem;
	//sem_id interrupt_sem;
	//thread_id interrupt_handler;

    LM_RX_PACKET_Q RxPacketReadQ;

	void *mem_list[16];
	int mem_list_num;

	area_id lockmem_list[16];
	int lockmem_list_num;

	area_id mem_base;

	vint32 opened;

	int block;
	spinlock lock;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	sem_id				linkChangeSem;
#endif
};

struct B_UM_PACKET {
	struct _LM_PACKET pkt;

	void *data;
	size_t size;
};

static inline void b44_MM_MapRxDma(PLM_DEVICE_BLOCK pDevice,
	struct _LM_PACKET *pPacket,
	LM_UINT32 *paddr)
{
	physical_entry entry;

	get_memory_map(pPacket->u.Rx.pRxBufferVirt,pPacket->u.Rx.RxBufferSize,&entry,1);
	*paddr = entry.address;
}

static inline void b44_MM_MapTxDma(PLM_DEVICE_BLOCK pDevice,
	struct _LM_PACKET *pPacket,
	LM_UINT32 *paddr, LM_UINT32 *len, int frag)
{
	struct B_UM_PACKET *pkt = (struct B_UM_PACKET *)pPacket;
	physical_entry entry;

	get_memory_map(pkt->data,pkt->size,&entry,1);
	*paddr = entry.address;
	*len = pPacket->PacketSize;
}

#if (BITS_PER_LONG == 64)
#define B44_MM_GETSTATS(_Ctr) \
	(unsigned long) (_Ctr).Low + ((unsigned long) (_Ctr).High << 32)
#else
#define B44_MM_GETSTATS(_Ctr) \
	(unsigned long) (_Ctr).Low
#endif

#define B44_MM_PTR(_ptr)   ((unsigned long) (_ptr))
#define printf(fmt, args...) dprintf(fmt, ##args)
#define DbgPrint(fmt, arg...) dprintf(fmt, ##arg)
#define DbgBreakPoint()
#define b44_MM_Wait(time) udelay(time)
#define ASSERT(expr)							\
	if (!(expr)) {							\
		dprintf("ASSERT failed: %s\n", #expr);	\
	}

#endif
