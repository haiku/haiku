/*
 * Copyright 2009, Colin Günther. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_MACHINE_BUS_H_
#define _FBSD_COMPAT_MACHINE_BUS_H_


#include <machine/_bus.h>


// TODO: x86 specific!
#define I386_BUS_SPACE_IO			0
#define I386_BUS_SPACE_MEM			1

#define BUS_SPACE_MAXADDR_32BIT		0xffffffff
#define BUS_SPACE_MAXADDR			0xffffffff

#define BUS_SPACE_MAXSIZE_32BIT		0xffffffff
#define BUS_SPACE_MAXSIZE			0xffffffff

#define BUS_SPACE_UNRESTRICTED	(~0)

#define BUS_SPACE_BARRIER_READ		1
#define BUS_SPACE_BARRIER_WRITE		2


uint8_t bus_space_read_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint16_t bus_space_read_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
uint32_t bus_space_read_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset);
void bus_space_write_1(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint8_t value);
void bus_space_write_2(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint16_t value);
void bus_space_write_4(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, uint32_t value);

void bus_space_write_multi_1(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, const u_int8_t *addr, size_t count);


static __inline void
bus_space_read_region_4(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, u_int32_t *addr, size_t count)
{

	if (tag == I386_BUS_SPACE_IO) {
		int _port_ = bsh + offset;
		__asm __volatile("				\n\
			cld					\n\
		1:	inl %w2,%%eax				\n\
			stosl					\n\
			addl $4,%2				\n\
			loop 1b"				:
		    "=D" (addr), "=c" (count), "=d" (_port_)	:
		    "0" (addr), "1" (count), "2" (_port_)	:
		    "%eax", "memory", "cc");
	} else {
		void* _port_ = (void*) (bsh + offset);
		memcpy(addr, _port_, count);
	}
}


static __inline void
bus_space_write_region_1(bus_space_tag_t tag, bus_space_handle_t bsh,
	bus_size_t offset, const u_int8_t *addr, size_t count)
{
	if (tag == I386_BUS_SPACE_IO) {
		int _port_ = bsh + offset;
		__asm __volatile("				\n\
			cld					\n\
		1:	lodsb					\n\
			outb %%al,%w0				\n\
			incl %0					\n\
			loop 1b"				:
		    "=d" (_port_), "=S" (addr), "=c" (count)	:
		    "0" (_port_), "1" (addr), "2" (count)	:
		    "%eax", "memory", "cc");
	} else {
		void* _port_ = (void*) (bsh + offset);
		memcpy(_port_, addr, count);
	}
}


static inline void
bus_space_barrier(bus_space_tag_t tag, bus_space_handle_t handle,
	bus_size_t offset, bus_size_t len, int flags)
{
	if (flags & BUS_SPACE_BARRIER_READ)
		__asm__ __volatile__ ("lock; addl $0,0(%%esp)" : : : "memory");
	else
		__asm__ __volatile__ ("" : : : "memory");
}


#include <machine/bus_dma.h>


#define bus_space_write_stream_4(t, h, o, v) \
	bus_space_write_4((t), (h), (o), (v))

#endif /* _FBSD_COMPAT_MACHINE_BUS_H_ */
