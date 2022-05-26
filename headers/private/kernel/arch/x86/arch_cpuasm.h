/*
 * Copyright 2018, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_CPUASM_H
#define _KERNEL_ARCH_x86_CPUASM_H


#define nop() __asm__ ("nop"::)

#define x86_read_cr0() ({ \
	size_t _v; \
	__asm__("mov    %%cr0,%0" : "=r" (_v)); \
	_v; \
})

#define x86_write_cr0(value) \
	__asm__("mov    %0,%%cr0" : : "r" (value))

#define x86_read_cr2() ({ \
	size_t _v; \
	__asm__("mov    %%cr2,%0" : "=r" (_v)); \
	_v; \
})

#define x86_read_cr3() ({ \
	size_t _v; \
	__asm__("mov    %%cr3,%0" : "=r" (_v)); \
	_v; \
})

static inline void
x86_write_cr3(size_t value)
{
	__asm__("mov %0,%%cr3" : : "r" (value) : "memory");
}

#define x86_read_cr4() ({ \
	size_t _v; \
	__asm__("mov    %%cr4,%0" : "=r" (_v)); \
	_v; \
})

#define x86_write_cr4(value) \
	__asm__("mov    %0,%%cr4" : : "r" (value))

#define x86_read_dr3() ({ \
	size_t _v; \
	__asm__("mov    %%dr3,%0" : "=r" (_v)); \
	_v; \
})

#define x86_write_dr3(value) \
	__asm__("mov    %0,%%dr3" : : "r" (value))

#define invalidate_TLB(va) \
	__asm__("invlpg (%0)" : : "r" (va))

#define wbinvd() \
	__asm__ volatile ("wbinvd" : : : "memory")

#define set_ac() \
	__asm__ volatile (ASM_STAC : : : "memory")

#define clear_ac() \
	__asm__ volatile (ASM_CLAC : : : "memory")

#define xgetbv(reg) ({ \
	uint32 low, high; \
	__asm__ volatile ("xgetbv" : "=a" (low), "=d" (high), "c" (reg)); \
	(low | (uint64)high << 32); \
})

#define xsetbv(reg, value) { \
	uint32 low = value; uint32 high = value >> 32; \
	__asm__ volatile ("xsetbv" : : "a" (low), "d" (high), "c" (reg)); }

#define out8(value,port) \
	__asm__ ("outb %%al,%%dx" : : "a" (value), "d" (port))

#define out16(value,port) \
	__asm__ ("outw %%ax,%%dx" : : "a" (value), "d" (port))

#define out32(value,port) \
	__asm__ ("outl %%eax,%%dx" : : "a" (value), "d" (port))

#define in8(port) ({ \
	uint8 _v; \
	__asm__ volatile ("inb %%dx,%%al" : "=a" (_v) : "d" (port)); \
	_v; \
})

#define in16(port) ({ \
	uint16 _v; \
	__asm__ volatile ("inw %%dx,%%ax":"=a" (_v) : "d" (port)); \
	_v; \
})

#define in32(port) ({ \
	uint32 _v; \
	__asm__ volatile ("inl %%dx,%%eax":"=a" (_v) : "d" (port)); \
	_v; \
})

#define out8_p(value,port) \
	__asm__ ("outb %%al,%%dx\n" \
			"\tjmp 1f\n" \
			"1:\tjmp 1f\n" \
			"1:" : : "a" (value), "d" (port))

#define in8_p(port) ({ \
	uint8 _v; \
	__asm__ volatile ("inb %%dx,%%al\n" \
			"\tjmp 1f\n" \
			"1:\tjmp 1f\n" \
			"1:" : "=a" (_v) : "d" (port)); \
	_v; \
})


#endif /* _KERNEL_ARCH_x86_CPUASM_H */
